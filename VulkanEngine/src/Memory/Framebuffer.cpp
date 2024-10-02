#include "Memory/Framebuffer.h"
#include "Memory/RenderTargetContext.h"

void VK_NAMESPACE::Framebuffer::TransitionColorAttachmentLayouts(vk::ImageLayout newLayout,
	vk::PipelineStageFlags newStage) const
{
	if (mData->ColorAttachments.empty())
		return;

	if (newLayout == mData->ColorAttachmentInfo.Layout)
		return;

	uint32_t GraphicsOwner = mProcessHandler.GetQueueManager().
		FindOptimalQueueFamilyIndex(vk::QueueFlagBits::eGraphics);

	auto [commandsAllocator, Executor] =
		mProcessHandler.FetchProcessHandler(GraphicsOwner, QueueAccessType::eWorker);

	auto commandBuffer = commandsAllocator.BeginOneTimeCommands();

	RecordTransitionColorAttachmentLayoutsInternal(commandBuffer, newLayout, newStage);

	mData->ColorAttachmentInfo.Layout = newLayout;
	mData->ColorAttachmentInfo.Stages = newStage;

	ResetColorTransitionLayoutStatesInternal();

	commandsAllocator.EndOneTimeCommands(commandBuffer, Executor);
}

void VK_NAMESPACE::Framebuffer::TransitionDepthAttachmentLayouts(vk::ImageLayout newLayout, vk::PipelineStageFlags newStage) const
{
	if (!mData->DepthStencilAttachment)
		return;

	if (newLayout == mData->DepthStencilAttachmentInfo.Layout)
		return;

	uint32_t GraphicsOwner = mProcessHandler.GetQueueManager().
		FindOptimalQueueFamilyIndex(vk::QueueFlagBits::eGraphics);

	auto [commandsAllocator, Executor] =
		mProcessHandler.FetchProcessHandler(GraphicsOwner, QueueAccessType::eWorker);

	auto commandBuffer = commandsAllocator.BeginOneTimeCommands();

	RecordTransitionDepthAttachmentLayoutsInternal(commandBuffer, newLayout, newStage);

	mData->DepthStencilAttachmentInfo.Layout = newLayout;
	mData->DepthStencilAttachmentInfo.Stages = newStage;

	ResetDepthStencilTransitionLayoutStatesInternal();

	commandsAllocator.EndOneTimeCommands(commandBuffer, Executor);
}

void VK_NAMESPACE::Framebuffer::BeginCommands(vk::CommandBuffer commandBuffer) const
{
	TraverseAllAttachments([commandBuffer](const Image& image, size_t index, AttachmentTypeFlags flags)
	{
		image.BeginCommands(commandBuffer);
	});
}

void VK_NAMESPACE::Framebuffer::RecordTransitionColorAttachmentLayouts(
	vk::ImageLayout newLayout, vk::PipelineStageFlags newStages) const
{
	TraverseAllColorAttachments([newLayout, newStages](const Image& image, size_t index)
	{
		image.RecordTransitionLayout(newLayout, newStages);
	});
}

void VK_NAMESPACE::Framebuffer::RecordTransitionDepthStencilAttachmentLayouts(
	vk::ImageLayout newLayout, vk::PipelineStageFlags newStages) const
{
	TraverseAllDepthStencilAttachments([newLayout, newStages](const Image& image, AttachmentTypeFlags flags)
	{
		image.RecordTransitionLayout(newLayout, newStages);
	});
}

void VK_NAMESPACE::Framebuffer::RecordBlit(const Framebuffer& src,
	const BlitInfo& blitInfo, bool retrievePreviousLayout /*= true*/) const
{
	std::vector<Image>& DstImages = mData->ColorAttachments;
	std::vector<Image>& SrcImages = src.mData->ColorAttachments;

	if (blitInfo.Flags & AttachmentTypeFlagBits::eColor)
	{
		for (size_t i = 0; i < DstImages.size() && i < SrcImages.size(); i++)
		{
			DstImages[i].RecordBlit(SrcImages[i], blitInfo.BlitInfo, retrievePreviousLayout);
		}
	}

	if ((blitInfo.Flags & AttachmentTypeFlags(AttachmentTypeFlagBits::eDepth |
		AttachmentTypeFlagBits::eStencil)) && mData->DepthStencilAttachment &&
		src.mData->DepthStencilAttachment)
	{
		mData->DepthStencilAttachment.RecordBlit(src.mData->DepthStencilAttachment, 
			blitInfo.BlitInfo, retrievePreviousLayout);
	}
}

void VK_NAMESPACE::Framebuffer::EndCommands() const
{
	TraverseAllAttachments([]
	(const Image& image, size_t index, AttachmentTypeFlags flags)
	{
		image.EndCommands();
	});
}

void VK_NAMESPACE::Framebuffer::ResetColorTransitionLayoutStatesInternal() const
{
	TraverseAllColorAttachments([this](const Image& image, size_t index)
	{
		image.mChunk.ImageHandles->Config.CurrLayout = mData->ColorAttachmentInfo.Layout;
		image.mChunk.ImageHandles->Config.PrevStage = mData->ColorAttachmentInfo.Stages;
	});
}

void VK_NAMESPACE::Framebuffer::ResetDepthStencilTransitionLayoutStatesInternal() const
{
	TraverseAllDepthStencilAttachments([this](const Image& image, AttachmentTypeFlags)
	{
		image.mChunk.ImageHandles->Config.CurrLayout = mData->DepthStencilAttachmentInfo.Layout;
		image.mChunk.ImageHandles->Config.PrevStage = mData->DepthStencilAttachmentInfo.Stages;
	});
}

void VK_NAMESPACE::Framebuffer::RecordTransitionImageLayoutInternal(vk::CommandBuffer commandBuffer,
	const Image& image, vk::QueueFlags flags, vk::ImageLayout newLayout, vk::PipelineStageFlags newStages) const
{
	Core::ImageLayoutTransitionInfo transitionInfo{ *image.mChunk.ImageHandles };
	transitionInfo.CmdBuffer = commandBuffer;
	transitionInfo.OldLayout = mData->ColorAttachmentInfo.Layout;
	transitionInfo.NewLayout = newLayout;
	transitionInfo.Subresource = image.GetSubresourceRanges().front();
	transitionInfo.SrcStageFlags = mData->ColorAttachmentInfo.Stages;
	transitionInfo.DstStageFlags = newStages;

	Core::Utils::FillImageTransitionLayoutMasks(transitionInfo, flags,
		mData->ColorAttachmentInfo.Layout, newLayout,
		mData->ColorAttachmentInfo.Stages, newStages);

	Core::Utils::RecordImageLayoutTransition(transitionInfo);
}

void VK_NAMESPACE::Framebuffer::RecordTransitionColorAttachmentLayoutsInternal(vk::CommandBuffer commandBuffer,
	vk::ImageLayout newLayout, vk::PipelineStageFlags newStages) const
{
	TraverseAllColorAttachments([this, commandBuffer, newLayout, newStages]
	(const Image& image, size_t index)
	{
		auto flags = mProcessHandler.GetQueueManager().GetFamilyCapabilities(
		mProcessHandler.GetQueueManager().FindOptimalQueueFamilyIndex(vk::QueueFlagBits::eGraphics));

		RecordTransitionImageLayoutInternal(commandBuffer, image, flags, newLayout, newStages);
	});
}

void VK_NAMESPACE::Framebuffer::RecordTransitionDepthAttachmentLayoutsInternal(vk::CommandBuffer commandBuffer, vk::ImageLayout newLayout, vk::PipelineStageFlags newStages) const
{
	TraverseAllDepthStencilAttachments([this, commandBuffer, newLayout, newStages]
	(const Image& image, AttachmentTypeFlags type)
	{
		auto flags = mProcessHandler.GetQueueManager().GetFamilyCapabilities(
			mProcessHandler.GetQueueManager().FindOptimalQueueFamilyIndex(vk::QueueFlagBits::eGraphics));

		RecordTransitionImageLayoutInternal(commandBuffer, image, flags, newLayout, newStages);
	});
}

#include "Memory/Image.h"

VK_NAMESPACE::Core::Ref<vk::ImageView> VK_NAMESPACE::Image::
	CreateImageView(const Core::ImageViewInfo& info) const
{
	auto Device = mChunk.Device;
	auto ImageHandles = mChunk.ImageHandles;

	if (info == mChunk.ImageHandles->IdentityViewInfo)
		return Core::CreateRef(mChunk.ImageHandles->IdentityView, [Device, ImageHandles](vk::ImageView) {});

	auto Handle = Core::Utils::CreateImageView(*mChunk.Device, mChunk.ImageHandles->Handle, info);

	return Core::CreateRef(Handle, [Device, ImageHandles](vk::ImageView view)
	{ Device->destroyImageView(view); });
}

void VK_NAMESPACE::Image::Blit(const Image& src, const ImageBlitInfo imageBlitInfo)
{
	uint32_t Owner = mChunk.ImageHandles->Config.ResourceOwner;
	auto OwnerCaps = mProcessHandler.GetQueueManager().GetFamilyCapabilities(Owner);

	auto [CommandsAllocator, Executor] =
		mProcessHandler.FetchProcessHandler(Owner, QueueAccessType::eWorker);

	auto CmdBuffer = CommandsAllocator.BeginOneTimeCommands();

	RecordTransitionLayoutInternal(vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, 
		mChunk.ImageHandles->Config.CurrLayout, mChunk.ImageHandles->Config.PrevStage,
		CmdBuffer, OwnerCaps);

	src.RecordTransitionLayoutInternal(vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, 
		src.mChunk.ImageHandles->Config.CurrLayout, src.mChunk.ImageHandles->Config.PrevStage,
		CmdBuffer, OwnerCaps);

	RecordBlitInternal(imageBlitInfo, vk::ImageLayout::eTransferDstOptimal,
		src, vk::ImageLayout::eTransferSrcOptimal, CmdBuffer);

	RecordTransitionLayoutInternal(mChunk.ImageHandles->Config.CurrLayout, 
		mChunk.ImageHandles->Config.PrevStage, 
		vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, CmdBuffer, OwnerCaps);

	src.RecordTransitionLayoutInternal(src.mChunk.ImageHandles->Config.CurrLayout, 
		src.mChunk.ImageHandles->Config.PrevStage, 
		vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, CmdBuffer, OwnerCaps);

	CommandsAllocator.EndOneTimeCommands(CmdBuffer, Executor);
}

void VK_NAMESPACE::Image::TransitionLayout(
	vk::ImageLayout NewLayout, vk::PipelineStageFlags usageStage) const
{
	_STL_ASSERT(NewLayout != vk::ImageLayout::eUndefined && NewLayout != vk::ImageLayout::ePreinitialized,
		"Can't transition image layout to Undefined or Preinitialized format!");

	if (mChunk.ImageHandles->Config.CurrLayout == NewLayout)
		return;

	uint32_t Owner = mChunk.ImageHandles->Config.ResourceOwner;
	auto [CommandsAllocator, Executor] =
		mProcessHandler.FetchProcessHandler(Owner, QueueAccessType::eWorker);

	auto QueueCaps = mProcessHandler.GetQueueManager().GetFamilyCapabilities(Owner);

	auto CmdBuffer = CommandsAllocator.BeginOneTimeCommands();

	RecordTransitionLayoutInternal(NewLayout, usageStage, 
		mChunk.ImageHandles->Config.CurrLayout, mChunk.ImageHandles->Config.PrevStage,
		CmdBuffer, QueueCaps);

	CommandsAllocator.EndOneTimeCommands(CmdBuffer, Executor);

	mChunk.ImageHandles->Config.CurrLayout = NewLayout;
	mChunk.ImageHandles->Config.PrevStage = usageStage;
}

void VK_NAMESPACE::Image::BeginCommands(vk::CommandBuffer commandBuffer) const
{
	DefaultBegin(commandBuffer);

	mChunk.mRecordedLayout = { mChunk.ImageHandles->Config.CurrLayout,
	mChunk.ImageHandles->Config.PrevStage };
}

void VK_NAMESPACE::Image::RecordBlit(const Image& src, 
	const ImageBlitInfo& blitInfo, bool restoreOriginalLayout /*= true*/) const
{
	auto DstLayout = mChunk.mRecordedLayout;
	auto SrcLayout = src.mChunk.mRecordedLayout;

	auto OwnerCaps = mProcessHandler.GetQueueManager().GetFamilyCapabilities(
		mChunk.ImageHandles->Config.ResourceOwner);

	RecordTransitionLayout(vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer);
	src.RecordTransitionLayoutInternal(
		vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer,
		SrcLayout.Layout, SrcLayout.Stages, mWorkingCommandBuffer, OwnerCaps);

	RecordBlitInternal(blitInfo, vk::ImageLayout::eTransferDstOptimal,
		src, vk::ImageLayout::eTransferSrcOptimal, mWorkingCommandBuffer);

	if(restoreOriginalLayout)
		RecordTransitionLayout(DstLayout.Layout, DstLayout.Stages);

	src.RecordTransitionLayoutInternal(SrcLayout.Layout, SrcLayout.Stages,
		vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, 
		mWorkingCommandBuffer, OwnerCaps);
}

void VK_NAMESPACE::Image::RecordTransitionLayout(vk::ImageLayout newLayout, 
	vk::PipelineStageFlags usageStage) const
{
	if (mChunk.mRecordedLayout.Layout == newLayout)
		return;

	auto OwnerCaps = mProcessHandler.GetQueueManager().GetFamilyCapabilities(
		mChunk.ImageHandles->Config.ResourceOwner);

	RecordTransitionLayoutInternal(newLayout, usageStage,
		mChunk.mRecordedLayout.Layout, mChunk.mRecordedLayout.Stages,
		mWorkingCommandBuffer, OwnerCaps);

	mChunk.mRecordedLayout = { newLayout, usageStage };
}

void VK_NAMESPACE::Image::EndCommands() const
{
	RecordTransitionLayout(mChunk.ImageHandles->Config.CurrLayout,
		mChunk.ImageHandles->Config.PrevStage);

	DefaultEnd();
}

void VK_NAMESPACE::Image::TransferOwnership(uint32_t queueFamilyIndex) const
{
	if (mChunk.ImageHandles->Config.ResourceOwner == queueFamilyIndex)
		return;

	vk::ImageLayout PrevLayout = mChunk.ImageHandles->Config.CurrLayout;
	vk::PipelineStageFlags PrevStage = mChunk.ImageHandles->Config.PrevStage;

	TransitionLayout(vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTopOfPipe);

	ReleaseImage(queueFamilyIndex);
	AcquireImage(queueFamilyIndex);

	TransitionLayout(PrevLayout, PrevStage);
}

void VK_NAMESPACE::Image::TransferOwnership(vk::QueueFlagBits Cap) const
{
	uint32_t DstQueueFamily = mProcessHandler.GetQueueManager().FindOptimalQueueFamilyIndex(Cap);
	TransferOwnership(DstQueueFamily);
}

std::vector<vk::ImageSubresourceRange> VK_NAMESPACE::Image::GetSubresourceRanges() const
{
	/* TODO: Hard coding it now, it needs to be addressed as soon as possible!!! */

	vk::ImageSubresourceRange Subresource = mChunk.ImageHandles->IdentityViewInfo.Subresource;

	/* ------------------------------------------------------------------------- */

	return { Subresource };
}

std::vector<vk::ImageSubresourceLayers> VK_NAMESPACE::Image::GetSubresourceLayers() const
{
	vk::ImageSubresourceLayers Subresource;

	/* TODO: Hard coding it now, it needs to be addressed as soon as possible!!! */

	Subresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	Subresource.mipLevel = 0;
	Subresource.layerCount = 1;
	Subresource.baseArrayLayer = 0;

	/* ------------------------------------------------------------------------- */

	return { Subresource };
}

void VK_NAMESPACE::Image::RecordTransitionLayoutInternal(vk::ImageLayout newLayout, 
	vk::PipelineStageFlags newStages, vk::ImageLayout oldLayout, 
	vk::PipelineStageFlags oldStages, vk::CommandBuffer CmdBuffer, vk::QueueFlags QueueCaps) const
{
	if (newLayout == oldLayout)
		return;

	Core::ImageLayoutTransitionInfo transitionInfo{ *mChunk.ImageHandles };

	transitionInfo.OldLayout = oldLayout;
	transitionInfo.NewLayout = newLayout;
	transitionInfo.CmdBuffer = CmdBuffer;

	transitionInfo.Subresource = GetSubresourceRanges().front();

	transitionInfo.SrcStageFlags = oldStages;
	transitionInfo.DstStageFlags = newStages;

	Core::Utils::FillImageTransitionLayoutMasks(transitionInfo, QueueCaps, transitionInfo.OldLayout,
		newLayout, transitionInfo.SrcStageFlags, newStages);
	Core::Utils::RecordImageLayoutTransition(transitionInfo);
}

void VK_NAMESPACE::Image::RecordBlitInternal(const ImageBlitInfo& imageBlitInfo, vk::ImageLayout dstLayout,
	const Image& src, vk::ImageLayout srcLayout, vk::CommandBuffer CmdBuffer) const
{
	vk::ImageBlit blitInfo{};

	blitInfo.srcOffsets[0] = vk::Offset3D(imageBlitInfo.SrcBeginRegion.x, imageBlitInfo.SrcBeginRegion.y, 0);
	blitInfo.srcOffsets[1] = vk::Offset3D(imageBlitInfo.SrcEndRegion.x, imageBlitInfo.SrcEndRegion.y, 1);

	blitInfo.dstOffsets[0] = vk::Offset3D(imageBlitInfo.DstBeginRegion.x, imageBlitInfo.DstBeginRegion.y, 0);
	blitInfo.dstOffsets[1] = vk::Offset3D(imageBlitInfo.DstEndRegion.x, imageBlitInfo.DstEndRegion.y, 1);

	blitInfo.srcSubresource = src.GetSubresourceLayers().front();
	blitInfo.dstSubresource = GetSubresourceLayers().front();

	CmdBuffer.blitImage(src.mChunk.ImageHandles->Handle, srcLayout, mChunk.ImageHandles->Handle,
		dstLayout, blitInfo, imageBlitInfo.Filter);
}

void VK_NAMESPACE::Image::PerformImageCopyTaskGPU(Core::Image& DstImage, const Core::Image& SrcImage,
	const vk::ArrayProxy<vk::ImageCopy>& CopyRegions) const
{
	uint32_t Owner = mChunk.ImageHandles->Config.ResourceOwner;
	auto OwnerCaps = mProcessHandler.GetQueueManager().GetFamilyCapabilities(Owner);

	auto [CommandsAllocator, Executor] =
		mProcessHandler.FetchProcessHandler(Owner, QueueAccessType::eWorker);

	auto CmdBuffer = CommandsAllocator.BeginOneTimeCommands();

	CmdBuffer.copyImage(SrcImage.Handle, SrcImage.Config.CurrLayout,
		DstImage.Handle, DstImage.Config.CurrLayout, CopyRegions);

	CommandsAllocator.EndOneTimeCommands(CmdBuffer, Executor);
}

void VK_NAMESPACE::Image::PerformBufferToImageCopyTaskGPU(Core::Image& DstImage, 
	const Core::Buffer& SrcBuffer, const vk::ArrayProxy<vk::BufferImageCopy>& CopyRegions) const
{
	uint32_t Owner = mChunk.ImageHandles->Config.ResourceOwner;
	auto OwnerCaps = mProcessHandler.GetQueueManager().GetFamilyCapabilities(Owner);

	auto [CommandsAllocator, Executor] = mProcessHandler.FetchProcessHandler(Owner, QueueAccessType::eWorker);

	auto CmdBuffer = CommandsAllocator.BeginOneTimeCommands();

	CmdBuffer.copyBufferToImage(SrcBuffer.Handle, DstImage.Handle, 
		DstImage.Config.CurrLayout, CopyRegions);

	CommandsAllocator.EndOneTimeCommands(CmdBuffer, Executor);
}

void VK_NAMESPACE::Image::ReleaseImage(uint32_t dstQueueFamily) const
{
	uint32_t Owner = mChunk.ImageHandles->Config.ResourceOwner;
	auto OwnerCaps = mProcessHandler.GetQueueManager().GetFamilyCapabilities(Owner);

	auto [CommandsAllocator, Executor] = mProcessHandler.FetchProcessHandler(Owner, QueueAccessType::eWorker);

	auto CmdBuffer = CommandsAllocator.BeginOneTimeCommands();

	Core::ImageLayoutTransitionInfo transferInfo{*mChunk.ImageHandles};
	transferInfo.CmdBuffer = CmdBuffer;
	transferInfo.OldLayout = mChunk.ImageHandles->Config.CurrLayout;

	transferInfo.SrcStageFlags = mChunk.ImageHandles->Config.PrevStage;
	transferInfo.DstStageFlags = vk::PipelineStageFlagBits::eTransfer;

	Core::Utils::FillImageReleaseMasks(transferInfo, OwnerCaps, 
		transferInfo.OldLayout, transferInfo.SrcStageFlags);
	transferInfo.Subresource = GetSubresourceRanges().front();

	Core::Utils::RecordImageLayoutTransition(transferInfo, Owner, dstQueueFamily);

	CommandsAllocator.EndOneTimeCommands(CmdBuffer, Executor);
}

void VK_NAMESPACE::Image::AcquireImage(uint32_t dstQueueFamily) const
{
	uint32_t& Owner = mChunk.ImageHandles->Config.ResourceOwner;
	auto OwnerCaps = mProcessHandler.GetQueueManager().GetFamilyCapabilities(dstQueueFamily);

	auto [CommandsAllocator, Executor] = 
		mProcessHandler.FetchProcessHandler(dstQueueFamily, QueueAccessType::eWorker);

	auto CmdBuffer = CommandsAllocator.BeginOneTimeCommands();

	Core::ImageLayoutTransitionInfo transferInfo{ *mChunk.ImageHandles };
	transferInfo.CmdBuffer = CmdBuffer;
	transferInfo.NewLayout = mChunk.ImageHandles->Config.CurrLayout;

	if (transferInfo.NewLayout == vk::ImageLayout::eUndefined ||
		transferInfo.NewLayout == vk::ImageLayout::ePreinitialized)
	{
		transferInfo.NewLayout = vk::ImageLayout::eGeneral;
	}

	transferInfo.SrcStageFlags = vk::PipelineStageFlagBits::eTransfer;
	transferInfo.DstStageFlags = mChunk.ImageHandles->Config.PrevStage;

	Core::Utils::FillImageAcquireMasks(transferInfo, OwnerCaps,
		transferInfo.NewLayout, transferInfo.DstStageFlags);

	transferInfo.Subresource = GetSubresourceRanges().front();

	Core::Utils::RecordImageLayoutTransition(transferInfo, Owner, dstQueueFamily);

	CommandsAllocator.EndOneTimeCommands(CmdBuffer, Executor);

	Owner = dstQueueFamily;

	mChunk.ImageHandles->Config.CurrLayout = transferInfo.NewLayout;
	mChunk.ImageHandles->Config.PrevStage = transferInfo.DstStageFlags;
}

void VK_NAMESPACE::Image::MakeHollow()
{
	mChunk.ImageHandles.Reset();
}

void VK_NAMESPACE::RecordBlitImages(vk::CommandBuffer commandBuffer,
	Image& Dst, vk::ImageLayout dstLayout, 
	const Image& Src, vk::ImageLayout srcLayout, const ImageBlitInfo& blitInfo)
{
	vk::ImageBlit imageBlitInfo{};

	imageBlitInfo.srcOffsets[0] = vk::Offset3D(blitInfo.SrcBeginRegion.x, blitInfo.SrcBeginRegion.y, 0);
	imageBlitInfo.srcOffsets[1] = vk::Offset3D(blitInfo.SrcEndRegion.x, blitInfo.SrcEndRegion.y, 1);

	imageBlitInfo.dstOffsets[0] = vk::Offset3D(blitInfo.DstBeginRegion.x, blitInfo.DstBeginRegion.y, 0);
	imageBlitInfo.dstOffsets[1] = vk::Offset3D(blitInfo.DstEndRegion.x, blitInfo.DstEndRegion.y, 1);

	imageBlitInfo.srcSubresource = Src.GetSubresourceLayers().front();
	imageBlitInfo.dstSubresource = Dst.GetSubresourceLayers().front();

	commandBuffer.blitImage(Src.mChunk.ImageHandles->Handle, srcLayout,
		Dst.mChunk.ImageHandles->Handle, dstLayout, imageBlitInfo, blitInfo.Filter);
}

#include "Memory/RenderContext.h"
#include "Memory/MemoryResourceManager.h"

VK_NAMESPACE::Framebuffer VK_NAMESPACE::RenderContext::CreateFramebuffer(uint32_t width, uint32_t height) const
{
	FramebufferData bufferData{};
	bufferData.Width = width;
	bufferData.Height = height;
	bufferData.ParentContextData = mData;
	bufferData.AttachmentFlags = mAttachmentFlags;

	CreateImageAndImageViews(bufferData);

	std::vector<vk::ImageView> views = bufferData.ColorAttachmentViews;
	if (bufferData.AttachmentFlags &
		(AttachmentTypeFlags) (AttachmentTypeFlagBits::eDepth | AttachmentTypeFlagBits::eStencil))
		views.emplace_back(bufferData.DepthStencilAttachmentView);

	vk::FramebufferCreateInfo createInfo{};
	createInfo.setWidth(width);
	createInfo.setHeight(height);
	createInfo.setRenderPass(mData->RenderPass);
	createInfo.setLayers(1);
	createInfo.setAttachments(views);

	bufferData.Handle = mDevice->createFramebuffer(createInfo);
	auto Device = mDevice;

	Core::Ref<FramebufferData> bufferDataRef = Core::CreateRef(bufferData,
		[Device](const FramebufferData& data)
	{
		Device->destroyFramebuffer(data.Handle);
	});

	Framebuffer framebuffer{};
	framebuffer.mData = bufferDataRef;
	framebuffer.mDevice = Device;
	framebuffer.mProcessHandler = mProcessHandler;

	framebuffer.mData->ColorAttachmentInfo.Layout = vk::ImageLayout::eColorAttachmentOptimal;
	framebuffer.mData->ColorAttachmentInfo.Stages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	return framebuffer;
}

void VK_NAMESPACE::RenderContext::CreateImageAndImageViews(FramebufferData& bufferData) const
{
	auto& ColorAttachments = bufferData.ColorAttachments;
	auto& ColorAttachmentViews = bufferData.ColorAttachmentViews;

	auto& DepthStencilAttachment = bufferData.DepthStencilAttachment;
	auto& DepthStencilAttachmentView = bufferData.DepthStencilAttachmentView;

	ColorAttachments.resize(mData->Info.ColorAttachments.size());

	ColorAttachmentViews.clear();
	ColorAttachmentViews.reserve(mData->Info.ColorAttachments.size());

	size_t i = 0;

	for (const auto& attachDesc : mData->Info.ColorAttachments)
	{
		Core::ImageConfig config;
		config.Extent = vk::Extent3D(bufferData.Width, bufferData.Height, 1);
		config.Format = attachDesc.Format;
		config.LogicalDevice = *mDevice;
		config.PhysicalDevice = mPhysicalDevice;
		config.ResourceOwner = mProcessHandler.GetQueueManager()
			.FindOptimalQueueFamilyIndex(vk::QueueFlagBits::eGraphics);

		config.Usage = attachDesc.Usage | vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eTransferDst;

		::new(&ColorAttachments[i]) Image(CreateImageChunk(config));
		ColorAttachments[i].mProcessHandler = mProcessHandler;

		ColorAttachments[i].TransitionLayout(attachDesc.Layout,
			vk::PipelineStageFlagBits::eColorAttachmentOutput);

		ColorAttachmentViews.emplace_back(ColorAttachments[i].GetIdentityImageView());

		i++;
	}

	if (mAttachmentFlags & (AttachmentTypeFlags)(AttachmentTypeFlagBits::eDepth | AttachmentTypeFlagBits::eStencil))
	{
		Core::ImageConfig config;
		config.Extent = vk::Extent3D(bufferData.Width, bufferData.Height, 1);
		config.Format = mData->Info.DepthStencilAttachment.Format;
		config.LogicalDevice = *mDevice;
		config.PhysicalDevice = mPhysicalDevice;
		config.ResourceOwner = mProcessHandler.GetQueueManager().
			FindOptimalQueueFamilyIndex(vk::QueueFlagBits::eGraphics);

		config.Usage = mData->Info.DepthStencilAttachment.Usage | vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eTransferDst;

		::new(&DepthStencilAttachment) Image(CreateImageChunk(config));
		DepthStencilAttachment.mProcessHandler = mProcessHandler;
		DepthStencilAttachment.TransitionLayout(mData->Info.DepthStencilAttachment.Layout,
			vk::PipelineStageFlagBits::eColorAttachmentOutput);

		DepthStencilAttachmentView = DepthStencilAttachment.GetIdentityImageView();
	}
}

VK_NAMESPACE::VK_CORE::ImageChunk VK_NAMESPACE::RenderContext::CreateImageChunk(Core::ImageConfig& config) const
{
	Core::ImageChunk chunk{};
	chunk.Device = mDevice;

	auto ImageHandles = Core::Utils::CreateImage(config);
	auto Device = mDevice;

	chunk.ImageHandles = Core::CreateRef(ImageHandles, [Device](const Core::Image& handles)
	{
		Device->freeMemory(handles.Memory);
		Device->destroyImage(handles.Handle);
		Device->destroyImageView(handles.IdentityView);
	});

	return chunk;
}

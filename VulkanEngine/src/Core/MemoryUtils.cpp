#include "Core/Utils/MemoryUtils.h"

struct MemoryUtilsHelper {
	std::unordered_map<VkImageLayout, VkAccessFlags> mAccessFlagsByImageLayout;
	std::unordered_map<VkPipelineStageFlagBits, VkAccessFlags> mAccessFlagsByPipelineStage;
	std::unordered_map<VkQueueFlags, VkAccessFlags> mQueueFlagsAccessFlags;
	std::unordered_map<VkFormat, VkImageAspectFlags> mFormatToAspectFlags;

	// Constructor to initialize the mappings
	MemoryUtilsHelper() {
		mAccessFlagsByImageLayout = {
			{VK_IMAGE_LAYOUT_UNDEFINED, 0},
			{VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT},
			{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT},
			{VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
			{VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT},
			{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT},
			{VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT},
			{VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT},
			{VK_IMAGE_LAYOUT_PREINITIALIZED, VK_ACCESS_HOST_WRITE_BIT},
			{VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_MEMORY_READ_BIT},
			{VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT},
		};

		mAccessFlagsByPipelineStage = {
			{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0},
			{VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT},
			{VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT},
			{VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT},
			{VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT},
			{VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT},
			{VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
			{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT},
			{VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT},
			{VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT},
			{VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0},
			{VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT},
		};

		mQueueFlagsAccessFlags = {
			{VK_QUEUE_GRAPHICS_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
							VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT |
							VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
							VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
							VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT},
			{VK_QUEUE_COMPUTE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
						   VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT},
			{VK_QUEUE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT},
			{VK_QUEUE_SPARSE_BINDING_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT},
		};

		mFormatToAspectFlags = {
			// Depth formats
			{VK_FORMAT_D16_UNORM, VK_IMAGE_ASPECT_DEPTH_BIT},
			{VK_FORMAT_X8_D24_UNORM_PACK32, VK_IMAGE_ASPECT_DEPTH_BIT},
			{VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT},

			// Stencil format
			{VK_FORMAT_S8_UINT, VK_IMAGE_ASPECT_STENCIL_BIT},

			// Depth-stencil formats
			{VK_FORMAT_D16_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT},
			{VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT},
			{VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT},
		};
	}

	// Function to get access flags for a given image layout
	vk::AccessFlags GetAccessFlagsForLayout(vk::ImageLayout layout) const 
	{ return (vk::AccessFlags) mAccessFlagsByImageLayout.at((VkImageLayout) layout); }

	// Function to get access flags for a given pipeline stage
	vk::AccessFlags GetAccessFlagsForStage(vk::PipelineStageFlags stages) const 
	{ 
		VkAccessFlags accessFlags = 0;
		for (auto& pair : mAccessFlagsByPipelineStage) 
		{
			if ((VkPipelineStageFlags)stages & pair.first) {
				accessFlags |= pair.second;
			}
		}
		return (vk::AccessFlags) accessFlags;
	}

	vk::AccessFlags GetAccessFlagsForQueueFlags(vk::QueueFlags queueFlags) const 
	{
		VkAccessFlags accessFlags = 0;
		for (const auto& entry : mQueueFlagsAccessFlags) 
		{
			if ((VkQueueFlags)queueFlags & entry.first) 
				accessFlags |= entry.second;
		}

		return vk::AccessFlags(accessFlags);
	}

	vk::ImageAspectFlags GetImageAspectFlagsForFormat(vk::Format format) const
	{
		auto found = mFormatToAspectFlags.find((VkFormat) format);

		if (found == mFormatToAspectFlags.end())
			return vk::ImageAspectFlagBits::eColor;

		return (vk::ImageAspectFlags) mFormatToAspectFlags.at((VkFormat) format);
	}

} static const sMemoryHelper;

uint32_t VK_NAMESPACE::VK_CORE::VK_UTILS::FindMemoryTypeIndex(vk::PhysicalDevice device, uint32_t memTypeBits, vk::MemoryPropertyFlags memProps)
{
	auto deviceMemProps = device.getMemoryProperties();

	for (uint32_t i = 0; i < deviceMemProps.memoryTypeCount; i++)
	{
		bool supported = memTypeBits & (1 << i);
		bool sufficient = (deviceMemProps.memoryTypes[i].propertyFlags & memProps) == memProps;

		if (supported && sufficient)
			return i;
	}

	return -1;
}

VK_NAMESPACE::VK_CORE::Buffer VK_NAMESPACE::VK_CORE::VK_UTILS::CreateBuffer(BufferConfig& bufferInput)
{
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.setSharingMode(vk::SharingMode::eExclusive);
	bufferInfo.setSize(bufferInput.ElemCount * bufferInput.TypeSize);
	bufferInfo.setUsage(bufferInput.Usage);
	bufferInfo.setQueueFamilyIndices(bufferInput.ResourceOwner);

	vk::Buffer Handle = bufferInput.LogicalDevice.createBuffer(bufferInfo);
	auto memReq = bufferInput.LogicalDevice.getBufferMemoryRequirements(Handle);

	bufferInput.ElemCount = memReq.size / bufferInput.TypeSize;

	auto Memory = AllocateMemory(memReq, bufferInput.MemProps, 
		bufferInput.LogicalDevice, bufferInput.PhysicalDevice);

	bufferInput.LogicalDevice.bindBufferMemory(Handle, Memory, 0);

	return { Handle, Memory, memReq, bufferInput.ElemCount, bufferInput };
}

vk::DeviceMemory VK_NAMESPACE::VK_CORE::VK_UTILS::AllocateMemory(
	const vk::MemoryRequirements& memReq, vk::MemoryPropertyFlags props,
	vk::Device logicalDevice, vk::PhysicalDevice physicalDevice)
{
	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.setAllocationSize(memReq.size);
	allocInfo.setMemoryTypeIndex(FindMemoryTypeIndex(physicalDevice,
		memReq.memoryTypeBits, props));

	return logicalDevice.allocateMemory(allocInfo);
}

void VK_NAMESPACE::VK_CORE::VK_UTILS::RecordBufferTransferBarrier(const BufferOwnershipTransferInfo& barrierInfo)
{
	vk::BufferMemoryBarrier barrier{};
	barrier.setBuffer(barrierInfo.Buffer.Handle);
	barrier.setSrcQueueFamilyIndex(barrierInfo.SrcFamilyIndex);
	barrier.setDstQueueFamilyIndex(barrierInfo.DstFamilyIndex);
	barrier.setOffset(0);
	barrier.setSize(VK_WHOLE_SIZE);
	barrier.setSrcAccessMask(barrierInfo.SrcAccess);
	barrier.setDstAccessMask(barrierInfo.DstAccess);

	barrierInfo.CmdBuffer.pipelineBarrier(
		barrierInfo.SrcStage,
		barrierInfo.DstStage,
		{}, {}, barrier, {});
}

vk::AccessFlags VK_NAMESPACE::VK_CORE::VK_UTILS::GetAllBufferAccessFlags(vk::QueueFlagBits flag)
{
	// TODO: Implement empty flags

	switch (flag)
	{
		case vk::QueueFlagBits::eGraphics:
			return vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eVertexAttributeRead |
				vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite |
				vk::AccessFlagBits::eUniformRead;
		case vk::QueueFlagBits::eCompute:
			return vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite |
				vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite |
				vk::AccessFlagBits::eUniformRead;
		case vk::QueueFlagBits::eTransfer:
			return vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite |
				vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite;
		case vk::QueueFlagBits::eSparseBinding:
			return vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite;
			// Not really using these queue abilities as for now
		case vk::QueueFlagBits::eProtected:
			return vk::AccessFlagBits();
		case vk::QueueFlagBits::eVideoDecodeKHR:
			return vk::AccessFlagBits();
		case vk::QueueFlagBits::eVideoEncodeKHR:
			return vk::AccessFlagBits();
		case vk::QueueFlagBits::eOpticalFlowNV:
			return vk::AccessFlagBits();
		default:
			return vk::AccessFlagBits();
	}
}

vk::PipelineStageFlags VK_NAMESPACE::VK_CORE::VK_UTILS::GetAllBufferStageFlags(vk::QueueFlagBits flag)
{
	switch (flag)
	{
		case vk::QueueFlagBits::eGraphics:
			return vk::PipelineStageFlagBits::eTopOfPipe | vk::PipelineStageFlagBits::eVertexInput
				| vk::PipelineStageFlagBits::eBottomOfPipe;
		case vk::QueueFlagBits::eCompute:
			return vk::PipelineStageFlagBits::eComputeShader;
		case vk::QueueFlagBits::eTransfer:
			return vk::PipelineStageFlagBits::eTransfer;
		case vk::QueueFlagBits::eSparseBinding:
			return vk::PipelineStageFlagBits::eTransfer;
		// Not really using these queue abilities as for now
		case vk::QueueFlagBits::eProtected:
			return vk::PipelineStageFlagBits();
		case vk::QueueFlagBits::eVideoDecodeKHR:
			return vk::PipelineStageFlagBits();
		case vk::QueueFlagBits::eVideoEncodeKHR:
			return vk::PipelineStageFlagBits();
		case vk::QueueFlagBits::eOpticalFlowNV:
			return vk::PipelineStageFlagBits();
		default:
			return vk::PipelineStageFlagBits();
	}
}

void VK_NAMESPACE::VK_CORE::VK_UTILS::FillBufferReleaseMasks(
	BufferOwnershipTransferInfo& Info, vk::QueueFlags SrcFlags)
{
	auto SrcCapBits = BreakIntoIndividualFlagBits(SrcFlags);

	vk::AccessFlags SrcAccessMask;

	vk::PipelineStageFlags SrcStageMask;

	for (auto SrcCapBit : SrcCapBits)
	{
		SrcAccessMask |= GetAllBufferAccessFlags(SrcCapBit);
		SrcStageMask |= GetAllBufferStageFlags(SrcCapBit);
	}

	Info.SrcAccess = SrcAccessMask;
	Info.SrcStage = SrcStageMask;
	Info.DstAccess = vk::AccessFlagBits();
	Info.DstStage = vk::PipelineStageFlagBits::eBottomOfPipe;
}

void VK_NAMESPACE::VK_CORE::VK_UTILS::FillBufferAcquireMasks(
	BufferOwnershipTransferInfo& Info, vk::QueueFlags DstFlags)
{
	auto DstCapBits = BreakIntoIndividualFlagBits(DstFlags);

	vk::AccessFlags DstAccessMask;
	vk::PipelineStageFlags DstStageMask;

	for (auto DstCapBit : DstCapBits)
	{
		DstAccessMask |= GetAllBufferAccessFlags(DstCapBit);
		DstStageMask |= GetAllBufferStageFlags(DstCapBit);
	}

	Info.DstAccess = DstAccessMask;
	Info.DstStage = DstStageMask;
	Info.SrcAccess = vk::AccessFlagBits();
	Info.SrcStage = vk::PipelineStageFlagBits::eTopOfPipe;
}

VK_NAMESPACE::VK_CORE::Image VK_NAMESPACE::VK_CORE::VK_UTILS::CreateImage(ImageConfig& config)
{
	vk::ImageCreateInfo createInfo{};
	createInfo.setExtent(config.Extent);
	createInfo.setFormat(config.Format);
	createInfo.setImageType(config.Type);
	createInfo.setTiling(config.Tiling);
	createInfo.setUsage(config.Usage);
	createInfo.setArrayLayers(1);
	createInfo.setMipLevels(1);
	createInfo.setSamples(vk::SampleCountFlagBits::e1);
	createInfo.setInitialLayout(config.CurrLayout);
	createInfo.setSharingMode(vk::SharingMode::eExclusive);

	vk::Image image = config.LogicalDevice.createImage(createInfo);
	vk::MemoryRequirements memreq = config.LogicalDevice.getImageMemoryRequirements(image);

	vk::DeviceMemory Memory = AllocateMemory(memreq, config.MemProps,
		config.LogicalDevice, config.PhysicalDevice);

	config.LogicalDevice.bindImageMemory(image, Memory, 0);

	ImageViewInfo viewInfo{};
	viewInfo.Format = config.Format;
	viewInfo.ComponentMaps =
	{
		vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity
	};

	viewInfo.Subresource.aspectMask = sMemoryHelper.GetImageAspectFlagsForFormat(config.Format);
	viewInfo.Subresource.setBaseArrayLayer(0);
	viewInfo.Subresource.setBaseMipLevel(0);
	viewInfo.Subresource.setLevelCount(1);
	viewInfo.Subresource.setLayerCount(1);

	viewInfo.Type = config.ViewType;

	auto ViewHandle = CreateImageView(config.LogicalDevice, image, viewInfo);

	return { image, Memory, memreq, config, ViewHandle, viewInfo };
}

void VK_NAMESPACE::VK_CORE::VK_UTILS::RecordImageLayoutTransition(const ImageLayoutTransitionInfo& transitionInfo)
{
	vk::ImageMemoryBarrier barrier;

	barrier.setImage(transitionInfo.Handles.Handle);
	barrier.setOldLayout(transitionInfo.OldLayout);
	barrier.setNewLayout(transitionInfo.NewLayout);
	barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
	barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
	barrier.setSrcAccessMask(transitionInfo.SrcAccessFlags);
	barrier.setDstAccessMask(transitionInfo.DstAccessFlags);
	barrier.setSubresourceRange(transitionInfo.Subresource);

	transitionInfo.CmdBuffer.pipelineBarrier(
		transitionInfo.SrcStageFlags,
		transitionInfo.DstStageFlags,
		vk::DependencyFlagBits(),
		nullptr, nullptr,
		barrier
	);
}

void VK_NAMESPACE::VK_CORE::VK_UTILS::RecordImageLayoutTransition(
	const ImageLayoutTransitionInfo& transitionInfo, uint32_t srcQueueFamily, uint32_t dstQueueFamily)
{
	vk::ImageMemoryBarrier barrier;

	barrier.setImage(transitionInfo.Handles.Handle);
	barrier.setOldLayout(transitionInfo.OldLayout);
	barrier.setNewLayout(transitionInfo.NewLayout);
	barrier.setSrcQueueFamilyIndex(srcQueueFamily);
	barrier.setDstQueueFamilyIndex(dstQueueFamily);
	barrier.setSrcAccessMask(transitionInfo.SrcAccessFlags);
	barrier.setDstAccessMask(transitionInfo.DstAccessFlags);
	barrier.setSubresourceRange(transitionInfo.Subresource);

	transitionInfo.CmdBuffer.pipelineBarrier(
		transitionInfo.SrcStageFlags,
		transitionInfo.DstStageFlags,
		vk::DependencyFlagBits(),
		nullptr, nullptr,
		barrier
	);
}

vk::ImageView VK_NAMESPACE::VK_CORE::VK_UTILS::CreateImageView(
	vk::Device device, vk::Image image, const ImageViewInfo& info)
{
	vk::ImageViewCreateInfo createInfo{};
	createInfo.setImage(image);
	createInfo.setViewType(info.Type);
	createInfo.setFormat(info.Format);
	createInfo.setComponents(info.ComponentMaps);
	createInfo.setSubresourceRange(info.Subresource);

	return device.createImageView(createInfo);
}

void VK_NAMESPACE::VK_CORE::VK_UTILS::FillImageTransitionLayoutMasks(
	ImageLayoutTransitionInfo& Info, vk::QueueFlags flags, vk::ImageLayout initLayout, 
	vk::ImageLayout finalLayout, vk::PipelineStageFlags SrcFlags, vk::PipelineStageFlags DstFlags)
{
	auto queueFamiltFlags = sMemoryHelper.GetAccessFlagsForQueueFlags(flags);

	Info.SrcAccessFlags |= (sMemoryHelper.GetAccessFlagsForLayout(initLayout) &
		sMemoryHelper.GetAccessFlagsForStage(SrcFlags)) & queueFamiltFlags;
	
	Info.DstAccessFlags |= (sMemoryHelper.GetAccessFlagsForLayout(finalLayout) &
		sMemoryHelper.GetAccessFlagsForStage(DstFlags)) & queueFamiltFlags;
}

void VK_NAMESPACE::VK_CORE::VK_UTILS::FillImageReleaseMasks(ImageLayoutTransitionInfo& info, vk::QueueFlags flags, 
	vk::ImageLayout layout, vk::PipelineStageFlags SrcFlags)
{
	info.NewLayout = vk::ImageLayout::eTransferDstOptimal;
	FillImageTransitionLayoutMasks(info, flags, layout, info.NewLayout, 
		SrcFlags, vk::PipelineStageFlagBits::eTransfer);
}

void VK_NAMESPACE::VK_CORE::VK_UTILS::FillImageAcquireMasks(
	ImageLayoutTransitionInfo& info, vk::QueueFlags flags, vk::ImageLayout layout, vk::PipelineStageFlags DstFlags)
{
	info.OldLayout = vk::ImageLayout::eTransferDstOptimal;
	FillImageTransitionLayoutMasks(info, flags, info.OldLayout, layout, 
		vk::PipelineStageFlagBits::eTransfer, DstFlags);
}


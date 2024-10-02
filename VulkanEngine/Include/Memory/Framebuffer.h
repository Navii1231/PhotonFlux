#pragma once
#include "MemoryConfig.h"
#include "glm/glm.hpp"

#include "Image.h"

#include "../Core/Utils/FramebufferUtils.h"

VK_BEGIN

struct FramebufferData
{
	vk::Framebuffer Handle;
	Core::Ref<RenderContextData> ParentContextData;

	std::vector<Image> ColorAttachments;
	// We can only have one depth stencil attachment...
	Image DepthStencilAttachment;

	std::vector<vk::ImageView> ColorAttachmentViews;
	vk::ImageView DepthStencilAttachmentView;

	AttachmentTypeFlags AttachmentFlags = AttachmentTypeFlags();

	uint32_t Width = 0;
	uint32_t Height = 0;

	mutable ImageLayoutInfo ColorAttachmentInfo;
	mutable ImageLayoutInfo DepthStencilAttachmentInfo;
};

struct BlitInfo
{
	AttachmentTypeFlags Flags = AttachmentTypeFlagBits::eColor;
	ImageBlitInfo BlitInfo;
};

class Framebuffer
{
public:
	Framebuffer() = default;

	void TransitionColorAttachmentLayouts(vk::ImageLayout newLayout, vk::PipelineStageFlags newStage) const;
	void TransitionDepthAttachmentLayouts(vk::ImageLayout newLayout, vk::PipelineStageFlags newStage) const;

	// Recording of resources manipulation...
	void BeginCommands(vk::CommandBuffer commandBuffer) const;

	void RecordTransitionColorAttachmentLayouts(vk::ImageLayout newLayout, vk::PipelineStageFlags newStages) const;
	void RecordTransitionDepthStencilAttachmentLayouts(
		vk::ImageLayout newLayout, vk::PipelineStageFlags newStages) const;

	void RecordBlit(const Framebuffer& src, const BlitInfo& blitInfo, bool retrievePreviousLayout = true) const;
	void EndCommands() const;

	const FramebufferData& GetFramebufferData() const { return *mData; }
	vk::Framebuffer GetNativeHandle() const { return mData->Handle; }

	glm::uvec2 GetResolution() const { return { mData->Width, mData->Height }; }
	AttachmentTypeFlags GetAttachmentFlags() const { return AttachmentTypeFlagBits::eColor; }

	std::vector<Image> GetColorAttachments() const { return mData->ColorAttachments; }
	Image GetDepthStencilAttachment() const { return mData->DepthStencilAttachment; }

	explicit operator bool() const { return static_cast<bool>(mData); }

private:
	Core::Ref<FramebufferData> mData;
	Core::Ref<vk::Device> mDevice;

	ProcessManager mProcessHandler;

	// Transition internal state...

	friend class RenderTargetContext;
	friend class Swapchain;

	template <typename PipelineContextType, typename BasePipeline>
	friend class GraphicsPipeline;

	template <typename Fn>
	void TraverseAllAttachments(Fn&& fn);

	template <typename Fn>
	void TraverseAllAttachments(Fn&& fn) const;

	template <typename Fn>
	void TraverseAllColorAttachments(Fn&& fn);

	template <typename Fn>
	void TraverseAllColorAttachments(Fn&& fn) const;

	template <typename Fn>
	void TraverseAllDepthStencilAttachments(Fn&& fn);

	template <typename Fn>
	void TraverseAllDepthStencilAttachments(Fn&& fn) const;

	void RecordTransitionImageLayoutInternal(vk::CommandBuffer commandBuffer, const Image& image, 
		vk::QueueFlags flags, vk::ImageLayout newLayout, vk::PipelineStageFlags newStages) const;

	void RecordTransitionColorAttachmentLayoutsInternal(vk::CommandBuffer commandBuffer,
		vk::ImageLayout newLayout, vk::PipelineStageFlags newStages) const;
	void RecordTransitionDepthAttachmentLayoutsInternal(vk::CommandBuffer commandBuffer,
		vk::ImageLayout newLayout, vk::PipelineStageFlags newStages) const;

	void ResetColorTransitionLayoutStatesInternal() const;
	void ResetDepthStencilTransitionLayoutStatesInternal() const;
};

template <typename Fn>
void VK_NAMESPACE::Framebuffer::TraverseAllDepthStencilAttachments(Fn&& fn) const
{
	if (mData->DepthStencilAttachment)
		fn(mData->DepthStencilAttachment,
			(AttachmentTypeFlags) (mData->AttachmentFlags &
				(AttachmentTypeFlags) ~(AttachmentTypeFlagBits::eColor)));
}

template <typename Fn>
void VK_NAMESPACE::Framebuffer::TraverseAllDepthStencilAttachments(Fn&& fn)
{
	if (mData->DepthStencilAttachment)
		fn(mData->DepthStencilAttachment,
			(AttachmentTypeFlags) (mData->AttachmentFlags &
				(AttachmentTypeFlags) ~(AttachmentTypeFlagBits::eColor)));
}

template <typename Fn>
void VK_NAMESPACE::Framebuffer::TraverseAllColorAttachments(Fn&& fn) const
{
	std::vector<Image>& Images = mData->ColorAttachments;

	for (size_t i = 0; i < Images.size(); i++)
		fn(Images[i], i);
}

template <typename Fn>
void VK_NAMESPACE::Framebuffer::TraverseAllColorAttachments(Fn&& fn)
{
	std::vector<Image>& Images = mData->ColorAttachments;

	for (size_t i = 0; i < Images.size(); i++)
		fn(Images[i], i);
}

template <typename Fn>
void VK_NAMESPACE::Framebuffer::TraverseAllAttachments(Fn&& fn) const
{
	std::vector<Image>& Images = mData->ColorAttachments;

	for (size_t i = 0; i < Images.size(); i++)
		fn(Images[i], i, AttachmentTypeFlagBits::eColor);

	if(mData->DepthStencilAttachment)
		fn(mData->DepthStencilAttachment, 0, 
			(AttachmentTypeFlags)(mData->AttachmentFlags &
				(AttachmentTypeFlags)~(AttachmentTypeFlagBits::eColor)));
}

template <typename Fn>
void VK_NAMESPACE::Framebuffer::TraverseAllAttachments(Fn&& fn)
{
	std::vector<Image>& Images = mData->ColorAttachments;

	for (size_t i = 0; i < Images.size(); i++)
		fn(Images[i], i, AttachmentTypeFlagBits::eColor);

	if (mData->DepthStencilAttachment)
		fn(mData->DepthStencilAttachment, 0, 
			(AttachmentTypeFlags) (mData->AttachmentFlags & 
				(AttachmentTypeFlags)~(AttachmentTypeFlagBits::eColor)));
}

VK_END

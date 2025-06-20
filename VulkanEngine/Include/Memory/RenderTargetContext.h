#pragma once
#include "MemoryConfig.h"
#include "Framebuffer.h"

VK_BEGIN

// NOTE: The render pass could hold multiple color attachments
// for which we never left any space

class RenderTargetContext
{
public:
	RenderTargetContext() = default;

	Framebuffer CreateFramebuffer(uint32_t width, uint32_t height) const;

	// TODO: Write another function which can take images from the user
	// to form a framebuffer out of them

	const RenderContextData& GetData() const { return *mData; }

	uint32_t GetColorAttachmentCount() const
	{ return static_cast<uint32_t>(mData->Info.ColorAttachments.size()); }

	const std::vector<vkEngine::ImageAttachmentInfo> GetColorAttachmentInfos() const
	{ return mData->Info.ColorAttachments; }

	const vkEngine::ImageAttachmentInfo& GetDepthStencilAttachmentInfos() const
	{ return mData->Info.DepthStencilAttachment; }

	AttachmentTypeFlags GetAttachmentFlags() const { return mAttachmentFlags; }

	explicit operator bool() const { return static_cast<bool>(mData); }

private:
	Core::Ref<RenderContextData> mData;

	ProcessManager mProcessHandler;
	Core::Ref<vk::Device> mDevice;
	vk::PhysicalDevice mPhysicalDevice;

	AttachmentTypeFlags mAttachmentFlags;

	friend class RenderContextBuilder;
	friend class Swapchain;

private:
	//Helper methods
	void CreateImageAndImageViews(FramebufferData& bufferData) const;
	Core::Ref<Core::ImageChunk> CreateImageChunk(Core::ImageConfig & config) const;
};

VK_END

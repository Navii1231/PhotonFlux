#pragma once
#include "MemoryConfig.h"
#include "Framebuffer.h"

VK_BEGIN

class RenderTargetContext
{
public:
	RenderTargetContext() = default;

	Framebuffer CreateFramebuffer(uint32_t width, uint32_t height) const;

	const RenderContextData& GetData() const { return *mData; }

	AttachmentTypeFlags GetUsageAttachments() const { return mAttachmentFlags; }

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
	Core::ImageChunk CreateImageChunk(Core::ImageConfig& config) const;
};

VK_END

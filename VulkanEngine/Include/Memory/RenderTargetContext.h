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

	// TODO: write another implementation of this very function 
	// where the user can provide the images themselves. Or use this class
	// as a state machine for storing the image creation information
	Framebuffer CreateFramebuffer(uint32_t width, uint32_t height) const;

	// TODO: Write another function which can take images from the user
	// to form a framebuffer out of them

	const RenderContextData& GetData() const { return *mData; }

	uint32_t GetColorAttachmentCount() const
	{ return static_cast<uint32_t>(mData->Info.Attachments.size() - UsingDepthOrStencilAttachment()); }

	const std::vector<vkEngine::ImageAttachmentInfo>& GetAttachmentInfos() const
	{ return mData->Info.Attachments; }

	const vkEngine::ImageAttachmentInfo& GetDepthStencilAttachmentInfos() const
	{ return mData->Info.Attachments.back(); }

	AttachmentTypeFlags GetAttachmentFlags() const { return mAttachmentFlags; }

	bool UsingDepthOrStencilAttachment() const
	{ return mData->Info.UsingDepthAttachment || mData->Info.UsingStencilAttachment; }

	QueueManagerRef GetQueueManager() const { return mQueueManager; }

	explicit operator bool() const { return static_cast<bool>(mData); }

private:
	Core::Ref<RenderContextData> mData;
	Core::Ref<vk::Device> mDevice;

	CommandPools mCommandPools;
	QueueManagerRef mQueueManager;

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

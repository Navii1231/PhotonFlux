#pragma once
#include "../Core/Config.h"
#include "../Core/Ref.h"

#include "PhysicalDevice.h"
#include "../Process/Commands.h"

#include "../Memory/Framebuffer.h"
#include "../Memory/RenderTargetContext.h"
#include "../Memory/RenderContextBuilder.h"

#include "../Memory/Image.h"

#include "../Core/Utils/SwapchainUtils.h"

VK_BEGIN

struct SwapchainFrame
{
	Framebuffer RenderTarget;

	vk::CommandBuffer CmdBuffer;
	vk::Semaphore ImageRendered;

	SwapchainFrame(Framebuffer&& renderTarget)
		: RenderTarget(std::move(renderTarget)) {}
};

struct SwapchainData
{
	CommandPools CommandPoolManager;
	CommandBufferAllocator CommandsAllocator;

	RenderTargetContext TargetContext;

	std::vector<Core::Ref<SwapchainFrame>> Frames;
	Core::Ref<vk::Semaphore> ImageAcquired;
};

class Swapchain
{
public:
	vk::ResultValue<uint32_t> AcquireNextFrame(
		Core::Ref<vk::Semaphore> semaphore, Core::Ref<vk::Fence> fence, uint64_t timeout = UINT64_MAX) const
	{
		return mDevice->acquireNextImageKHR(*mHandle, timeout, *semaphore, *fence);
	}

	vk::ResultValue<uint32_t> AcquireNextFrame(
		Core::Ref<vk::Semaphore> semaphore, uint64_t timeout = UINT64_MAX) const
	{
		return mDevice->acquireNextImageKHR(*mHandle, timeout, *semaphore, nullptr);
	}

	Core::Ref<vk::SwapchainKHR> GetHandle() const { return mHandle; }
	//Core::Ref<vk::RenderPass> GetRenderPass() const { return mRenderPass; }

	SwapchainSupportDetails GetSupportDetails() const { return mSupportDetails; }
	SwapchainInfo GetInfo() const { return mInfo; }

	SwapchainData GetSwapchainData() const { return mData; }

	QueueManagerRef GetQueueManager() const { return mQueueManager; }

	const SwapchainFrame& operator[](size_t index) const { return *mData.Frames[index]; }

private:
	Core::Ref<vk::SwapchainKHR> mHandle;
	RenderContextBuilder mRenderContextBuilder;

	SwapchainSupportDetails mSupportDetails;
	SwapchainInfo mInfo;

	QueueManagerRef mQueueManager;
	CommandPools mCommandPools;

	SwapchainData mData;

	Core::Ref<vk::Device> mDevice;
	PhysicalDevice mPhysicalDevice;

	Swapchain(Core::Ref<vk::Device> device, PhysicalDevice physicalDevice,
		const RenderContextBuilder& builder, const SwapchainInfo& info, 
		QueueManagerRef mQueueManager, CommandPools mCommandPools);

	Swapchain(const Swapchain&) = delete;
	Swapchain& operator=(const Swapchain&) = delete;

	friend class Context;

private:
	void InsertImageHandle(Image& image, vk::Image handle, vk::ImageView viewHandle, const Core::SwapchainBundle& bundle);
};

VK_END

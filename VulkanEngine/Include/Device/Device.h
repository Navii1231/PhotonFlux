#pragma once
#include "../Instance/PhysicalDeviceMenagerie.h"
#include "../Core/Ref.h"

#include "Descriptors/DescriptorPoolManager.h"

#include "../Process/Queues.h"
#include "../Process/QueueManager.h"

#include "DeviceConfig.h"
#include "Swapchain.h"

#include "../Memory/MemoryResourceManager.h"

#include "../Memory/RenderContextBuilder.h"
#include "../Pipeline/PipelineBuilder.h"

#include <unordered_map>

VK_BEGIN

class CommandBufferAllocator;
class ProcessManager;

class Device
{
public:
	explicit Device(const DeviceCreateInfo& info);

	// Sync stuff...
	Core::Ref<vk::Semaphore> CreateSemaphore();
	Core::Ref<vk::Fence> CreateFence(bool Signaled = true);

	void ResetFence(Core::Ref<vk::Fence> Fence);
	void WaitForFence(Core::Ref<vk::Fence> fence, uint64_t timeout = UINT64_MAX);

	template <typename It> 
	void WaitForFencesRange(It Begin, It End, uint64_t timeout = UINT64_MAX);

	// Commands...
	CommandPoolManager CreateCommandPoolManager(
		bool IsTransient = false, bool IsProtected = false) const;

	// Pipelines and RenderTargets...
	PipelineBuilder MakePipelineBuilder() const;

	// Vulkan RenderPass wrapped into VK_NAMESPACE::RenderContext
	RenderContextBuilder FetchRenderContextBuilder(vk::PipelineBindPoint bindPoint);

	// Descriptors...
	DescriptorPoolManager FetchDescriptorPoolManager() const;

	// Resources and memory...
	MemoryResourceManager MakeMemoryResourceManager() const;

	// Getters...
	std::shared_ptr<const QueueManager> GetQueueManager() const { return mQueueManager; }

	Core::Ref<vk::Device> GetHandle() const { return mHandle; }
	const DeviceCreateInfo& GetDeviceInfo() const { return mDeviceInfo; }

	std::shared_ptr<Swapchain> GetSwapchain() const { return mSwapchain; }

	// Process handlers and Queries...
	ProcessManager MakeProcessManager(bool IsTransient = false, bool IsProtected = false) const;

	// Swapchain creation and invalidation
	void InvalidateSwapchain(const SwapchainInvalidateInfo& newInfo);

	virtual ~Device() {}

private:
	Core::Ref<vk::Device> mHandle;

	std::shared_ptr<QueueManager> mQueueManager;

	Core::DescriptorPoolBuilder mDescPoolBuilder;
	
	DeviceCreateInfo mDeviceInfo;

	// Swapchain Stuff
	std::shared_ptr<Swapchain> mSwapchain;

private:
	// Helper functions...
	void CreateSwapchain(const SwapchainInfo& info);
	void DoSanityChecks();
};

template <typename It>
void VK_NAMESPACE::Device::WaitForFencesRange(It Begin, It End, uint64_t timeout)
{
	for (It Curr = Begin; Curr != End; Curr++)
		mHandle->waitForFences(*(*Curr), VK_TRUE, timeout);
}

VK_END

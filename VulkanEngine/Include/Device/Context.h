#pragma once
#include "../Instance/PhysicalDeviceMenagerie.h"
#include "../Core/Ref.h"

#include "Descriptors/DescriptorPoolManager.h"

#include "../Process/Queues.h"
#include "../Process/QueueManager.h"

#include "ContextConfig.h"
#include "Swapchain.h"

#include "../Memory/ResourcePool.h"

#include "../Memory/RenderContextBuilder.h"
#include "../Pipeline/PipelineBuilder.h"

VK_BEGIN

class CommandBufferAllocator;

class Context
{
public:
	Context() = default;
	explicit Context(const ContextCreateInfo& info);

	// Sync stuff...
	Core::Ref<vk::Semaphore> CreateSemaphore();
	Core::Ref<vk::Fence> CreateFence(bool Signaled = true);

	void ResetFence(Core::Ref<vk::Fence> Fence);
	void WaitForFence(Core::Ref<vk::Fence> fence, uint64_t timeout = UINT64_MAX);

	template <typename It> 
	void WaitForFencesRange(It Begin, It End, uint64_t timeout = UINT64_MAX);

	// Commands...
	CommandPools CreateCommandPools(bool IsTransient = false, bool IsProtected = false) const;

	// Pipelines and RenderTargets...
	PipelineBuilder MakePipelineBuilder() const;

	// Vulkan RenderPass wrapped in VK_NAMESPACE::RenderContext
	RenderContextBuilder FetchRenderContextBuilder(vk::PipelineBindPoint bindPoint);

	// Descriptors...
	DescriptorPoolManager FetchDescriptorPoolManager() const;

	// Resources and memory...
	ResourcePool CreateResourcePool() const;

	// Mirrored from VK_NAMESPACE::QueueManager for convenience
	// Executor for submitting work into queues asynchronously
	Core::Executor FetchExecutor(uint32_t familyIndex, QueueAccessType accessType) const
	{ return GetQueueManager()->FetchExecutor(familyIndex, accessType); }

	uint32_t FindOptimalQueueFamilyIndex(vk::QueueFlagBits flag) const 
	{ GetQueueManager()->FindOptimalQueueFamilyIndex(flag); }

	vk::QueueFlags GetFamilyCapabilities(uint32_t index) const
	{ return GetQueueManager()->GetFamilyCapabilities(index); }

	uint32_t GetQueueCount(uint32_t familyIndex) const
	{ return GetQueueManager()->GetQueueCount(familyIndex); }

	Core::QueueFamilyIndices GetPresentQueueFamilyIndices() const
	{ return GetQueueManager()->GetPresentQueueFamilyIndices(); }

	// Getters...
	QueueManagerRef GetQueueManager() const { return mQueueManager; }

	Core::Ref<vk::Device> GetHandle() const { return mHandle; }
	const ContextCreateInfo& GetDeviceInfo() const { return mDeviceInfo; }

	std::shared_ptr<Swapchain> GetSwapchain() const { return mSwapchain; }

	// Swapchain creation and invalidation
	void InvalidateSwapchain(const SwapchainInvalidateInfo& newInfo);

	explicit operator bool() const { return static_cast<bool>(mHandle); }

	virtual ~Context() {}

private:
	Core::Ref<vk::Device> mHandle;

	std::shared_ptr<QueueManager> mQueueManager;

	Core::DescriptorPoolBuilder mDescPoolBuilder;
	
	ContextCreateInfo mDeviceInfo;

	// Swapchain Stuff
	std::shared_ptr<Swapchain> mSwapchain;

private:
	// Helper functions...
	void CreateSwapchain(const SwapchainInfo& info);
	void DoSanityChecks();
};

template <typename It>
void VK_NAMESPACE::Context::WaitForFencesRange(It Begin, It End, uint64_t timeout)
{
	for (It Curr = Begin; Curr != End; Curr++)
		mHandle->waitForFences(*(*Curr), VK_TRUE, timeout);
}

VK_END

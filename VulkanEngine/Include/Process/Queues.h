#pragma once
#include "ProcessConfig.h"
#include "../Core/Ref.h"

VK_BEGIN

class Context;

VK_CORE_BEGIN

// Thin wrapper over vk::Queue and it's corresponding vk::Fence
// Thread safe
class Queue
{
public:
	Queue() = default;

	// TODO: Fix me --> These copy and assignment functionalities should not be public!
	Queue(const Queue& Other);
	Queue& operator=(const Queue& Other);

	bool Submit(vk::CommandBuffer buffer, 
		std::chrono::nanoseconds timeOut = std::chrono::nanoseconds::max()) const;

	bool Submit(vk::Semaphore signalSemaphore, vk::CommandBuffer buffer,
		std::chrono::nanoseconds timeOut = std::chrono::nanoseconds::max());

	bool Submit(const QueueWaitingPoint& waitPoint, vk::Semaphore signalSemaphore, 
		vk::CommandBuffer buffer, std::chrono::nanoseconds timeOut = std::chrono::nanoseconds::max());

	bool Submit(const vk::SubmitInfo& submitInfo,
		std::chrono::nanoseconds timeOut = std::chrono::nanoseconds::max()) const;

	bool SubmitRange(vk::CommandBuffer* Begin, vk::CommandBuffer* End, 
		std::chrono::nanoseconds timeOut = std::chrono::nanoseconds::max()) const;
	bool SubmitRange(const vk::SubmitInfo* Begin, const vk::SubmitInfo* End, 
		std::chrono::nanoseconds timeOut = std::chrono::nanoseconds::max()) const;

	bool BindSparse(const vk::BindSparseInfo& bindSparseInfo,
		std::chrono::nanoseconds timeOut = std::chrono::nanoseconds::max()) const;

	vk::Result PresentKHR(const vk::PresentInfoKHR& presentInfo) const;

	bool WaitIdle(std::chrono::nanoseconds timeOut = std::chrono::nanoseconds::max()) const;

	QueueFamily* GetQueueFamilyInfo() const { return mFamilyInfo; }
	uint32_t GetQueueIndex() const { return mQueueIndex; }

private:
	vk::Queue mHandle;
	vk::Fence mFence;

	uint32_t mQueueIndex = -1;
	QueueFamily* mFamilyInfo  = nullptr;

	mutable std::mutex mLock;
	vk::Device mDevice;

	Queue(vk::Queue handle, vk::Fence fence, uint32_t queueIndex, vk::Device device)
		: mHandle(handle), mFence(fence), mQueueIndex(queueIndex), mDevice(device) {}

	vk::Result WaitIdleAsync(uint64_t timeout) const;

	friend class Context;
	friend class QueueManager;
};

VK_CORE_END
VK_END

#include "Process/Queues.h"

VK_NAMESPACE::VK_CORE::Queue::Queue(const Queue& Other)
	: mHandle(Other.mHandle), mFence(Other.mFence), mDevice(Other.mDevice) {}

vk::Result VK_NAMESPACE::VK_CORE::Queue::WaitIdleAsync(uint64_t timeout) const
{
	return mDevice.waitForFences(mFence, VK_TRUE, timeout);
}

VK_NAMESPACE::VK_CORE::Queue& VK_NAMESPACE::VK_CORE::Queue::operator=(const Queue& Other)
{
	std::scoped_lock locker(mLock);

	mHandle = Other.mHandle;
	mFence = Other.mFence;
	mDevice = Other.mDevice;

	return *this;
}

bool VK_NAMESPACE::VK_CORE::Queue::Submit(const vk::SubmitInfo& submitInfo, 
	std::chrono::nanoseconds timeOut /*= std::chrono::nanoseconds::max()*/) const
{
	std::scoped_lock locker(mLock);

	if (WaitIdleAsync(timeOut.count()) != vk::Result::eSuccess)
		return false;

	mDevice.resetFences(mFence);
	mHandle.submit(submitInfo, mFence);

	return true;
}

bool VK_NAMESPACE::VK_CORE::Queue::Submit(vk::Semaphore signalSemaphore,
	vk::CommandBuffer buffer, std::chrono::nanoseconds timeOut /*= std::chrono::nanoseconds::max()*/)
{
	vk::SubmitInfo submitInfo{};
	submitInfo.setCommandBuffers(buffer);
	submitInfo.setSignalSemaphores(signalSemaphore);

	return Submit(submitInfo, timeOut);
}

bool VK_NAMESPACE::VK_CORE::Queue::Submit(const QueueWaitingPoint& waitPoint,
	vk::Semaphore signalSemaphore, vk::CommandBuffer buffer,
	std::chrono::nanoseconds timeOut /*= std::chrono::nanoseconds::max()*/)
{
	vk::PipelineStageFlags WaitStages = { waitPoint.WaitDst };

	vk::SubmitInfo submitInfo{};
	submitInfo.setCommandBuffers(buffer);
	submitInfo.setWaitDstStageMask(WaitStages);
	submitInfo.setWaitSemaphores(waitPoint.WaitSemaphore);
	submitInfo.setSignalSemaphores(signalSemaphore);

	return Submit(submitInfo, timeOut);
}

bool VK_NAMESPACE::VK_CORE::Queue::Submit(vk::CommandBuffer buffer,
	std::chrono::nanoseconds timeOut /*= std::chrono::nanoseconds::max()*/) const
{
	vk::SubmitInfo submitInfo{};
	submitInfo.setCommandBuffers(buffer);

	return Submit(submitInfo, timeOut);
}

bool VK_NAMESPACE::VK_CORE::Queue::SubmitRange(
	const vk::SubmitInfo* Begin, const vk::SubmitInfo* End,
	std::chrono::nanoseconds timeOut /*= std::chrono::nanoseconds::max()*/) const
{
	std::vector<vk::SubmitInfo> submitInfos(Begin, End);

	std::scoped_lock locker(mLock);

	if (WaitIdleAsync(timeOut.count()) != vk::Result::eSuccess)
		return false;

	mDevice.resetFences(mFence);
	mHandle.submit(submitInfos);

	return true;
}

bool VK_NAMESPACE::VK_CORE::Queue::SubmitRange(vk::CommandBuffer* Begin, vk::CommandBuffer* End,
	std::chrono::nanoseconds timeOut /*= std::chrono::nanoseconds::max()*/) const
{
	std::vector<vk::SubmitInfo> submitInfos(End - Begin);

	for (auto& info : submitInfos)
		info.setCommandBuffers(*(Begin++));
	
	std::scoped_lock locker(mLock);

	if (WaitIdleAsync(timeOut.count()) != vk::Result::eSuccess)
		return false;

	mDevice.resetFences(mFence);
	mHandle.submit(submitInfos);

	return true;
}

bool VK_NAMESPACE::VK_CORE::Queue::BindSparse(const vk::BindSparseInfo& bindSparseInfo,
	std::chrono::nanoseconds timeOut /*= std::chrono::nanoseconds::max()*/) const
{
	std::scoped_lock locker(mLock);

	if (WaitIdleAsync(timeOut.count()) != vk::Result::eSuccess)
		return false;

	mDevice.resetFences(mFence);
	mHandle.bindSparse(bindSparseInfo, mFence);

	return true;
}

vk::Result VK_NAMESPACE::VK_CORE::Queue::PresentKHR(const vk::PresentInfoKHR& presentInfo) const
{
	return mHandle.presentKHR(presentInfo);
}

bool VK_NAMESPACE::VK_CORE::Queue::WaitIdle(
	std::chrono::nanoseconds timeOut /*= std::chrono::nanoseconds::max()*/)const
{
	auto Result = WaitIdleAsync(timeOut.count());

	// Notify any thread, in case, it's stuck waiting for the resource to be free
	// --> The notification is received by Core::Executor::SubmitWork(...)
	if (Result == vk::Result::eSuccess)
	{
		std::scoped_lock locker(mFamilyInfo->NotifierMutex);
		mFamilyInfo->IdleNotifier.notify_one();
	}

	return Result == vk::Result::eSuccess;
}

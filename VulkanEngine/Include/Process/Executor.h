#pragma once
#include "Queues.h"
#include "QueueManager.h"

#include "../Core/Ref.h"

VK_BEGIN
VK_CORE_BEGIN

class Executor
{
public:
	Executor() = default;

	uint32_t SubmitWork(const vk::SubmitInfo& submitInfo,
		std::chrono::nanoseconds timeOut = std::chrono::nanoseconds::max());

	uint32_t SubmitWork(vk::CommandBuffer cmdBuffer,
		std::chrono::nanoseconds timeOut = std::chrono::nanoseconds::max());

	uint32_t SubmitWorkRange(const vk::SubmitInfo* begin, const vk::SubmitInfo* end,
		std::chrono::nanoseconds timeOut = std::chrono::nanoseconds::max());

	uint32_t SubmitWorkRange(vk::CommandBuffer* begin, vk::CommandBuffer* end,
		std::chrono::nanoseconds timeOut = std::chrono::nanoseconds::max());

	bool WaitIdle(std::chrono::nanoseconds timeOut = std::chrono::nanoseconds::max());

	uint32_t GetFamilyIndex() const { return mFamilyData->Index; }
	size_t GetWorkerQueueCount() const { return mFamilyData->Queues.size() - mFamilyData->WorkerBeginIndex; }
	size_t GetQueueCount() const { return mFamilyData->Queues.size(); }

	Ref<Queue> operator[](size_t index) const { return mFamilyData->Queues[index]; }

	bool operator ==(const Executor& Other) const;
	bool operator !=(const Executor& Other) const { return !(*this == Other); }

private:
	const QueueFamily* mFamilyData = nullptr;
	QueueAccessType mAccessType = QueueAccessType::eGeneric;

	friend class ::VK_NAMESPACE::QueueManager;

private:
	Executor(const QueueFamily* familyData, QueueAccessType accessType)
		: mFamilyData(familyData), mAccessType(accessType) {}

	template <typename Fn> 
	uint32_t TraverseIdleQueues(size_t begin, size_t end, std::chrono::nanoseconds timeOut, Fn fn) const;

	uint32_t SubmitToQueuesRange(size_t begin, size_t end, 
		std::chrono::nanoseconds timeOut, const vk::SubmitInfo& submitInfo) const;

	uint32_t SubmitToQueuesRange(size_t begin, size_t end, std::chrono::nanoseconds timeOut,
		const vk::SubmitInfo* submitBegin, const vk::SubmitInfo* submitEnd) const;
};

template <typename Fn> 
uint32_t Executor::TraverseIdleQueues(size_t begin, size_t end,
	std::chrono::nanoseconds timeOut, Fn fn) const
{
	const auto& queueFamily = *mFamilyData;

	while (true)
	{
		std::unique_lock locker(queueFamily.NotifierMutex);

		for (size_t Curr = begin; Curr < end; Curr++)
		{
			if (fn(queueFamily.Queues[Curr]))
				return static_cast<uint32_t>(Curr);
		}

		std::cv_status status = queueFamily.IdleNotifier.wait_for(locker, timeOut);

		if (status == std::cv_status::timeout)
			return -1;
	}

	return -1;
}

VK_CORE_END
VK_END

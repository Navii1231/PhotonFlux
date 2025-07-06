#pragma once
#include "ProcessConfig.h"
#include "Commands.h"

VK_BEGIN

class RecordableResource
{
public:
	RecordableResource(const std::shared_ptr<QueueManager>& queueManager,
		const CommandPools& commandPool)
		: mQueueManager(queueManager), mCommandPools(commandPool) {}

	virtual void BeginCommands(vk::CommandBuffer commandBuffer) const { DefaultBegin(commandBuffer); }
	virtual void EndCommands() const { DefaultEnd(); }

	template<typename Fn>
	void InvokeOneTimeProcess(uint32_t index, Fn&& fn) const;

	template <typename Fn>
	void InvokeProcess(uint32_t index, Fn&& fn) const;

	virtual ~RecordableResource() = default;

protected:
	mutable vk::CommandBuffer mWorkingCommandBuffer = nullptr;

	QueueManagerRef mQueueManager;
	CommandPools mCommandPools;

	RecordableResource() = default;

	void DefaultBegin(vk::CommandBuffer commandBuffer) const
	{
		_STL_ASSERT(mWorkingCommandBuffer == nullptr, "You must call EndCommands before calling "
			"BeginCommands again");

		mWorkingCommandBuffer = commandBuffer;
	}

	void DefaultEnd() const
	{
		_STL_ASSERT(mWorkingCommandBuffer, "EndCommands can't find a suitable command buffer\n"
			"Did you forget to call BeginCommands?");

		mWorkingCommandBuffer = nullptr;
	}

};

template <typename Fn>
void VK_NAMESPACE::RecordableResource::InvokeProcess(uint32_t index, Fn&& fn) const
{
	auto executor = mQueueManager->FetchExecutor(index, QueueAccessType::eWorker);
	auto cmdBuf = mCommandPools[index].Allocate();

	auto queue = executor.SubmitWork(fn(cmdBuf));
	executor[queue]->WaitIdle();

	mCommandPools[index].Free(cmdBuf);
}

template<typename Fn>
void VK_NAMESPACE::RecordableResource::InvokeOneTimeProcess(uint32_t index, Fn&& fn) const
{
	auto executor = mQueueManager->FetchExecutor(index, QueueAccessType::eWorker);
	auto cmdBuf = mCommandPools[index].BeginOneTimeCommands();

	// Recording done inside this function
	fn(cmdBuf);

	mCommandPools[index].EndOneTimeCommands(cmdBuf, executor);
}

VK_END

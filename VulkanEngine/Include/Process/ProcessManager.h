#pragma once
#include "Commands.h"
#include "Executor.h"

VK_BEGIN

// Combines QueueManager and CommandPoolManager
class ProcessManager
{
public:
	struct Handler
	{
		CommandBufferAllocator CmdBufferAllocator;
		Core::Executor Executor;
	};

public:
	ProcessManager() = default;

	Handler FetchProcessHandler(uint32_t queueFamily, QueueAccessType accessType) const
	{
		return { mPoolManager.GetAllocator(queueFamily), 
			mQueueInspector->FetchExecutor(queueFamily, accessType) };
	}

	const QueueManager& GetQueueManager() const { return *mQueueInspector; }
	const CommandPoolManager& GetCommandPoolManager() const { return mPoolManager; }

	explicit operator bool() const { return static_cast<bool>(mQueueInspector); }

private:
	CommandPoolManager mPoolManager;
	std::shared_ptr<const QueueManager> mQueueInspector;

	ProcessManager(CommandPoolManager&& poolManager,
		std::shared_ptr<const QueueManager> queueInspector)
		: mPoolManager(poolManager), mQueueInspector(queueInspector) {}

	friend class Device;
};

VK_END

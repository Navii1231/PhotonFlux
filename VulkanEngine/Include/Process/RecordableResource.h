#pragma once
#include "ProcessConfig.h"
#include "ProcessManager.h"

VK_BEGIN

class RecordableResource
{
public:
	RecordableResource(const ProcessManager& processManager)
		: mProcessHandler(processManager) {}

	virtual void BeginCommands(vk::CommandBuffer commandBuffer) const
	{
		DefaultBegin(commandBuffer);
	}

	virtual void EndCommands() const
	{ 
		DefaultEnd();
	}

	virtual ~RecordableResource() = default;

protected:
	mutable vk::CommandBuffer mWorkingCommandBuffer = nullptr;
	ProcessManager mProcessHandler;

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

VK_END

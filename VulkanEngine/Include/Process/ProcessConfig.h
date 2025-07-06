#pragma once
// Vulkan configuration and queue structures...
#include "../Core/Config.h"
#include "../Core/Ref.h"

VK_BEGIN
class QueueManager;

VK_CORE_BEGIN

class Queue;

using QueueFamilyIndices = std::set<uint32_t>;
// QueueCapability --> Family Indices
using QueueIndexMap = std::unordered_map<vk::QueueFlagBits, std::set<uint32_t>>;
using CommandBufferSet = std::set<vk::CommandBuffer>;

template <typename T>
using QueueFamilyMap = std::unordered_map<uint32_t, T>;

struct QueueFamily
{
	uint32_t Index = -1;
	size_t WorkerBeginIndex = -1;
	vk::QueueFlags Capabilities;

	mutable std::condition_variable IdleNotifier;
	mutable std::mutex NotifierMutex;

	std::vector<Core::Ref<Queue>> Queues;
};

struct QueueWaitingPoint
{
	vk::Semaphore WaitSemaphore{};
	vk::PipelineStageFlags WaitDst{};
};

struct CommandPoolData
{
	vk::CommandPool Handle;
	std::mutex Lock;

	CommandPoolData() = default;

	CommandPoolData(const CommandPoolData& Other)
		: Handle(Other.Handle) {}

	CommandPoolData& operator =(const CommandPoolData& Other)
	{ Handle = Other.Handle; }

	bool operator ==(const CommandPoolData& Other) const { return Handle == Other.Handle; }
};

struct CommandBufferDeleter
{
	void operator()(vk::CommandBuffer buffer) const
	{ Device->freeCommandBuffers(*CommandPool, buffer); }

	Core::Ref<vk::CommandPool> CommandPool;
	Core::Ref<vk::Device> Device;
};

VK_CORE_END
VK_END

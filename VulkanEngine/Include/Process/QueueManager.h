#pragma once
#include "Queues.h"

VK_BEGIN

enum class QueueAccessType
{
	eGeneric = 0,
	eWorker = 1,
};

VK_CORE_BEGIN

class Executor;

VK_CORE_END

class QueueManager
{
public:
	// Executor for submitting work into queues asynchronously
	Core::Executor FetchExecutor(uint32_t familyIndex, QueueAccessType accessType) const;

	uint32_t FindOptimalQueueFamilyIndex(vk::QueueFlagBits flag) const;
	vk::QueueFlags GetFamilyCapabilities(uint32_t index) const 
	{ return mQueues.at(index).Capabilities; }

	// Getter for info...
	uint32_t GetQueueCount(uint32_t familyIndex) const;

	Core::QueueFamilyIndices GetPresentQueueFamilyIndices() const;

	const Core::QueueFamily& GetQueueFamily(uint32_t familyIndex) const { return mQueues.at(familyIndex); }
	const Core::QueueFamilyIndices& GetQueueFamilyIndices() const { return mQueueIndices; }
	const Core::QueueIndexMap& GetFamilyIndicesByCapabilityMap() const { return mFamilyIndicesByCapabilities; }

private:
	Core::QueueFamilyMap<Core::QueueFamily> mQueues;

	Core::QueueFamilyIndices mQueueIndices;
	Core::QueueIndexMap mFamilyIndicesByCapabilities;

	Core::Ref<vk::Device> mDevice;

private:
	QueueManager(const Core::QueueFamilyMap<std::pair<size_t, std::vector<Core::Ref<Core::Queue>>>>& queues,
		const Core::QueueFamilyIndices& queueIndices,
		const Core::QueueIndexMap& queueCaps,
		const std::vector<vk::QueueFamilyProperties> queueProps,
		Core::Ref<vk::Device> device);

	QueueManager(const QueueManager&) = delete;
	QueueManager& operator =(const QueueManager&) = delete;

	friend class Context;
};

using QueueManagerRef = std::shared_ptr<const QueueManager>;

VK_END

#include "Process/QueueManager.h"
#include "Process/Executor.h"

#include "Core/Utils/DeviceCreation.h"

VK_NAMESPACE::Core::Executor VK_NAMESPACE::QueueManager::FetchExecutor(uint32_t familyIndex, QueueAccessType accessType) const
{
	return { &mQueues.at(familyIndex), accessType };
}

uint32_t VK_NAMESPACE::QueueManager::FindOptimalQueueFamilyIndex(vk::QueueFlagBits flag) const
{
	const auto& Indices = mFamilyIndicesByCapabilities.at(flag);

	uint32_t OptimalFamilyIndex = -1;
	size_t MinSize = -1;

	for (auto index : Indices)
	{
		auto Caps = GetFamilyCapabilities(index);
		auto IndividualFlags = Core::Utils::BreakIntoIndividualFlagBits(Caps);

		if (MinSize > IndividualFlags.size())
		{
			OptimalFamilyIndex = index;
			MinSize = IndividualFlags.size();
		}
	}

	return OptimalFamilyIndex;
}

uint32_t VK_NAMESPACE::QueueManager::GetQueueCount(uint32_t FamilyIndex) const
{
	return static_cast<uint32_t>(mQueues.at(FamilyIndex).Queues.size());
}

VK_NAMESPACE::QueueManager::QueueManager(
	const Core::QueueFamilyMap<std::pair<size_t, std::vector<Core::Ref<Core::Queue>>>>& queues, 
	const Core::QueueFamilyIndices& queueIndices, 
	const Core::QueueIndexMap& queueCaps,
	const std::vector<vk::QueueFamilyProperties> queueProps,
	Core::Ref<vk::Device> device) 

	: mQueues(), mQueueIndices(queueIndices),
	mFamilyIndicesByCapabilities(queueCaps), mDevice(device)

{
	for (const auto& queueFamily : queues)
	{
		mQueues[queueFamily.first].WorkerBeginIndex = queueFamily.second.first;
		mQueues[queueFamily.first].Index = queueFamily.first;
		mQueues[queueFamily.first].Queues = std::move(queueFamily.second.second);
		mQueues[queueFamily.first].Capabilities = queueProps[queueFamily.first].queueFlags;
	}

	for (auto& queueFamily : mQueues)
	{
		for (auto& queue : queueFamily.second.Queues)
			queue->mFamilyInfo = &queueFamily.second;
	}
}

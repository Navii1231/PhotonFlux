#pragma once
#include "../Config.h"
#include "../../Device/ContextConfig.h"
#include "../../Device/PhysicalDevice.h"

#include <unordered_set>

VK_BEGIN
VK_CORE_BEGIN
VK_UTILS_BEGIN

// Device...
vk::Device CreateDevice(const ContextCreateInfo& createInfo);

template <typename VulkanFlagBits>
std::set<VulkanFlagBits> BreakIntoIndividualFlagBits(vk::Flags<VulkanFlagBits> flags)
{
	std::set<VulkanFlagBits> flagBits;

	for (uint32_t i = 0; i < 8 * sizeof(VulkanFlagBits); i++)
	{
		auto ComposedFlag = (VulkanFlagBits)(1 << i);

		if (flags & ComposedFlag)
			flagBits.insert(ComposedFlag);
	}

	return flagBits;
}

template <typename VulkanFlagBits>
vk::Flags<VulkanFlagBits> ChainFlagsTogether(const std::set<VulkanFlagBits>& flags)
{
	vk::Flags<VulkanFlagBits> ChainedFlags;

	for (auto individual : flags)
		ChainedFlags |= individual;

	return ChainedFlags;
}

// Sync Stuff...
vk::Semaphore CreateSemaphore(vk::Device device);
vk::Fence CreateFence(vk::Device device, bool Signaled);

VK_UTILS_END
VK_CORE_END
VK_END

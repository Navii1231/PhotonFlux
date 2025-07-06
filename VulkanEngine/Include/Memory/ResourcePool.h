#pragma once
#include "MemoryConfig.h"
#include "Buffer.h"
#include "Image.h"
#include "../Process/Commands.h"

VK_BEGIN

class ResourcePool
{
public:
	ResourcePool() = default;

	template <typename T>
	Buffer<T> CreateBuffer(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memProps) const;

	GenericBuffer CreateGenericBuffer(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memProps) const;

	Image CreateImage(const ImageCreateInfo& info) const;

	Core::Ref<vk::Sampler> CreateSampler(const SamplerInfo& samplerInfo, SamplerCache cache = {}) const;

	SamplerCache CreateSamplerCache() const;

	std::shared_ptr<const QueueManager> GetQueueManager() const { return mQueueManager; }

	explicit operator bool() const { return static_cast<bool>(mDevice); }

private:
	Core::Ref<vk::Device> mDevice;
	PhysicalDevice mPhysicalDevice;

	CommandPools mBufferCommandPools;
	CommandPools mImageCommandPools;

	std::shared_ptr<const QueueManager> mQueueManager;

	friend class Context;
};

template <typename T>
VK_NAMESPACE::Buffer<T> VK_NAMESPACE::ResourcePool::CreateBuffer(
	vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memProps) const
{
	auto Device = mDevice;

	Core::BufferChunk Chunk;
	Core::BufferConfig Config{};

	Config.ElemCount = 0;
	Config.LogicalDevice = *Device;
	Config.PhysicalDevice = mPhysicalDevice.Handle;

	Config.Usage = usage | vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eTransferSrc;

	Config.MemProps = memProps;
	Config.TypeSize = sizeof(T);

	// Creating an empty buffer
	Chunk.BufferHandles = Core::CreateRef(vkEngine::Core::Buffer(),
		[Device](Core::Buffer buffer)
	{
		if (buffer.Handle)
		{
			Device->destroyBuffer(buffer.Handle);
			Device->freeMemory(buffer.Memory);
		}
	});

	Chunk.Device = Device;
	Chunk.BufferHandles->Config = Config;

	Buffer<T> buffer(std::move(Chunk));
	buffer.mQueueManager = GetQueueManager();
	buffer.mCommandPools = mBufferCommandPools;

	return buffer;
}

VK_END

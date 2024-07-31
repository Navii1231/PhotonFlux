#pragma once
#include "MemoryConfig.h"
#include "Buffer.h"
#include "Image.h"

VK_BEGIN

class MemoryResourceManager
{
public:
	MemoryResourceManager() = default;

	template <typename T>
	Buffer<T> CreateBuffer(const BufferCreateInfo& Input) const;

	Image CreateImage(const ImageCreateInfo& info) const;

	Core::Ref<vk::Sampler> CreateSampler(const SamplerInfo& samplerInfo, SamplerCache cache = {}) const;

	SamplerCache CreateSamplerCache() const;

	explicit operator bool() const { return static_cast<bool>(mDevice); }

private:
	Core::Ref<vk::Device> mDevice;
	PhysicalDevice mPhysicalDevice;

	ProcessManager mBufferProcessHandler;
	ProcessManager mImageProcessHandler;

	std::shared_ptr<const QueueManager> mQueueManager;

	friend class Device;
};

template <typename T>
VK_NAMESPACE::Buffer<T>
VK_NAMESPACE::MemoryResourceManager::CreateBuffer(const BufferCreateInfo& Input) const
{
	auto Device = mDevice;

	Core::BufferChunk Chunk;
	Core::BufferConfig Config{};

	Config.ElemCount = Input.Size == 0 ? 1 : Input.Size;
	Config.LogicalDevice = *Device;
	Config.PhysicalDevice = mPhysicalDevice.Handle;

	Config.Usage = Input.Usage | vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eTransferSrc;

	Config.MemProps = Input.MemProps;
	Config.TypeSize = sizeof(T);

	Chunk.BufferHandles = Core::CreateRef(Core::Utils::CreateBuffer(Config),
		[Device](Core::Buffer buffer)
	{
		Device->destroyBuffer(buffer.Handle);
		Device->freeMemory(buffer.Memory);
	});

	Chunk.Device = Device;
	Chunk.BufferHandles->ElemCount = Input.Size;

	Buffer<T> buffer(std::move(Chunk));

	buffer.mProcessHandler = mBufferProcessHandler;
	return buffer;
}

VK_END

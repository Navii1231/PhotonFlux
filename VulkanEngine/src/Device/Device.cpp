#include "Device/Device.h"

#include "../Process/Commands.h"
#include "../Process/ProcessManager.h"

#include "Core/Utils/DeviceCreation.h"
#include "Core/Utils/FramebufferUtils.h"
#include "Core/Utils/SwapchainUtils.h"

VK_NAMESPACE::Device::Device(const DeviceCreateInfo& info)
	: mHandle(), mDeviceInfo(info), mSwapchain()
{
	DoSanityChecks();

	auto Instance = mDeviceInfo.PhysicalDevice.ParentInstance;

	mHandle = Core::Ref(Core::Utils::CreateDevice(mDeviceInfo),
		[Instance](vk::Device device) { device.destroy(); });

	// Retrieving the queues from the device here...

	auto QueueCapabilities = info.PhysicalDevice.GetQueueIndexMap(info.DeviceCapabilities);
	auto [Found, Indices] = info.PhysicalDevice.GetQueueFamilyIndices(info.DeviceCapabilities);

	Core::QueueFamilyMap<std::pair<size_t, std::vector<Core::Ref<Core::Queue>>>> Queues;

	auto Device = mHandle;

	for (auto index : Indices)
	{
		auto& FamilyProps = info.PhysicalDevice.QueueProps[index];
		auto& [WorkerBegin, FamilyRef] = Queues[index];

		size_t Count = std::min(info.MaxQueueCount, FamilyProps.queueCount);
		WorkerBegin = std::max(1, static_cast<int>(0.2f * Count));

		for (size_t i = 0; i < Count; i++)
		{
			auto Fence = mHandle->createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
			Core::Queue Queue(mHandle->getQueue(index, static_cast<uint32_t>(i)),
				Fence, static_cast<uint32_t>(i), *mHandle);

			FamilyRef.emplace_back(Queue, [Device](Core::Queue queue)
				{ Device->destroyFence(queue.mFence); });
		}
	}

	// Creating a queue inspector...
	mQueueManager = std::shared_ptr<QueueManager>(
		new QueueManager(Queues, Indices, QueueCapabilities, 
			mDeviceInfo.PhysicalDevice.QueueProps, mHandle));

	mDescPoolBuilder = { mHandle };
	 
	// Creating the swapchain here...
	CreateSwapchain(info.SwapchainInfo);
}

VK_NAMESPACE::Core::Ref<vk::Semaphore> VK_NAMESPACE::Device::CreateSemaphore()
{
	auto Device = mHandle;
	return Core::CreateRef(Core::Utils::CreateSemaphore(*mHandle), 
		[Device](vk::Semaphore semaphore) { Device->destroySemaphore(semaphore); });
}

VK_NAMESPACE::Core::Ref<vk::Fence> VK_NAMESPACE::Device::CreateFence(bool Signaled)
{
	auto Device = mHandle;

	return Core::CreateRef(Core::Utils::CreateFence(*mHandle, Signaled),
		[Device](vk::Fence fence) { Device->destroyFence(fence); });
}

void VK_NAMESPACE::Device::ResetFence(Core::Ref<vk::Fence> Fence)
{
	mHandle->resetFences(*Fence);
}

void VK_NAMESPACE::Device::WaitForFence(Core::Ref<vk::Fence> fence, uint64_t timeout /*= UINT64_MAX*/)
{
	vk::Result Temp = mHandle->waitForFences(*fence, VK_TRUE, timeout);
}

VK_NAMESPACE::CommandPoolManager VK_NAMESPACE::Device::CreateCommandPoolManager(
	bool IsTransient /*= false*/, bool IsProtected /*= false*/) const
{
	vk::CommandPoolCreateFlags CreationFlags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	if (IsTransient)
		CreationFlags |= vk::CommandPoolCreateFlagBits::eTransient;

	if (IsProtected)
		CreationFlags |= vk::CommandPoolCreateFlagBits::eProtected;

	return { mHandle, mQueueManager->GetQueueFamilyIndices(), CreationFlags };
}

VK_NAMESPACE::PipelineBuilder VK_NAMESPACE::Device::MakePipelineBuilder() const
{
	PipelineBuilder builder{};
	builder.mDevice = mHandle;
	builder.mMemoryManager = MakeMemoryResourceManager();

	PipelineBuilderData data{};
	vk::PipelineCacheCreateInfo cacheInfo{};
	data.Cache = mHandle->createPipelineCache(cacheInfo);

	auto Device = mHandle;

	builder.mData = Core::CreateRef(data, [Device](const PipelineBuilderData& builderData)
	{
		Device->destroyPipelineCache(builderData.Cache);
	});

	builder.mDevice = mHandle;
	builder.mDescPoolManager = FetchDescriptorPoolManager();

	return builder;
}

VK_NAMESPACE::DescriptorPoolManager VK_NAMESPACE::Device::FetchDescriptorPoolManager() const
{
	DescriptorPoolManager manager;
	manager.mPoolBuilder = mDescPoolBuilder;

	return manager;
}

VK_NAMESPACE::MemoryResourceManager VK_NAMESPACE::Device::MakeMemoryResourceManager() const
{
	MemoryResourceManager manager{};
	manager.mDevice = mHandle;
	manager.mPhysicalDevice = mDeviceInfo.PhysicalDevice;
	manager.mBufferProcessHandler = MakeProcessManager(true);
	manager.mImageProcessHandler = MakeProcessManager(true);
	manager.mQueueManager = mQueueManager;

	return manager;
}

VK_NAMESPACE::RenderContextBuilder VK_NAMESPACE::Device::FetchRenderContextBuilder(vk::PipelineBindPoint bindPoint)
{
	RenderContextBuilder Context;
	Context.mDevice = mHandle;
	Context.mPhysicalDevice = mDeviceInfo.PhysicalDevice.Handle;
	Context.mProcessHandler = MakeProcessManager(true, false);
	Context.mBindPoint = bindPoint;

	return Context;
}

VK_NAMESPACE::ProcessManager VK_NAMESPACE::Device::MakeProcessManager(
	bool IsTransient /*= false*/, bool IsProtected /*= false*/) const
{
	return { CreateCommandPoolManager(IsTransient), mQueueManager };
}

void VK_NAMESPACE::Device::InvalidateSwapchain(const SwapchainInvalidateInfo& newInfo)
{
	SwapchainInfo swapchainInfo{};
	swapchainInfo.Width = newInfo.Width;
	swapchainInfo.Height = newInfo.Height;
	swapchainInfo.PresentMode = newInfo.PresentMode;
	swapchainInfo.Surface = mSwapchain->mInfo.Surface;

	CreateSwapchain(swapchainInfo);
}

void VK_NAMESPACE::Device::CreateSwapchain(const SwapchainInfo& info)
{
	mSwapchain = std::shared_ptr<Swapchain>(new Swapchain(mHandle, mDeviceInfo.PhysicalDevice, 
		FetchRenderContextBuilder(vk::PipelineBindPoint::eGraphics), info, MakeProcessManager()));
}

void VK_NAMESPACE::Device::DoSanityChecks()
{
	_STL_ASSERT(*mDeviceInfo.SwapchainInfo.Surface,
		"Device requires a Vulkan Surface to create swapchain\n");

	auto& extensions = mDeviceInfo.Extensions;
	auto found = std::find(extensions.begin(), extensions.end(), VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	if (found == extensions.end())
		extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

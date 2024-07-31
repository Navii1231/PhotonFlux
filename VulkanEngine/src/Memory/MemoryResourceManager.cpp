#include "Memory/MemoryResourceManager.h"

VK_NAMESPACE::Image VK_NAMESPACE::MemoryResourceManager::CreateImage(const ImageCreateInfo& info) const
{
	auto Device = mDevice;

	Core::ImageConfig config{};
	config.Extent = info.Extent;
	config.Format = info.Format;
	config.LogicalDevice = *Device;
	config.PhysicalDevice = mPhysicalDevice.Handle;
	config.ResourceOwner = mQueueManager->FindOptimalQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
	config.Tiling = info.Tiling;
	config.Type = info.Type;
	config.MemProps = info.MemProps;
	config.Usage = info.Usage | vk::ImageUsageFlagBits::eTransferSrc
		| vk::ImageUsageFlagBits::eTransferDst;

	Core::Image Handles = Core::Utils::CreateImage(config);

	Core::ImageChunk chunk{};
	chunk.Device = Device;

	chunk.ImageHandles = Core::CreateRef(Handles, [Device](const Core::Image handles)
	{
		Device->freeMemory(handles.Memory);
		Device->destroyImage(handles.Handle);
		Device->destroyImageView(handles.IdentityView);
	});

	Image image(chunk);
	image.mProcessHandler = mImageProcessHandler;

	return image;
}

VK_NAMESPACE::SamplerCache VK_NAMESPACE::MemoryResourceManager::CreateSamplerCache() const
{
	SamplerCache cache = std::make_shared<BasicSamplerCachePayload<SamplerInfo>>();
	return cache;
}

VK_NAMESPACE::VK_CORE::Ref<vk::Sampler> VK_NAMESPACE::MemoryResourceManager::CreateSampler(
	const SamplerInfo& samplerInfo, SamplerCache cache /*= {} */) const
{
	if (cache)
	{
		auto found = cache->find(samplerInfo);

		if (found != cache->end())
			return found->second;
	}

	vk::SamplerCreateInfo createInfo{};
	createInfo.setAddressModeU(samplerInfo.AddressModeU);
	createInfo.setAddressModeV(samplerInfo.AddressModeV);
	createInfo.setAddressModeW(samplerInfo.AddressModeW);
	createInfo.setAnisotropyEnable(samplerInfo.AnisotropyEnable);
	createInfo.setBorderColor(samplerInfo.BorderColor);
	createInfo.setMagFilter(samplerInfo.MagFilter);
	createInfo.setMinFilter(samplerInfo.MinFilter);
	createInfo.setMaxAnisotropy(samplerInfo.MaxAnisotropy);
	createInfo.setMaxLod(samplerInfo.MaxLod);
	createInfo.setMinLod(samplerInfo.MinLod);
	createInfo.setMipLodBias(0.0f);
	createInfo.setUnnormalizedCoordinates(!samplerInfo.NormalisedCoordinates);

	auto handle = mDevice->createSampler(createInfo);
	auto Device = mDevice;

	auto Result = Core::CreateRef(handle, [Device](vk::Sampler sampler)
	{ Device->destroySampler(sampler); });

	if (cache)
		(*cache)[samplerInfo] = Result;

	return Result;
}

#pragma once
#include "../Core/AqCore.h"
#include "stb/stb_image.h"

AQUA_BEGIN

class StbImage
{
public:
	void SetResourcePool(vkEngine::ResourcePool manager) { mResourcePool = manager; }

	inline vkEngine::Image CreateImageBuffer(const std::string& filepath);

	// No. of channels are fixed to four!
	void SetFormat(vk::Format val) { mFormat = val; }
	void SetTiling(vk::ImageTiling val) { mTiling = val; }

	void SetUsage(vk::ImageUsageFlags val) { mUsage = val; }
	void SetMemProps(vk::MemoryPropertyFlags val) { mMemProps = val; }

private:
	vkEngine::ResourcePool mResourcePool;

	// Image metadata...
	vk::Format mFormat = vk::Format::eR8G8B8A8Unorm;
	vk::ImageTiling mTiling = vk::ImageTiling::eOptimal;

	vk::ImageUsageFlags mUsage = vk::ImageUsageFlagBits::eColorAttachment 
		| vk::ImageUsageFlagBits::eSampled;
	vk::MemoryPropertyFlags mMemProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
};

inline vkEngine::Image StbImage::CreateImageBuffer(const std::string& filepath)
{
	int width = 0, height = 0, channels = 0;

	stbi_uc* result = stbi_load(filepath.c_str(), &width, &height, &channels, 4);

	if (!result)
		return vkEngine::Image{};

	vkEngine::Buffer<int> image_buffer = mResourcePool.CreateBuffer<int>(
		vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostCoherent);

	image_buffer.Resize(width * height * 4);

	int* buf = image_buffer.MapMemory(image_buffer.GetSize());
	std::memcpy(buf, result, image_buffer.GetSize());
	image_buffer.UnmapMemory();

	stbi_image_free(result);

	vkEngine::ImageCreateInfo createInfo{};
	createInfo.Format = mFormat;
	createInfo.MemProps = mMemProps;
	createInfo.Tiling = mTiling;
	createInfo.Usage = mUsage;

	createInfo.Extent = vk::Extent3D(width, height, 1);

	vkEngine::Image image = mResourcePool.CreateImage(createInfo);
	
	image.TransitionLayout(vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eComputeShader);
	image.CopyBufferData(image_buffer);

	return image;
}

AQUA_END

#pragma once
#include "../../Core/AqCore.h"
#include "G_BufferPipeline.h"

AQUA_BEGIN

/*
* -Front end of the shader
* -- all the buffers, uniforms and utils stuff
* 
* -User shader
* -- the shader itself
* 
* - Back end
* -- where the evaluation occurs
*/

struct ShaderEvaluatorCreateInfo
{
	std::string ShaderCode;
};

class ShaderEvaluator : public vkEngine::ComputePipelineContext
{
public:
	ShaderEvaluator() = default;

	template <typename T>
	void InsertBufferResource(const vkEngine::DescriptorLocation& location,
		const vkEngine::Buffer<T>& buffer);

	void InsertImageResource(const vkEngine::DescriptorLocation& location,
		const vkEngine::Image& image);

	inline void RemoveResource(const vkEngine::DescriptorLocation& location);

	void Prepare(const ShaderEvaluatorCreateInfo& createInfo, float intersectionTolerence);

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override;

private:
	RendererGeometry mGeometry;

	// Other sets...
	std::unordered_map<vkEngine::DescriptorLocation, vkEngine::Core::BufferChunk> mBuffers;
	std::unordered_map<vkEngine::DescriptorLocation, vkEngine::Image> mImages;

private:
	void UpdateImage(const vkEngine::DescriptorLocation& location, 
		vkEngine::DescriptorWriter& writer, vkEngine::Image image);
};

template <typename Buf>
bool UpdateIfExists(const vkEngine::DescSetLayoutBindingMap& setBindings,
	uint32_t setNo, uint32_t bindingNo, Buf buf, vkEngine::DescriptorWriter& writer)
{
	vkEngine::StorageBufferWriteInfo sInfo{};

	if (setBindings.find(setNo) == setBindings.end())
		return false;

	const auto& vec = setBindings.at(setNo);

	bool Exists = false;

	for (const auto& val : vec)
	{
		if (val.binding == bindingNo)
		{
			Exists = true;
			break;
		}
	}

	if (Exists)
	{
		sInfo.Buffer = buf.GetNativeHandles().Handle;
		writer.Update({ setNo, bindingNo, 0 }, sInfo);

		return true;
	}

	return false;
};

void ShaderEvaluator::InsertImageResource(
	const vkEngine::DescriptorLocation& location, const vkEngine::Image& image)
{
	RemoveResource(location);

	mImages[location] = image;
}

inline void ShaderEvaluator::RemoveResource(const vkEngine::DescriptorLocation& location)
{
	if (mBuffers.find(location) != mBuffers.end())
		mBuffers.erase(location);

	if (mImages.find(location) != mImages.end())
		mImages.erase(location);
}

void ShaderEvaluator::Prepare(const ShaderEvaluatorCreateInfo& createInfo, float intersectionTolerence)
{

}

void ShaderEvaluator::UpdateDescriptors(vkEngine::DescriptorWriter& writer)
{
	auto setLayoutBindingMap = GetPipelineLayoutInfo().first;

	UpdateImage({ 0, 0, 0 }, writer, mGeometry.Depth);
	UpdateImage({ 0, 1, 0 }, writer, mGeometry.Depth);
	UpdateImage({ 0, 1, 0 }, writer, mGeometry.Depth);
	UpdateImage({ 0, 1, 0 }, writer, mGeometry.Depth);

	for (const auto& [location, image] : mImages)
	{
		UpdateImage(location, writer, image);
	}

	for (const auto& [location, buffer] : mBuffers)
	{
		if (buffer.BufferHandles->Config.Usage & vk::BufferUsageFlagBits::eUniformBuffer)
		{
			vkEngine::UniformBufferWriteInfo uniformInfo{};
			uniformInfo.Buffer = buffer.BufferHandles->Handle;

			writer.Update(location, uniformInfo);
		}

		if (buffer.BufferHandles->Config.Usage & vk::BufferUsageFlagBits::eStorageBuffer)
		{
			vkEngine::StorageBufferWriteInfo storageInfo{};
			storageInfo.Buffer = buffer.BufferHandles->Handle;

			writer.Update(location, storageInfo);
		}
	}
}

void ShaderEvaluator::UpdateImage(const vkEngine::DescriptorLocation& location, 
	vkEngine::DescriptorWriter& writer, vkEngine::Image image)
{
	vkEngine::SampledImageWriteInfo sampledImage{};
	sampledImage.ImageView = image.GetIdentityImageView();
	sampledImage.ImageLayout = vk::ImageLayout::eGeneral;
	sampledImage.Sampler = *image.GetSampler();

	writer.Update(location, sampledImage);
}

template <typename T>
void AQUA_NAMESPACE::ShaderEvaluator::InsertBufferResource(
		const vkEngine::DescriptorLocation& location, const vkEngine::Buffer<T>& buffer)
{
	RemoveResource(location);

	mBuffers[location] = buffer.GetBufferChunk();
}

AQUA_END

#include "Core/Aqpch.h"
#include "Wavefront/MaterialPipeline.h"

#define USE_DEFAULT_SHADER 0

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


void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::MaterialPipeline::InsertImageResource(
	const vkEngine::DescriptorLocation& location, const vkEngine::Image& image)
{
	RemoveResource(location);

	mHandle.mImages[location] = image;
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::MaterialPipeline::RemoveResource(
	const vkEngine::DescriptorLocation& location)
{
	if (mHandle.mBuffers.find(location) != mHandle.mBuffers.end())
		mHandle.mBuffers.erase(location);

	if (mHandle.mImages.find(location) != mHandle.mImages.end())
		mHandle.mImages.erase(location);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::MaterialPipeline::UpdateDescriptors()
{
	vkEngine::DescriptorWriter& writer = this->GetDescriptorWriter();

	auto setLayoutBindingMap = GetShader().GetPipelineLayoutInfo().first;

	vkEngine::StorageBufferWriteInfo storageInfo{};

	storageInfo.Buffer = mHandle.mRays.GetNativeHandles().Handle;
	writer.Update({ 0, 0, 0 }, storageInfo);

	storageInfo.Buffer = mHandle.mRayInfos.GetNativeHandles().Handle;
	writer.Update({ 0, 1, 0 }, storageInfo);

	storageInfo.Buffer = mHandle.mCollisionInfos.GetNativeHandles().Handle;
	writer.Update({ 0, 2, 0 }, storageInfo);

	storageInfo.Buffer = mHandle.mLightInfos.GetNativeHandles().Handle;
	writer.Update({ 0, 7, 0 }, storageInfo);

	storageInfo.Buffer = mHandle.mLightProps.GetNativeHandles().Handle;
	writer.Update({ 0, 8, 0 }, storageInfo);

	// Updating the shader constants...
	vkEngine::UniformBufferWriteInfo uniformInfo{};
	uniformInfo.Buffer = mHandle.mShaderData.GetNativeHandles().Handle;
	writer.Update({ 1, 0, 0 }, uniformInfo);

	// Layout binding map...

	UpdateIfExists(setLayoutBindingMap, 0, 3, mHandle.mGeometry.Vertices, writer);
	UpdateIfExists(setLayoutBindingMap, 0, 4, mHandle.mGeometry.Normals, writer);
	UpdateIfExists(setLayoutBindingMap, 0, 5, mHandle.mGeometry.TexCoords, writer);
	UpdateIfExists(setLayoutBindingMap, 0, 6, mHandle.mGeometry.Faces, writer);

	for (const auto& [location, image] : mHandle.mImages)
	{
		vkEngine::SampledImageWriteInfo sampledImage{};
		sampledImage.ImageView = image.GetIdentityImageView();
		sampledImage.ImageLayout = vk::ImageLayout::eGeneral;
		sampledImage.Sampler = *image.GetSampler();

		writer.Update(location, sampledImage);
	}

	for (const auto& [location, buffer] : mHandle.mBuffers)
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

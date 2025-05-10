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


void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::MaterialPipelineContext::InsertImageResource(
	const vkEngine::DescriptorLocation& location, const vkEngine::Image& image)
{
	RemoveResource(location);

	mImages[location] = image;
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::MaterialPipelineContext::RemoveResource(
	const vkEngine::DescriptorLocation& location)
{
	if (mBuffers.find(location) != mBuffers.end())
		mBuffers.erase(location);

	if (mImages.find(location) != mImages.end())
		mImages.erase(location);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::MaterialPipelineContext::UpdateDescriptors(
	vkEngine::DescriptorWriter& writer)
{
	auto setLayoutBindingMap = GetPipelineLayoutInfo().first;

	vkEngine::StorageBufferWriteInfo storageInfo{};

	storageInfo.Buffer = mRays.GetNativeHandles().Handle;
	writer.Update({ 0, 0, 0 }, storageInfo);

	storageInfo.Buffer = mRayInfos.GetNativeHandles().Handle;
	writer.Update({ 0, 1, 0 }, storageInfo);

	storageInfo.Buffer = mCollisionInfos.GetNativeHandles().Handle;
	writer.Update({ 0, 2, 0 }, storageInfo);

	storageInfo.Buffer = mLightInfos.GetNativeHandles().Handle;
	writer.Update({ 0, 7, 0 }, storageInfo);

	storageInfo.Buffer = mLightProps.GetNativeHandles().Handle;
	writer.Update({ 0, 8, 0 }, storageInfo);

	// Layout binding map...

	UpdateIfExists(setLayoutBindingMap, 0, 3, mGeometry.Vertices, writer);
	UpdateIfExists(setLayoutBindingMap, 0, 4, mGeometry.Normals, writer);
	UpdateIfExists(setLayoutBindingMap, 0, 5, mGeometry.TexCoords, writer);
	UpdateIfExists(setLayoutBindingMap, 0, 6, mGeometry.Faces, writer);

	for (const auto& [location, image] : mImages)
	{
		vkEngine::SampledImageWriteInfo sampledImage{};
		sampledImage.ImageView = image.GetIdentityImageView();
		sampledImage.ImageLayout = vk::ImageLayout::eGeneral;
		sampledImage.Sampler = *image.GetSampler();

		writer.Update(location, sampledImage);
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

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::MaterialPipelineContext::Prepare(
	const MaterialCreateInfo& createInfo, float intersectionTolerence)
{
	mCreateInfo = createInfo;

	/*
	* user defined macro definitions...
	* SHADER_TOLERENCE = 0.001, POWER_HEURISTIC_EXP = 2.0,
	* EMPTY_MATERIAL_ID = -1, SKYBOX_MATERIAL_ID = -2, LIGHT_MATERIAL_ID = -3,
	*/

	AddMacro("WORKGROUP_SIZE", std::to_string(createInfo.WorkGroupSize));
	AddMacro("SHADING_TOLERENCE", std::to_string(static_cast<double>(createInfo.ShadingTolerence)));
	AddMacro("TOLERENCE", std::to_string(static_cast<double>(intersectionTolerence)));
	AddMacro("EPSILON", std::to_string(static_cast<double>(FLT_EPSILON)));
	AddMacro("POWER_HEURISTICS_EXP", std::to_string(static_cast<double>(createInfo.PowerHeuristics)));
	AddMacro("EMPTY_MATERIAL_ID", std::to_string(static_cast<int>(-1)));
	AddMacro("SKYBOX_MATERIAL_ID", std::to_string(static_cast<int>(-2)));
	AddMacro("LIGHT_MATERIAL_ID", std::to_string(static_cast<int>(-3)));

	vkEngine::ShaderInput input{};
	input.SrcCode = createInfo.ShaderCode;

	input.Stage = vk::ShaderStageFlagBits::eCompute;
	input.OptimizationFlag = vkEngine::OptimizerFlag::eNone;
	input.OptimizationFlag = vkEngine::OptimizerFlag::eO3;

	auto Errors = CompileShaders({ input });

	CompileErrorChecker checker("Logging/ShaderFails/Shader.glsl");

	auto ErrorInfos = checker.GetErrors(Errors);
	checker.AssertOnError(ErrorInfos);
}

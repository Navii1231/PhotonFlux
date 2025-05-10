#include "Wavefront/WavefrontWorkflow.h"

AQUA_BEGIN

std::string GetShaderDirectory();

AQUA_END

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::IntersectionPipelineContext::Prepare(uint32_t workGroupSize, float Tolerence)
{
	AddMacro("WORKGROUP_SIZE", std::to_string(workGroupSize));
	AddMacro("TOLERENCE", std::to_string(Tolerence));
	AddMacro("MAX_DIS", std::to_string(FLT_MAX));
	AddMacro("FLT_MAX", std::to_string(FLT_MAX));

	vkEngine::ShaderInput input{};
	input.FilePath = GetShaderDirectory() + "Wavefront/Intersection.glsl";
	input.Stage = vk::ShaderStageFlagBits::eCompute;
	input.OptimizationFlag = vkEngine::OptimizerFlag::eO3;
	//input.OptimizationFlag = vkEngine::OptimizerFlag::eNone;

	auto Errors = CompileShaders({ input });

	CompileErrorChecker checker("../vkEngineTester/Logging/ShaderFails/Shader.glsl");

	auto ErrorInfos = checker.GetErrors(Errors);
	checker.AssertOnError(ErrorInfos);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::IntersectionPipelineContext::
	UpdateDescriptors(vkEngine::DescriptorWriter& writer)
{
	vkEngine::StorageBufferWriteInfo rayBufferWrite{};
	rayBufferWrite.Buffer = mRays.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, rayBufferWrite);

	vkEngine::StorageBufferWriteInfo collisionInfo{};
	collisionInfo.Buffer = mCollisionInfos.GetNativeHandles().Handle;

	writer.Update({ 0, 2, 0 }, collisionInfo);

	vkEngine::UniformBufferWriteInfo sceneInfo{};
	sceneInfo.Buffer = mSceneInfo.GetNativeHandles().Handle;

	writer.Update({ 1, 9, 0 }, sceneInfo);

	UpdateGeometryBuffers(writer);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::IntersectionPipelineContext::UpdateGeometryBuffers(
	vkEngine::DescriptorWriter& writer)
{
/*
* Descriptor layout in shaders pipelines for GeometryBuffers and Geometry references
* 
	layout(std430, set = 1, binding = 0) readonly buffer VertexBuffer
	{
		vec3 sPositions[];
	};

	layout(std430, set = 1, binding = 1) readonly buffer NormalBuffer
	{
		vec3 sNormals[];
	};

	layout(std430, set = 1, binding = 2) readonly buffer TexCoordBuffer
	{
		vec2 sTexCoords[];
	};

	layout(std430, set = 1, binding = 3) readonly buffer IndexBuffer
	{
		Face sIndices[];
	};

	layout(std430, set = 1, binding = 4) readonly buffer NodeBuffer
	{
		Node sNodes[];
	};

	layout(std430, set = 1, binding = 6) readonly buffer LightPropsBuffer
	{
		LightProperties sLightPropsInfos[];
	};
	
	layout(std430, set = 1, binding = 7) readonly buffer MeshInfoBuffer
	{
		MeshInfo sMeshInfos[];
	};
	
	layout(std430, set = 1, binding = 8) readonly buffer LightInfoBuffer
	{
		LightInfo sLightInfos[];
	};

*/

	vkEngine::StorageBufferWriteInfo storageInfo{};
	storageInfo.Buffer = mGeometryBuffers.Vertices.GetNativeHandles().Handle;
	writer.Update({ 1, 0, 0 }, storageInfo);

#if 0
	storageInfo.Buffer = mGeometryBuffers.Normals.GetNativeHandles().Handle;
	writer.Update({ 1, 1, 0 }, storageInfo);

	storageInfo.Buffer = mGeometryBuffers.TexCoords.GetNativeHandles().Handle;
	writer.Update({ 1, 2, 0 }, storageInfo);

#endif

	storageInfo.Buffer = mGeometryBuffers.Faces.GetNativeHandles().Handle;
	writer.Update({ 1, 3, 0 }, storageInfo);

	storageInfo.Buffer = mGeometryBuffers.Nodes.GetNativeHandles().Handle;
	writer.Update({ 1, 4, 0 }, storageInfo);

	// Meta data about vertices and light sources
	storageInfo.Buffer = mLightProps.GetNativeHandles().Handle;
	//writer.Update({ 1, 6, 0 }, storageInfo);

	storageInfo.Buffer = mMeshInfos.GetNativeHandles().Handle;
	writer.Update({ 1, 7, 0 }, storageInfo);

	storageInfo.Buffer = mLightInfos.GetNativeHandles().Handle;
	writer.Update({ 1, 8, 0 }, storageInfo);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::RaySortEpilogue::Prepare(uint32_t workGroupSize, RaySortEvent sortEvent)
{
	mSortingEvent = sortEvent;

	AddMacro("WORKGROUP_SIZE", std::to_string(workGroupSize));

	vkEngine::ShaderInput input{};

	input.FilePath = GetFilePath(sortEvent);
	input.Stage = vk::ShaderStageFlagBits::eCompute;
	input.OptimizationFlag = vkEngine::OptimizerFlag::eO3;

	auto Errors = CompileShaders({ input });

	CompileErrorChecker checker("../vkEngineTester/Logging/ShaderFails/Shader.glsl");

	auto ErrorInfos = checker.GetErrors(Errors);
	checker.AssertOnError(ErrorInfos);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::RaySortEpilogue::UpdateDescriptors(vkEngine::DescriptorWriter& writer)
{

	if (mSortingEvent == RaySortEvent::eFinish)
	{
		vkEngine::StorageBufferWriteInfo collisionInfo{};
		collisionInfo.Buffer = mCollisionInfos.GetNativeHandles().Handle;

		writer.Update({ 0, 2, 0 }, collisionInfo);

		collisionInfo.Buffer = mRaysInfos.GetNativeHandles().Handle;
		writer.Update({ 0, 4, 0 }, collisionInfo);
	}

	vkEngine::StorageBufferWriteInfo rayBufferWrite{};
	rayBufferWrite.Buffer = mRays.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, rayBufferWrite);

	vkEngine::StorageBufferWriteInfo rayRefs{};
	rayRefs.Buffer = mRayRefs.GetNativeHandles().Handle;

	writer.Update({ 0, 3, 0 }, rayRefs);
}

std::string AQUA_NAMESPACE::PH_FLUX_NAMESPACE::RaySortEpilogue::GetFilePath(RaySortEvent sortEvent)
{
	switch (sortEvent)
	{
		case RaySortEvent::ePrepare:
			return GetShaderDirectory() + "Wavefront/PrepareRaySort.glsl";
		case RaySortEvent::eFinish:
			return GetShaderDirectory() + "Wavefront/FinishRaySort.glsl";
		default:
			break;
	}

	return "[Invalid Path]";
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::RayRefCounterContext::Prepare(uint32_t workGroupSize)
{
	AddMacro("WORKGROUP_SIZE", std::to_string(workGroupSize));
	AddMacro("PRIMITIVE_TYPE", "uint");

	vkEngine::ShaderInput input{};

	input.FilePath = GetShaderDirectory() + "Utils/CountElements.glsl";
	input.Stage = vk::ShaderStageFlagBits::eCompute;
	input.OptimizationFlag = vkEngine::OptimizerFlag::eO3;

	auto Errors = CompileShaders({ input });

	CompileErrorChecker checker("../vkEngineTester/Logging/ShaderFails/Shader.glsl");

	auto ErrorInfos = checker.GetErrors(Errors);
	checker.AssertOnError(ErrorInfos);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::RayRefCounterContext::UpdateDescriptors(vkEngine::DescriptorWriter& writer)
{
	vkEngine::StorageBufferWriteInfo rayRefs{};
	rayRefs.Buffer = mRayRefs.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, rayRefs);

	vkEngine::StorageBufferWriteInfo counts{};
	counts.Buffer = mRefCounts.GetNativeHandles().Handle;

	writer.Update({ 0, 1, 0 }, counts);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::PrefixSumContext::Prepare(uint32_t workGroupSize)
{
	AddMacro("WORKGROUP_SIZE", std::to_string(workGroupSize));

	vkEngine::ShaderInput input{};

	input.FilePath = GetShaderDirectory() + "Utils/PrefixSum.glsl";
	input.Stage = vk::ShaderStageFlagBits::eCompute;
	input.OptimizationFlag = vkEngine::OptimizerFlag::eO3;

	auto Errors = CompileShaders({ input });

	CompileErrorChecker checker("Logging/ShaderFails/Shader.glsl");

	auto ErrorInfos = checker.GetErrors(Errors);
	checker.AssertOnError(ErrorInfos);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::PrefixSumContext::UpdateDescriptors(vkEngine::DescriptorWriter& writer)
{
	vkEngine::StorageBufferWriteInfo counts{};
	counts.Buffer = mRefCounts.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, counts);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::LuminanceMeanContext::Prepare(uint32_t workGroupSize)
{
	AddMacro("WORKGROUP_SIZE", std::to_string(workGroupSize));

	vkEngine::ShaderInput input{};

	input.FilePath = GetShaderDirectory() + "Wavefront/LuminanceMean.glsl";
	input.Stage = vk::ShaderStageFlagBits::eCompute;
	input.OptimizationFlag = vkEngine::OptimizerFlag::eO3;
	//input.OptimizationFlag = vkEngine::OptimizerFlag::eNone;

	auto Errors = CompileShaders({ input });

	CompileErrorChecker checker("Logging/ShaderFails/Shader.glsl");

	auto ErrorInfos = checker.GetErrors(Errors);
	checker.AssertOnError(ErrorInfos);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::LuminanceMeanContext::UpdateDescriptors(vkEngine::DescriptorWriter& writer)
{
	vkEngine::StorageImageWriteInfo mean{};
	mean.ImageLayout = vk::ImageLayout::eGeneral;

	mean.ImageView = mPresentable .GetIdentityImageView();
	writer.Update({ 2, 0, 0 }, mean);

	mean.ImageView = mPixelVariance.GetIdentityImageView();
	//writer.Update({ 2, 2, 0 }, mean);

	mean.ImageView = mPixelMean.GetIdentityImageView();
	writer.Update({ 2, 1, 0 }, mean);

	vkEngine::StorageBufferWriteInfo rayInfos{};
	rayInfos.Buffer = mRays.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, rayInfos);

	rayInfos.Buffer = mRayInfos.GetNativeHandles().Handle;

	writer.Update({ 0, 4, 0 }, rayInfos);

	vkEngine::UniformBufferWriteInfo sceneInfo{};
	sceneInfo.Buffer = mSceneInfo.GetNativeHandles().Handle;

	writer.Update({ 1, 9, 0 }, sceneInfo);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::PostProcessImageContext::Prepare(const glm::ivec2& workGroupSize)
{
	AddMacro("WORKGROUP_SIZE_X", std::to_string(workGroupSize.x));
	AddMacro("WORKGROUP_SIZE_Y", std::to_string(workGroupSize.y));

	AddMacro("APPLY_TONE_MAP", std::to_string(static_cast<uint32_t>(PostProcessFlagBits::eToneMap)));

	AddMacro("APPLY_GAMMA_CORRECTION", 
		std::to_string(static_cast<uint32_t>(PostProcessFlagBits::eGammaCorrection)));

	AddMacro("APPLY_GAMMA_CORRECTION_INV", 
		std::to_string(static_cast<uint32_t>(PostProcessFlagBits::eGammaCorrectionInv)));

	vkEngine::ShaderInput input{};

	input.FilePath = GetShaderDirectory() + "Wavefront/PostProcessImage.glsl";
	input.Stage = vk::ShaderStageFlagBits::eCompute;
	input.OptimizationFlag = vkEngine::OptimizerFlag::eO3;
	input.OptimizationFlag = vkEngine::OptimizerFlag::eNone;

	auto Errors = CompileShaders({ input });

	CompileErrorChecker checker("../vkEngineTester/Logging/ShaderFails/Shader.glsl");

	auto ErrorInfos = checker.GetErrors(Errors);
	checker.AssertOnError(ErrorInfos);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::PostProcessImageContext::UpdateDescriptors(vkEngine::DescriptorWriter& writer)
{
	vkEngine::StorageImageWriteInfo mean{};
	mean.ImageLayout = vk::ImageLayout::eGeneral;

	mean.ImageView = mPresentable.GetIdentityImageView();
	writer.Update({ 0, 0, 0 }, mean);
}

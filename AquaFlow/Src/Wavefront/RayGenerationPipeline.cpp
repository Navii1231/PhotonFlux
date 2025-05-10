#include "Wavefront/RayGenerationPipeline.h"

AQUA_BEGIN

std::string GetShaderDirectory();

AQUA_END

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::RayGenerationPipelineContext::Prepare(uint32_t workGroupSize)
{
	AddMacro("WORKGROUP_SIZE", std::to_string(workGroupSize));

	vkEngine::ShaderInput input{};
	input.Stage = vk::ShaderStageFlagBits::eCompute;
	input.OptimizationFlag = vkEngine::OptimizerFlag::eO3;
	//input.OptimizationFlag = vkEngine::OptimizerFlag::eNone;
	input.FilePath = GetShaderDirectory() + "Wavefront/RayGeneration.comp";

	auto CompileErrors = CompileShaders({ input });

	for (size_t i = 0; i < CompileErrors.size(); i++)
	{
		if (CompileErrors[i].Type != vkEngine::ErrorType::eNone)
		{
			vkEngine::WriteFile("D:/Dev/VulkanEngine/vkEngineTester/Logging/ShaderFails/Shader.glsl"
				, CompileErrors[i].SrcCode);
			std::cout << CompileErrors[i].Info << std::endl;

			_STL_ASSERT(false, "Failed to compile the ray generation glsl code!");
		}
	}
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::RayGenerationPipelineContext::SetSceneInfo(const WavefrontSceneInfo& sceneInfo)
{
	mSceneInfo.Clear();
	mSceneInfo << sceneInfo;
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::RayGenerationPipelineContext::SetCamera(const PhysicalCamera& camera)
{
	mCamera.Clear();
	mCamera << camera;
	mCameraUpdated = false;
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::RayGenerationPipelineContext::UpdateDescriptors(
	vkEngine::DescriptorWriter& writer)
{
	vkEngine::StorageBufferWriteInfo bufferInfo{};
	bufferInfo.Buffer = mRays.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, bufferInfo);

	bufferInfo.Buffer = mRayInfos.GetNativeHandles().Handle;

	writer.Update({ 0, 4, 0 }, bufferInfo);

	vkEngine::UniformBufferWriteInfo cameraInfo{};
	cameraInfo.Buffer = mCamera.GetNativeHandles().Handle;

	writer.Update({ 0, 1, 0 }, cameraInfo);

	vkEngine::UniformBufferWriteInfo sceneInfo{};
	sceneInfo.Buffer = mSceneInfo.GetNativeHandles().Handle;

	writer.Update({ 1, 9, 0 }, sceneInfo);

	mCameraUpdated = true;
}

#pragma once
#include "RayTracingStructures.h"

AQUA_BEGIN
PH_BEGIN

class LocalRadixSortContext : public vkEngine::ComputePipelineContext
{
public:
	LocalRadixSortContext() {}

	void CompilerShader(uint32_t workGroupSize)
	{
		mWorkgroupSize = workGroupSize;

		/*** TODO: Shader source must be hard - coded in the class field! ***/

		std::string ShaderPath = "D:/Dev/VulkanEngine/vkEngineTester/Shaders/Utils/LocalRadixSort.glsl";

		std::string ShaderBackEnd;
		vkEngine::ReadFile(ShaderPath, ShaderBackEnd);

		/********************************************************************/

		// Combine the front-end implementation to the backend shader
		//std::string FullShader = "#version 440\n\n";
		//FullShader += mShaderFrontEnd + ShaderBackEnd;

		uint32_t Stride = 2;

		// TODO: Assert that the mWorkGroupSize must a multiple of two!

		AddMacro("WORKGROUP_SIZE", std::to_string(mWorkgroupSize));
		AddMacro("TREE_DEPTH", std::to_string((uint32_t) (glm::log2((float) mWorkgroupSize) + 0.5f)));
		AddMacro("STRIDE", std::to_string(Stride));

		// Preparing to compile
		vkEngine::ShaderInput Input;
		Input.SrcCode = ShaderBackEnd;
		Input.OptimizationFlag = vkEngine::OptimizerFlag::eO3;
		Input.Stage = vk::ShaderStageFlagBits::eCompute;

		// TODO: Setting up the necessary macros before compiling

		// Compile the shader
		auto CompilationResults = CompileShaders({ Input });

		if (CompilationResults[0].Type != vkEngine::ErrorType::eNone)
		{
			vkEngine::WriteFile("D:/Dev/VulkanEngine/vkEngineTester/Logging/ShaderFails/Shader.glsl"
				, CompilationResults[0].SrcCode);
			std::cout << CompilationResults[0].Info << std::endl;

			_STL_ASSERT(false, "Failed to compile the prefix sum code!");
		}
	}

	void UploadBuffer(const vkEngine::Buffer<uint32_t>& buffer)
	{
		mBuffer = buffer;
	}

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer)
	{
		vkEngine::StorageBufferWriteInfo prefixInfo{};
		prefixInfo.Buffer = mBuffer.GetNativeHandles().Handle;

		writer.Update({ 0, 0, 0 }, prefixInfo);
	}

	vkEngine::Buffer<uint32_t> GetPrefixBuffers() const { return mBuffer; }

private:
	vkEngine::Buffer<uint32_t> mBuffer;

	uint32_t mWorkgroupSize = 256;
};

using LocalRadixSortPipeline = vkEngine::ComputePipeline<LocalRadixSortContext>;

PH_END
AQUA_END

#pragma once
#include "RayTracingStructures.h"

AQUA_BEGIN
PH_BEGIN

extern std::string GetMergeSortCode();

template <typename CompType>
class MergeSorterPassContext : public vkEngine::ComputePipelineContext
{
public:
	using MyRefType = CompType;

	struct ArrayRef
	{
		CompType CompareElem;
		uint32_t ElemIdx;
	};

public:
	MergeSorterPassContext();

	void CompileShader(uint32_t workGroupSize);
	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer);

	void UploadBuffer(const vkEngine::Buffer<ArrayRef>& buffer)
		{ mBuffer = buffer; }

	vkEngine::Buffer<uint32_t> GetBufferData() const { return mBuffer; }
	std::string GetTypeIdString() const { return mTypeIdString; }

private:

	vkEngine::Buffer<ArrayRef> mBuffer;

	std::string mTypeIdString;
};

template <typename CompType>
using MergeSorterPass = vkEngine::ComputePipeline<MergeSorterPassContext<CompType>>;

template<typename CompType>
inline MergeSorterPassContext<CompType>::MergeSorterPassContext()
{
	auto AssignTypeIdName = [this](size_t TypeID, const std::string& AssignedName)
	{
		if (typeid(CompType).hash_code() == TypeID)
			mTypeIdString = AssignedName;
	};

	AssignTypeIdName(typeid(uint32_t).hash_code(), "uint");
	AssignTypeIdName(typeid(int32_t).hash_code(), "int");
	AssignTypeIdName(typeid(float).hash_code(), "float");

	if (mTypeIdString.empty())
		mTypeIdString = "InvalidType";
}

template<typename CompType>
inline void MergeSorterPassContext<CompType>::CompileShader(uint32_t workGroupSize)
{
	// Setting up the necessary macros before compiling shader
	AddMacro("WORKGROUP_SIZE", std::to_string(workGroupSize));
	AddMacro("PRIMITIVE_TYPE", mTypeIdString);

	// Preparing to compile
	vkEngine::ShaderInput Input;
	Input.SrcCode = GetMergeSortCode();
	Input.OptimizationFlag = vkEngine::OptimizerFlag::eO3;
	Input.Stage = vk::ShaderStageFlagBits::eCompute;

	// Compile the shader
	auto CompilationResults = CompileShaders({ Input });

	if (CompilationResults[0].Type != vkEngine::ErrorType::eNone)
	{
		vkEngine::WriteFile("D:/Dev/VulkanEngine/vkEngineTester/Logging/ShaderFails/Shader.glsl"
			, CompilationResults[0].SrcCode);
		std::cout << CompilationResults[0].Info << std::endl;

		_STL_ASSERT(false, "Failed to compile the merge sort glsl code!");
	}
}

template<typename CompType>
inline void MergeSorterPassContext<CompType>::UpdateDescriptors(vkEngine::DescriptorWriter& writer)
{
	vkEngine::StorageBufferWriteInfo bufferInfo{};
	bufferInfo.Buffer = mBuffer.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, bufferInfo);
}

PH_END
AQUA_END

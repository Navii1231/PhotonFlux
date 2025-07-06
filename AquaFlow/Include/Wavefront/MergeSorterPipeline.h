#pragma once
#include "RayTracingStructures.h"
#include "Utils/CompilerErrorChecker.h"

AQUA_BEGIN
PH_BEGIN

extern std::string GetMergeSortCode();

// TODO: change required

template <typename CompType>
class MergeSorterPass : public vkEngine::ComputePipeline
{
public:
	using MyRefType = CompType;

	struct ArrayRef
	{
		CompType CompareElem;
		uint32_t ElemIdx;
	};

public:
	MergeSorterPass() = default;
	MergeSorterPass(uint32_t workGroupSize);

	virtual void UpdateDescriptors() override;

	void SetBuffer(const vkEngine::Buffer<ArrayRef>& buffer)
		{ mBuffer = buffer; }

	vkEngine::Buffer<uint32_t> GetBufferData() const { return mBuffer; }
	std::string GetTypeIdString() const { return mTypeIdString; }

private:

	vkEngine::Buffer<ArrayRef> mBuffer;
	std::string mTypeIdString;

private:
	// Helper method

	void CompileShader(uint32_t workGroupSize);
};

template<typename CompType>
MergeSorterPass<CompType>::MergeSorterPass(uint32_t workGroupSize)
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

	vkEngine::PShader shader{};

	CompileShader(workGroupSize);
}

template<typename CompType>
inline void MergeSorterPass<CompType>::CompileShader(uint32_t workGroupSize)
{
	vkEngine::PShader shader;

	// Setting up the necessary macros before compiling shader
	shader.AddMacro("WORKGROUP_SIZE", std::to_string(workGroupSize));
	shader.AddMacro("PRIMITIVE_TYPE", mTypeIdString);

	shader.SetShader("eCompute", GetMergeSortCode(), vkEngine::OptimizerFlag::eO3);

	// Compile the shader
	auto Errors = shader.CompileShaders();

	CompileErrorChecker checker("../vkEngineTester/Logging/ShaderFails/Shader.glsl");
	auto ErrorInfos = checker.GetErrors(Errors);

	this->SetShader(shader);
}

template<typename CompType>
inline void MergeSorterPass<CompType>::UpdateDescriptors()
{
	vkEngine::DescriptorWriter& writer = this->GetDescriptorWriter();

	vkEngine::StorageBufferWriteInfo bufferInfo{};
	bufferInfo.Buffer = mBuffer.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, bufferInfo);
}

PH_END
AQUA_END

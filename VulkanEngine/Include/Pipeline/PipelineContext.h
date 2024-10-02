#pragma once
#include "PipelineConfig.h"
#include "../Descriptors/DescriptorWriter.h"

VK_BEGIN

enum class PipelineType
{
	eNone               = 1,
	eGraphics           = 2,
	eCompute            = 3,
	eRayTracing         = 4
};

class PipelineContext
{
public:
	PipelineContext(PipelineType type)
		: mType(type), mEnv(mConfig) {}

	// User defined stuff...

	void SetCompilerConfig(const CompilerConfig& config)
	{ mConfig = config; }

	void AddMacro(const std::string& macro, const std::string& definition)
	{ mEnv.AddMacro(macro, definition); }

	void RemoveMacro(const std::string& macro)
	{ mEnv.RemoveMacro(macro); }

	// user dependent stuff...
	virtual void UpdateDescriptors(DescriptorWriter& writer) = 0;

	// Compile shaders and store their result
	inline std::vector<CompileError> CompileShaders(const std::vector<ShaderInput>& ShaderSrcCode);

	const PushConstantSubrangeInfos& GetPushConstantSubranges() const { return mPushConstantSubranges; }

	// Non-virtual getters
	std::pair<DescSetLayoutBindingMap, std::vector<vk::PushConstantRange>>
		GetPipelineLayoutInfo() const { return { mPipelineSetLayoutInfo, mPushConstantRanges }; }

	inline std::vector<ShaderSPIR_V> GetShaderByteCodes() const;

protected:
	// The fields below is going to be set by the function CompileShaders function...
	DescSetLayoutBindingMap mPipelineSetLayoutInfo;
	PushConstantSubrangeInfos mPushConstantSubranges;
	std::vector<vk::PushConstantRange> mPushConstantRanges;

	std::vector<CompileResult> mCompileResults;

	CompilerConfig mConfig
	{
		glslang::EShTargetClientVersion::EShTargetVulkan_1_3,
		glslang::EShTargetLanguageVersion::EShTargetSpv_1_6,
		440
	};

	CompilerEnvironment mEnv;

	glm::uvec3 mWorkGroupSize{};

	PipelineType mType = PipelineType::eNone;

	template <typename Context, typename BasePipeline>
	friend class GraphicsPipeline;

	friend class Device;

private:
	// Helper methods...
	inline void SaveLayoutInfos(const CompileResult& result);
	inline void SavePushConstantRanges(const CompileResult& result);
	inline void SaveShaderMetaData(CompileResult Result);

	inline bool CheckIfSameBindings(const vk::DescriptorSetLayoutBinding& first,
		const vk::DescriptorSetLayoutBinding& second) const;
};

void PipelineContext::SaveLayoutInfos(const CompileResult& result)
{
	for (const auto& [SetIndex, Bindings] : result.SetLayoutBindingsMap)
	{
		auto& SetInfo = mPipelineSetLayoutInfo[SetIndex];
		size_t SetInfoSize = SetInfo.size();

		SetInfo.reserve(SetInfo.size() + Bindings.size());

		for (const auto& Binding : Bindings)
		{
			bool Found = false;

			std::for_each(SetInfo.begin(), SetInfo.begin() + SetInfoSize,
				[this, &Binding, &Found, &result](vk::DescriptorSetLayoutBinding& current)
			{
				// TODO: Trigger an assert or throw an exception if two bindings 
				// have same binding and set index but difference name, types or data types!
				Found |= CheckIfSameBindings(current, Binding);
				if (Found)
					current.stageFlags |= result.MetaData.ShaderType;
			});

			if(!Found)
				SetInfo.emplace_back(Binding);
			else
			{

			}
		}
	}
}

void PipelineContext::SavePushConstantRanges(const CompileResult& result)
{
	mPushConstantSubranges = result.LayoutData.PushConstantSubrangeInfos;
	mPushConstantRanges = result.LayoutData.PushConstantsData;
}

std::vector<ShaderSPIR_V> PipelineContext::GetShaderByteCodes() const
{
	std::vector<ShaderSPIR_V> byteCodes;

	for (const auto& result : mCompileResults)
		byteCodes.push_back(result.SPIR_V);

	return byteCodes;
}

inline void PipelineContext::SaveShaderMetaData(CompileResult Result)
{
	if (Result.MetaData.ShaderType == vk::ShaderStageFlagBits::eCompute)
		mWorkGroupSize = Result.MetaData.WorkGroupSize;
}

std::vector<CompileError> PipelineContext::CompileShaders(
	const std::vector<ShaderInput>& ShaderSrcCode)
{
	ShaderCompiler compiler(mEnv);

	mPipelineSetLayoutInfo.clear();
	mPushConstantSubranges.clear();
	mPushConstantRanges.clear();

	mCompileResults.clear();
	mCompileResults.reserve(ShaderSrcCode.size());

	std::vector<CompileError> Errors;
	Errors.reserve(ShaderSrcCode.size());

	for (const auto& srcCode : ShaderSrcCode)
	{
		auto Result = compiler.Compile(srcCode);
		Errors.push_back(Result.Error);

		// Retrieve the reflection results...
		SaveLayoutInfos(Result);
		SavePushConstantRanges(Result);
		SaveShaderMetaData(Result);

		mCompileResults.push_back(std::move(Result));
	}

	return Errors;
}

bool PipelineContext::CheckIfSameBindings(const vk::DescriptorSetLayoutBinding& first, 
	const vk::DescriptorSetLayoutBinding& second) const
{
	return first.binding == second.binding && 
		first.descriptorCount == second.descriptorCount &&
		first.descriptorType == second.descriptorType;
}

VK_END

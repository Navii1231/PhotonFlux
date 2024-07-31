#pragma once
#include "../Core/Config.h"
#include "glslang/Public/ShaderLang.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/Public/ResourceLimits.h"

#include <string>
#include <vector>

VK_BEGIN

using DescSetLayoutBindingMap = std::unordered_map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>>;

enum class ErrorType
{
	eNone = 0,
	ePreprocess = 1,
	eParsing = 2,
	eLinking = 3,
};

struct CompileError
{
	ErrorType Type = ErrorType::eNone;
	std::string Info;
	std::string DebugInfo;
	std::string SrcCode;
	std::string PreprocessedCode;

	std::string FilePath;
};

struct ShaderSPIR_V
{
	std::vector<uint32_t> ByteCode;
	vk::ShaderStageFlagBits Stage;
};

struct ShaderInput
{
	std::string SrcCode;
	vk::ShaderStageFlagBits Stage;

	std::string FilePath;
};

struct CompilerConfig
{
	glslang::EShTargetClientVersion VulkanVersion;
	glslang::EShTargetLanguageVersion SPV_Version = glslang::EShTargetSpv_1_5;
	int GlslVersion = 330;
};

struct DescriptorInfo
{
	uint32_t SetIndex = -1;
	uint32_t BindingIndex = -1;
	std::string Name;
	spirv_cross::SPIRType DescReflectionInfo;
	vk::DescriptorType DescType;
};

struct ShaderMetaData
{
	vk::ShaderStageFlagBits ShaderType;
	glm::uvec3 WorkGroupSize{}; // For compute shader...
};

struct CompileResult
{
	CompilerConfig Config;
	ShaderMetaData MetaData;

	std::vector<DescriptorInfo> LayoutInfos;
	DescSetLayoutBindingMap SetLayoutBindingsMap;
	std::vector<vk::PushConstantRange> PushConstantRanges;

	CompileError Error;
	ShaderSPIR_V SPIR_V;
};

VK_END

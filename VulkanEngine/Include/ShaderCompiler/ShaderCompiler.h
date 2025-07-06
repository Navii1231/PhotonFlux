#pragma once
#include "../Core/Config.h"
#include "ShaderConfig.h"
#include "CompilerEnvironment.h"
#include "Lexer.h"

VK_BEGIN

// A thread safe GLSL (for Vulkan) compiler library...
class ShaderCompiler
{
public:
	explicit ShaderCompiler(const CompilerEnvironment& env)
		: mEnvironment(env) { ResetInternal(env.GetConfig()); }

	~ShaderCompiler() = default;

	ShaderCompiler(const ShaderCompiler& other);
	ShaderCompiler& operator=(const ShaderCompiler& other);

	CompileResult Compile(const ShaderInput& Input);
	CompileResult Compile(ShaderInput&& Input);

	static vk::ShaderStageFlagBits ConvertShaderStage(EShLanguage Stage);
	static EShLanguage ConvertShaderStage(vk::ShaderStageFlagBits Stage);

	static std::string GetShaderStageString(vk::ShaderStageFlagBits flag);
	static vk::ShaderStageFlagBits GetShaderStageFlag(const std::string& shaderStage);

private:
	// Output/Input fields...
	CompilerEnvironment mEnvironment;

private:
	// Helper Functions...
	bool PreprocessShader(CompileResult& SrcCode,  EShLanguage stage);
	bool ParseShader(CompileResult& Result, EShLanguage stage);
	bool GenerateSPIR_V(CompileResult& Result, EShLanguage Stage, glslang::TShader& shader);

	void ReflectDescriptorLayouts(CompileResult& Result);
	void ResetInternal(const CompilerConfig& in);

	void OptimizeCode(CompileResult& Result, OptimizerFlag Flag);


	glslang::TShader MakeGLSLangShader(const CompilerEnvironment& Env, EShLanguage Stage);
	void ReflectShaderMetaData(CompileResult& Result);
	bool ReadAndPreprocess(CompileResult& Result, const ShaderInput& Input);

	void ResolveIncludeRecursive(CompileResult& Result, IncludeResult& Base, 
		std::shared_ptr<ShaderIncluder> Includer, uint32_t RecursionDepth);

	void ParseIncludeDirective(CompileResult& Result, Lexer& lexer, IncludeResult& Base, 
		std::shared_ptr<ShaderIncluder> Includer, uint32_t RecursionDepth, Token& token);

};

VK_END

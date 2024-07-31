#include "ShaderCompiler/ShaderCompiler.h"
#include "spirv_cross/spirv_cross.hpp"
#include "spirv_cross/spirv_glsl.hpp"

#include "ShaderCompiler/Lexer.h"

#include <iostream>
#include <unordered_map>

VK_BEGIN

struct ProcessInitializer
{
	ProcessInitializer()
	{
		glslang::InitializeProcess();
		InitMaps();
	}

	~ProcessInitializer()
	{
		glslang::FinalizeProcess();
	}

	DescSetLayoutBindingMap GetSetBindings(const std::vector<DescriptorInfo>& Infos, 
		vk::ShaderStageFlags Stages) const
	{
		DescSetLayoutBindingMap SetBindingsMap;
		SetBindingsMap.reserve(Infos.size());

		for (const auto& desc : Infos)
		{
			auto& Bindings = SetBindingsMap[desc.SetIndex];
			auto& RecentBinding = Bindings.emplace_back();

			RecentBinding.binding = desc.BindingIndex;
			RecentBinding.descriptorCount = 1;
			RecentBinding.descriptorType = desc.DescType;
			RecentBinding.stageFlags = Stages;
		}

		return SetBindingsMap;
	}

	std::unordered_map<vk::ShaderStageFlagBits, EShLanguage> mVulkanToEShStage;
	std::unordered_map<EShLanguage, vk::ShaderStageFlagBits> mEShToVulkanStage;

private:

	void InitMaps()
	{
		mEShToVulkanStage[EShLangVertex] = vk::ShaderStageFlagBits::eVertex;
		mEShToVulkanStage[EShLangTessControl] = vk::ShaderStageFlagBits::eTessellationControl;
		mEShToVulkanStage[EShLangTessEvaluation] = vk::ShaderStageFlagBits::eTessellationEvaluation;
		mEShToVulkanStage[EShLangGeometry] = vk::ShaderStageFlagBits::eGeometry;
		mEShToVulkanStage[EShLangFragment] = vk::ShaderStageFlagBits::eFragment;
		mEShToVulkanStage[EShLangCompute] = vk::ShaderStageFlagBits::eCompute;
		mEShToVulkanStage[EShLangRayGen] = vk::ShaderStageFlagBits::eRaygenKHR;
		mEShToVulkanStage[EShLangRayGenNV] = vk::ShaderStageFlagBits::eRaygenNV;
		mEShToVulkanStage[EShLangIntersect] = vk::ShaderStageFlagBits::eIntersectionKHR;
		mEShToVulkanStage[EShLangIntersectNV] = vk::ShaderStageFlagBits::eIntersectionNV;
		mEShToVulkanStage[EShLangAnyHit] = vk::ShaderStageFlagBits::eAnyHitKHR;
		mEShToVulkanStage[EShLangAnyHitNV] = vk::ShaderStageFlagBits::eAnyHitNV;
		mEShToVulkanStage[EShLangClosestHit] = vk::ShaderStageFlagBits::eClosestHitKHR;
		mEShToVulkanStage[EShLangClosestHitNV] = vk::ShaderStageFlagBits::eClosestHitNV;
		mEShToVulkanStage[EShLangMiss] = vk::ShaderStageFlagBits::eMissKHR;
		mEShToVulkanStage[EShLangMissNV] = vk::ShaderStageFlagBits::eMissNV;
		mEShToVulkanStage[EShLangCallable] = vk::ShaderStageFlagBits::eCallableKHR;
		mEShToVulkanStage[EShLangCallableNV] = vk::ShaderStageFlagBits::eCallableNV;
		mEShToVulkanStage[EShLangTask] = vk::ShaderStageFlagBits::eTaskEXT;
		mEShToVulkanStage[EShLangTaskNV] = vk::ShaderStageFlagBits::eTaskNV;
		mEShToVulkanStage[EShLangMesh] = vk::ShaderStageFlagBits::eMeshEXT;
		mEShToVulkanStage[EShLangMeshNV] = vk::ShaderStageFlagBits::eMeshNV;

		mVulkanToEShStage[vk::ShaderStageFlagBits::eVertex] = EShLangVertex;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eTessellationControl] = EShLangTessControl;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eTessellationEvaluation] = EShLangTessEvaluation;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eGeometry] = EShLangGeometry;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eFragment] = EShLangFragment;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eCompute] = EShLangCompute;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eRaygenKHR] = EShLangRayGen;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eRaygenNV] = EShLangRayGenNV;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eAnyHitKHR] = EShLangAnyHit;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eAnyHitNV] = EShLangAnyHitNV;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eClosestHitKHR] = EShLangClosestHit;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eClosestHitNV] = EShLangClosestHitNV;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eMissKHR] = EShLangMiss;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eMissNV] = EShLangMissNV;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eIntersectionKHR] = EShLangIntersect;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eIntersectionNV] = EShLangIntersectNV;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eCallableKHR] = EShLangCallable;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eCallableNV] = EShLangCallableNV;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eTaskEXT] = EShLangTask;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eTaskNV] = EShLangTaskNV;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eMeshEXT] = EShLangMesh;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eMeshNV] = EShLangMeshNV;

		mVulkanToEShStage[vk::ShaderStageFlagBits::eSubpassShadingHUAWEI] = EShLangCount;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eClusterCullingHUAWEI] = EShLangCount;

		mVulkanToEShStage[vk::ShaderStageFlagBits::eAllGraphics] = EShLangCount;
		mVulkanToEShStage[vk::ShaderStageFlagBits::eAll] = EShLangCount;

	}

} const sCompilerInitializer;

VK_END

VK_NAMESPACE::ShaderCompiler::ShaderCompiler(const ShaderCompiler& other)
	: mEnvironment(other.mEnvironment) {}

VK_NAMESPACE::ShaderCompiler& VK_NAMESPACE::ShaderCompiler::operator=(const ShaderCompiler& other)
{
	mEnvironment = other.mEnvironment;

	return *this;
}

VK_NAMESPACE::CompileResult VK_NAMESPACE::ShaderCompiler::Compile(ShaderInput&& Input)
{
	CompileResult Result;
	Result.Config = mEnvironment.GetConfig();

	if (!Input.FilePath.empty())
		ReadAndPreprocess(Result, Input);
	else
		Result.Error.SrcCode = std::move(Input.SrcCode);

	if (Result.Error.Type != ErrorType::eNone)
		return Result;

	Result.Error.FilePath = std::move(Input.FilePath);

	auto EShStage = ConvertShaderStage(Input.Stage);

	if (!PreprocessShader(Result, EShStage)) 
		return Result;

	if (!ParseShader(Result, EShStage))
		return Result;

	ReflectDescriptorLayouts(Result);
	ReflectShaderMetaData(Result);

	return Result;
}

vk::ShaderStageFlagBits VK_NAMESPACE::ShaderCompiler::ConvertShaderStage(EShLanguage Stage)
{
	return sCompilerInitializer.mEShToVulkanStage.at(Stage);
}

EShLanguage VK_NAMESPACE::ShaderCompiler::ConvertShaderStage(vk::ShaderStageFlagBits Stage)
{
	return sCompilerInitializer.mVulkanToEShStage.at(Stage);
}

VK_NAMESPACE::CompileResult VK_NAMESPACE::ShaderCompiler::Compile(const ShaderInput& Input)
{
	auto Copy = Input;
	return Compile(std::move(Copy));
}

bool VK_NAMESPACE::ShaderCompiler::PreprocessShader(
	CompileResult& ResultRef, EShLanguage Stage)
{
	auto TShader = MakeGLSLangShader(mEnvironment.GetConfig(), Stage);
	const char* Source = ResultRef.Error.SrcCode.c_str();

	TShader.setStrings(&Source, 1);

	glslang::TShader::ForbidIncluder basicIncluder = glslang::TShader::ForbidIncluder();

	bool Success = TShader.preprocess(GetDefaultResources(), 100, EProfile::ENoProfile,
		false, true, EShMsgDefault, &ResultRef.Error.PreprocessedCode, basicIncluder);

	if (!Success)
	{
		ResultRef.Error.DebugInfo = TShader.getInfoDebugLog();
		ResultRef.Error.Info = TShader.getInfoLog();
		ResultRef.Error.Type = ErrorType::ePreprocess;
	}

	return Success;
}

bool VK_NAMESPACE::ShaderCompiler::ParseShader(CompileResult& Result, EShLanguage Stage)
{
	auto TShader = MakeGLSLangShader(mEnvironment.GetConfig(), Stage);
	const char* Source = Result.Error.PreprocessedCode.c_str();

	TShader.setStrings(&Source, 1);

	glslang::TShader::ForbidIncluder basicIncluder = glslang::TShader::ForbidIncluder();

	bool Success = TShader.parse(GetDefaultResources(), 100, EProfile::ENoProfile,
		false, true, EShMsgDefault, basicIncluder);

	if (!Success)
	{
		Result.Error.DebugInfo = TShader.getInfoDebugLog();
		Result.Error.Info = TShader.getInfoLog();
		Result.Error.Type = ErrorType::eParsing;

		return Success;
	}

	return GenerateSPIR_V(Result, Stage, TShader);
}

bool VK_NAMESPACE::ShaderCompiler::GenerateSPIR_V(
	CompileResult& Result, EShLanguage Stage, glslang::TShader& shader)
{
	glslang::TProgram Program;
	Program.addShader(&shader);

	bool Success = Program.link(EShMsgDefault);

	if (!Success)
	{
		Result.Error.Info = Program.getInfoLog();
		Result.Error.DebugInfo = Program.getInfoDebugLog();
		Result.Error.Type = ErrorType::eLinking;

		return Success;
	}

	glslang::GlslangToSpv(
		*Program.getIntermediate(Stage), Result.SPIR_V.ByteCode);

	Result.SPIR_V.Stage = ConvertShaderStage(Stage);

	return Success;
}

void VK_NAMESPACE::ShaderCompiler::ResetInternal(const CompilerConfig& new_in)
{
	// Reset all the fields...
}

glslang::TShader VK_NAMESPACE::ShaderCompiler::MakeGLSLangShader(
	const CompilerConfig& in, EShLanguage Stage)
{
	glslang::TShader shader(Stage);

	shader.setEnvInput(
		glslang::EShSourceGlsl,
		Stage,
		glslang::EShClientVulkan,
		in.GlslVersion);

	shader.setEnvClient(glslang::EShClientVulkan, in.VulkanVersion);
	shader.setEnvTarget(glslang::EShTargetSpv, in.SPV_Version);

	return shader;
}

void VK_NAMESPACE::ShaderCompiler::ReflectShaderMetaData(CompileResult& Result)
{
	auto& ByteCode = Result.SPIR_V.ByteCode;
	Result.MetaData.ShaderType = Result.SPIR_V.Stage;

	spirv_cross::CompilerGLSL compiler(ByteCode);

	if (Result.MetaData.ShaderType == vk::ShaderStageFlagBits::eCompute)
	{
		spirv_cross::SPIREntryPoint entryPoint = compiler.get_entry_point("main", spv::ExecutionModelGLCompute);
		Result.MetaData.WorkGroupSize.x = entryPoint.workgroup_size.x;
		Result.MetaData.WorkGroupSize.y = entryPoint.workgroup_size.y;
		Result.MetaData.WorkGroupSize.z = entryPoint.workgroup_size.z;
	}
}

bool VK_NAMESPACE::ShaderCompiler::ReadAndPreprocess(CompileResult& Result, const ShaderInput& Input)
{
	if (!ReadFile(Input.FilePath, Result.Error.SrcCode))
		return false;

	auto Includer = mEnvironment.CreateShaderIncluder(Input.FilePath);

	IncludeResult Base;
	Base.Contents = Result.Error.SrcCode;
	Base.HeaderName;

	ResolveIncludeRecursive(Result, Base, Includer, 0);

	if(Result.Error.Type == ErrorType::eNone)
		Result.Error.SrcCode = Base.Contents;

	return true;
}

void VK_NAMESPACE::ShaderCompiler::ResolveIncludeRecursive(CompileResult& Result, IncludeResult& Base,
	std::shared_ptr<ShaderIncluder> Includer, uint32_t RecursionDepth)
{
	Lexer lexer(Base.Contents);
	lexer.SetWhiteSpacesAndDelimiters(" \t", "#<>\"\n");

	Token token = lexer.NextToken();

	while (token.Value.front() && Result.Error.Type == ErrorType::eNone)
	{
		if (token.Value == "#")
		{
			Token nextToken = lexer.NextToken();

			if (!nextToken.Value.front())
				break;

			if (nextToken.Value == "include")
				ParseIncludeDirective(Result, lexer, Base, Includer, RecursionDepth, token);
			else
				token = nextToken;
		}
		else
			token = lexer.NextToken();
	}
}

void VK_NAMESPACE::ShaderCompiler::ParseIncludeDirective(CompileResult& Result, Lexer& lexer, IncludeResult& Base,
	std::shared_ptr<ShaderIncluder> Includer, uint32_t RecursionDepth, Token& token)
{
	std::string lineString = std::to_string(token.LineNumber);
	std::string charOffset = std::to_string(token.CharOffset);

	size_t index = 0;
	std::array<Token, 4> tokens;

	while (index < tokens.size())
	{
		tokens[index] = lexer.NextToken();
		if (tokens[index].Value == "\n")
			break;

		index++;
	}

	if (index != 3)
	{
		Result.Error.Type = ErrorType::ePreprocess;
		Result.Error.Info = "Error: line " + lineString + ", " + charOffset + ": Ill formed #include directive";
		return;
	}

	Token HeaderName = tokens[1];
	std::string IncluderName = Base.HeaderName;

	IncludeResult* Inclusion = nullptr;
	std::filesystem::path includerPath(IncluderName);
	includerPath = includerPath.parent_path();

	if (tokens[0].Value == "<" && tokens[2].Value == ">")
		Inclusion = Includer->IncludeSystem(HeaderName.Value, includerPath.string(), RecursionDepth);
	if (tokens[0].Value == "\"" && tokens[2].Value == "\"")
		Inclusion = Includer->IncludeLocal(HeaderName.Value, includerPath.string(), RecursionDepth);
	else
	{
		Result.Error.Type = ErrorType::ePreprocess;
		Result.Error.Info = "Error: " + lineString + ", " + charOffset + ": Ill formed #include directive";
		return;
	}

	if (!Inclusion)
	{
		Result.Error.Type = ErrorType::ePreprocess;
		Result.Error.Info = "Error: line " + lineString + ", " + charOffset + 
			": Can't open file at " + HeaderName.Value;
		return;
	}

	ResolveIncludeRecursive(Result, *Inclusion, Includer, RecursionDepth + 1);

	size_t InitPos = token.Position;
	size_t FinalPos = tokens[3].Position;

	Base.Contents.erase(InitPos, FinalPos - InitPos);
	Base.Contents.insert(InitPos, Inclusion->Contents);

	lexer = Lexer(Base.Contents);
	lexer.SetWhiteSpacesAndDelimiters(" \t", "#<>\"\n");

	size_t CursorPos = InitPos + Inclusion->Contents.size();

	Includer->ReleaseInclude(Inclusion);

	if (Result.Error.Type != ErrorType::eNone)
		return;

	lexer.SetCursor(CursorPos);
	token = lexer.NextToken();
}

void VK_NAMESPACE::ShaderCompiler::ReflectDescriptorLayouts(CompileResult& Result)
{
	auto& ByteCode = Result.SPIR_V.ByteCode;

	spirv_cross::Compiler Compiler(ByteCode);

	auto ShaderResources = Compiler.get_shader_resources();

	auto FillDescriptor = [&Result, &Compiler]
	(const spirv_cross::SmallVector<spirv_cross::Resource>& Resources, vk::DescriptorType DescType)
	{
		for (const auto& Resource : Resources)
		{
			uint32_t Binding = Compiler.get_decoration(Resource.id, spv::DecorationBinding);
			uint32_t SetIndex = Compiler.get_decoration(Resource.id, spv::DecorationDescriptorSet);
			std::string name = Resource.name;
			const auto& Type = Compiler.get_type(Resource.type_id);

			Result.LayoutInfos.emplace_back(SetIndex, Binding, name, Type, DescType);
		}
	};

	// TODO: Add input attachment descriptor as well!!
	FillDescriptor(ShaderResources.uniform_buffers, vk::DescriptorType::eUniformBuffer);
	FillDescriptor(ShaderResources.storage_buffers, vk::DescriptorType::eStorageBuffer);
	FillDescriptor(ShaderResources.sampled_images, vk::DescriptorType::eCombinedImageSampler);
	FillDescriptor(ShaderResources.separate_samplers, vk::DescriptorType::eSampler);
	FillDescriptor(ShaderResources.separate_images, vk::DescriptorType::eSampledImage);
	FillDescriptor(ShaderResources.storage_images, vk::DescriptorType::eStorageImage);

	Result.SetLayoutBindingsMap = sCompilerInitializer.GetSetBindings(Result.LayoutInfos, Result.SPIR_V.Stage);
}

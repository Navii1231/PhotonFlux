#include "Core/Aqpch.h"
#include "Wavefront/WavefrontEstimator.h"

#include "ShaderCompiler/Lexer.h"
#include "Wavefront/BVHFactory.h"

AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::WavefrontEstimator(
	const WavefrontEstimatorCreateInfo& createInfo)
	: mCreateInfo(createInfo)
{
	mPipelineBuilder = mCreateInfo.Context.MakePipelineBuilder();
	mResourcePool = mCreateInfo.Context.CreateResourcePool();

	RetrieveFrontAndBackEndShaders();
}

AQUA_NAMESPACE::PH_FLUX_NAMESPACE::TraceSession AQUA_NAMESPACE::PH_FLUX_NAMESPACE::
	WavefrontEstimator::CreateTraceSession()
{
	TraceSession traceSession{};

	traceSession.mSessionInfo = std::make_shared<SessionInfo>();

	CreateTraceBuffers(*traceSession.mSessionInfo);

	return traceSession;
}

AQUA_NAMESPACE::PH_FLUX_NAMESPACE::MaterialPipeline AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::
	CreateMaterialPipeline(const MaterialCreateInfo& createInfo)
{
	/*
	* user defined macro definitions...
	* SHADER_TOLERENCE = 0.001, POWER_HEURISTIC_EXP = 2.0,
	* EMPTY_MATERIAL_ID = -1, SKYBOX_MATERIAL_ID = -2, LIGHT_MATERIAL_ID = -3,
	* RR_CUTOFF_CONST
	*/

	MaterialCreateInfo pipelineCreation = createInfo;
	pipelineCreation.ShaderCode = StitchFrontAndBackShaders(createInfo.ShaderCode);

	vkEngine::PShader shader{};

	shader.AddMacro("WORKGROUP_SIZE", std::to_string(createInfo.WorkGroupSize));
	shader.AddMacro("SHADING_TOLERENCE", std::to_string(static_cast<double>(createInfo.ShadingTolerence)));
	shader.AddMacro("TOLERENCE", std::to_string(static_cast<double>(createInfo.IntersectionTolerence)));
	shader.AddMacro("EPSILON", std::to_string(static_cast<double>(FLT_EPSILON)));
	shader.AddMacro("POWER_HEURISTICS_EXP", std::to_string(static_cast<double>(createInfo.PowerHeuristics)));
	shader.AddMacro("EMPTY_MATERIAL_ID", std::to_string(static_cast<int>(-1)));
	shader.AddMacro("SKYBOX_MATERIAL_ID", std::to_string(static_cast<int>(-2)));
	shader.AddMacro("LIGHT_MATERIAL_ID", std::to_string(static_cast<int>(-3)));
	shader.AddMacro("RR_CUTOFF_CONST", std::to_string(static_cast<int>(-4)));

	vkEngine::OptimizerFlag flag = vkEngine::OptimizerFlag::eO3;

	shader.SetShader("eCompute", createInfo.ShaderCode, flag);

	auto Errors = shader.CompileShaders();

	CompileErrorChecker checker("Logging/ShaderFails/Shader.glsl");
	checker.AssertOnError(Errors);

	return mPipelineBuilder.BuildComputePipeline<MaterialPipeline>(shader);
}

AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::
	CreateExecutor(const ExecutorCreateInfo& createInfo)
{
	Executor executor{};

	executor.mExecutorInfo = std::make_shared<ExecutionInfo>();
	executor.mExecutorInfo->PipelineResources = CreatePipelines();
	executor.mExecutorInfo->CreateInfo = createInfo;

	CreateExecutorBuffers(*executor.mExecutorInfo, createInfo);
	CreateExecutorImages(*executor.mExecutorInfo, createInfo);

	return executor;
}

AQUA_NAMESPACE::PH_FLUX_NAMESPACE::ExecutionPipelines AQUA_NAMESPACE::PH_FLUX_NAMESPACE::
	WavefrontEstimator::CreatePipelines()
{
	MaterialCreateInfo inactiveMaterialInfo{};
	inactiveMaterialInfo.PowerHeuristics = 2.0f;
	inactiveMaterialInfo.ShadingTolerence = 0.001f;
	inactiveMaterialInfo.WorkGroupSize = mCreateInfo.IntersectionWorkgroupSize;

	std::string emptyShader = "SampleInfo Evaluate(in Ray ray, in CollisionInfo collisionInfo)"
		"{ SampleInfo sampleInfo; sampleInfo.Weight = 1.0; sampleInfo.Luminance = vec3(0.0);"
		"return sampleInfo; }";

	inactiveMaterialInfo.ShaderCode = StitchFrontAndBackShaders(emptyShader);

	ExecutionPipelines pipelines;

	pipelines.SortRecorder = std::make_shared<SortRecorder<uint32_t>>(mPipelineBuilder, mResourcePool);

	//mRayRefs = mPipelineResources.SortRecorder->GetBuffer();

	pipelines.RayGenerator = mPipelineBuilder.BuildComputePipeline<RayGenerationPipeline>(GetRayGenerationShader());
	pipelines.IntersectionPipeline = mPipelineBuilder.BuildComputePipeline<IntersectionPipeline>(GetIntersectionShader());
	pipelines.RaySortPreparer = mPipelineBuilder.BuildComputePipeline<RaySortEpiloguePipeline>(GetRaySortEpilogueShader(RaySortEvent::ePrepare));
	pipelines.RaySortFinisher = mPipelineBuilder.BuildComputePipeline<RaySortEpiloguePipeline>(GetRaySortEpilogueShader(RaySortEvent::eFinish));
	pipelines.RayRefCounter = mPipelineBuilder.BuildComputePipeline<RayRefCounterPipeline>(GetRayRefCounterShader());
	pipelines.PrefixSummer = mPipelineBuilder.BuildComputePipeline<PrefixSumPipeline>(GetPrefixSumShader());
	pipelines.InactiveRayShader = CreateMaterialPipeline(inactiveMaterialInfo);
	pipelines.LuminanceMean = mPipelineBuilder.BuildComputePipeline<LuminanceMeanPipeline>(GetLuminanceMeanShader());
	pipelines.PostProcessor = mPipelineBuilder.BuildComputePipeline<PostProcessImagePipeline>(GetPostProcessImageShader());

	return pipelines;
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::CreateTraceBuffers(SessionInfo& session)
{
	vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eStorageBuffer;
	vk::MemoryPropertyFlags memProps = vk::MemoryPropertyFlagBits::eHostCoherent;

	session.SharedBuffers.Vertices = mResourcePool.CreateBuffer<glm::vec4>(usage, memProps);
	session.SharedBuffers.Faces = mResourcePool.CreateBuffer<Face>(usage, memProps);
	session.SharedBuffers.TexCoords = mResourcePool.CreateBuffer<glm::vec2>(usage, memProps);
	session.SharedBuffers.Normals = mResourcePool.CreateBuffer<glm::vec4>(usage, memProps);
	session.SharedBuffers.Nodes = mResourcePool.CreateBuffer<Node>(usage, memProps);

	session.MeshInfos = mResourcePool.CreateBuffer<MeshInfo>(usage, memProps);
	session.LightInfos = mResourcePool.CreateBuffer<LightInfo>(usage, memProps);
	session.LightPropsInfos = mResourcePool.CreateBuffer<LightProperties>(usage, memProps);

	memProps = vk::MemoryPropertyFlagBits::eDeviceLocal;

	session.LocalBuffers.Vertices = mResourcePool.CreateBuffer<glm::vec4>(usage, memProps);
	session.LocalBuffers.Faces = mResourcePool.CreateBuffer<Face>(usage, memProps);
	session.LocalBuffers.TexCoords = mResourcePool.CreateBuffer<glm::vec2>(usage, memProps);
	session.LocalBuffers.Normals = mResourcePool.CreateBuffer<glm::vec4>(usage, memProps);
	session.LocalBuffers.Nodes = mResourcePool.CreateBuffer<Node>(usage, memProps);

	// SceneInfo and physical camera buffer is a uniform and should be host coherent...
	usage = vk::BufferUsageFlagBits::eUniformBuffer;
	memProps = vk::MemoryPropertyFlagBits::eHostCoherent;

	// Intersection stage...
	session.CameraSpecsBuffer = mResourcePool.CreateBuffer<PhysicalCamera>(usage, memProps);
	session.ShaderConstData = mResourcePool.CreateBuffer<ShaderData>(usage, memProps);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::CreateExecutorBuffers(
	ExecutionInfo& executionInfo, const ExecutorCreateInfo& executorInfo)
{
	vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eStorageBuffer;
	vk::MemoryPropertyFlags memProps = vk::MemoryPropertyFlagBits::eDeviceLocal;

	// TODO: Making them host coherent for now... But they really should be
	// local buffers in the final build...

	executionInfo.PipelineResources.SortRecorder->ResizeBuffer(2 * executorInfo.TileSize.x * executorInfo.TileSize.y);
	executionInfo.RayRefs = executionInfo.PipelineResources.SortRecorder->GetBuffer();

	//createInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;
	// TODO: --^ Not necessary, in fact bad for performance

	executionInfo.Rays = mResourcePool.CreateBuffer<Ray>(usage, memProps);
	executionInfo.RayInfos = mResourcePool.CreateBuffer<RayInfo>(usage, memProps);
	executionInfo.CollisionInfos = mResourcePool.CreateBuffer<CollisionInfo>(usage, memProps);

	executionInfo.RefCounts = mResourcePool.CreateBuffer<uint32_t>(usage, memProps);

	uint32_t RayCount = executorInfo.TargetResolution.x * executorInfo.TargetResolution.y;

	executionInfo.Rays.Resize(2 * RayCount);
	executionInfo.RayInfos.Resize(2 * RayCount);
	executionInfo.CollisionInfos.Resize(2 * RayCount);

	usage = vk::BufferUsageFlagBits::eUniformBuffer;
	memProps = vk::MemoryPropertyFlagBits::eHostCoherent;

	executionInfo.Scene = mResourcePool.CreateBuffer<WavefrontSceneInfo>(usage, memProps);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::CreateExecutorImages(
	ExecutionInfo& executionInfo, const ExecutorCreateInfo& executorInfo)
{
	vkEngine::ImageCreateInfo imageInfo{};
	imageInfo.Extent = vk::Extent3D(executorInfo.TileSize.x, executorInfo.TileSize.y, 1);
	imageInfo.Format = vk::Format::eR32G32B32A32Sfloat;
	imageInfo.MemProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
	imageInfo.Type = vk::ImageType::e2D;
	imageInfo.Usage = vk::ImageUsageFlagBits::eStorage;

	executionInfo.Target.PixelMean = mResourcePool.CreateImage(imageInfo);
	executionInfo.Target.PixelVariance = mResourcePool.CreateImage(imageInfo);

	imageInfo.Format = vk::Format::eR8G8B8A8Snorm;
	executionInfo.Target.Presentable = mResourcePool.CreateImage(imageInfo);

	executionInfo.Target.ImageResolution = executorInfo.TargetResolution;

	executionInfo.Target.PixelMean.TransitionLayout(
		vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eTopOfPipe);

	executionInfo.Target.PixelVariance.TransitionLayout(
		vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eTopOfPipe);

	executionInfo.Target.Presentable.TransitionLayout(
		vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eTopOfPipe);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::AddText(std::string& text, const std::string& filepath)
{
	std::string fileText;
	vkEngine::ReadFile(filepath, fileText);
	text += fileText + "\n\n";
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::RetrieveFrontAndBackEndShaders()
{
	// TODO: Retrieve the vulkan version from the vkEngine library...
	mShaderFrontEnd = "#version 440\n\n";

	// All the front shaders and custom libraries...
	AddText(mShaderFrontEnd, GetShaderDirectory() + "Wavefront/Common.glsl");
	AddText(mShaderFrontEnd, GetShaderDirectory() + "BSDFs/CommonBSDF.glsl");
	AddText(mShaderFrontEnd, GetShaderDirectory() + "BSDFs/BSDF_Samplers.glsl");
	AddText(mShaderFrontEnd, GetShaderDirectory() + "MaterialShaders/ShaderFrontEnd.glsl");
	AddText(mShaderFrontEnd, GetShaderDirectory() + "BSDFs/Utils.glsl");

	/* TODO: This is temporary, should be dealt by an import system */

#if 0
	AddText(mShaderFrontEnd, "Shaders/BSDFs/DiffuseBSDF.glsl");
	AddText(mShaderFrontEnd, "Shaders/BSDFs/GlossyBSDF.glsl");
	AddText(mShaderFrontEnd, "Shaders/BSDFs/RefractionBSDF.glsl");
	AddText(mShaderFrontEnd, "Shaders/BSDFs/CookTorranceBSDF.glsl");
	AddText(mShaderFrontEnd, "Shaders/BSDFs/GlassBSDF.glsl");
#else
	AddText(mImportToShaders["DiffuseBSDF"], GetShaderDirectory() + "BSDFs/DiffuseBSDF.glsl");
	AddText(mImportToShaders["GlossyBSDF"], GetShaderDirectory() + "BSDFs/GlossyBSDF.glsl");
	AddText(mImportToShaders["RefractionBSDF"], GetShaderDirectory() + "BSDFs/RefractionBSDF.glsl");
	AddText(mImportToShaders["CookTorranceBSDF"], GetShaderDirectory() + "BSDFs/CookTorranceBSDF.glsl");
	AddText(mImportToShaders["GlassBSDF"], GetShaderDirectory() + "BSDFs/GlassBSDF.glsl");
#endif

	/****************************************************************/

	// Backend of the pipelines handling luminance calculations
	AddText(mShaderBackEnd, GetShaderDirectory() + "MaterialShaders/ShaderBackEnd.glsl");
}

void DispatchErrorMessage(AQUA_NAMESPACE::PH_FLUX_NAMESPACE::MaterialShaderError error)
{
	std::string errorMessage;

	switch (error.State)
	{
		case AQUA_NAMESPACE::PH_FLUX_NAMESPACE::MaterialPreprocessState::eSuccess:
			return;
		case AQUA_NAMESPACE::PH_FLUX_NAMESPACE::MaterialPreprocessState::eInvalidSyntax:
			errorMessage = "Invalid import syntax: " + error.Info;
			break;
		case AQUA_NAMESPACE::PH_FLUX_NAMESPACE::MaterialPreprocessState::eShaderNotFound:
			errorMessage = "Could not import the shader: " + error.Info;
			break;
		default:
			errorMessage = "Unknown shader error!";
			break;
	}

	_STL_ASSERT(false, errorMessage.c_str());
}

std::string AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::StitchFrontAndBackShaders(
	const std::string& shaderCode)
{
	std::string editedShaderCode = shaderCode;

	MaterialShaderError state = ImportShaders(editedShaderCode);

	DispatchErrorMessage(state);

	/*
	TODO: Only for debug, remove this piece of code after material pipeline is finished
	std::cout << editedShaderCode << std::endl;
	*/

	// Stitching the front, middle and back-end
	std::string pipelineCode;

	pipelineCode += mShaderFrontEnd + "\n\n";
	pipelineCode += editedShaderCode + "\n\n";
	pipelineCode += mShaderBackEnd;

	return pipelineCode;
}

vkEngine::PShader AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::GetRayGenerationShader()
{
	vkEngine::PShader shader{};

	shader.AddMacro("WORKGROUP_SIZE", std::to_string(mCreateInfo.RayGenWorkgroupSize.x));

	shader.SetFilepath("eCompute", GetShaderDirectory() + "Wavefront/RayGeneration.comp", vkEngine::OptimizerFlag::eO3);

	auto Errors = shader.CompileShaders();

	CompileErrorChecker checker("Logging/ShaderFails/Shader.glsl");
	checker.AssertOnError(Errors);

	return shader;
}

vkEngine::PShader AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::GetIntersectionShader()
{
	vkEngine::PShader shader;

	shader.AddMacro("WORKGROUP_SIZE", std::to_string(mCreateInfo.IntersectionWorkgroupSize));
	shader.AddMacro("TOLERENCE", std::to_string(mCreateInfo.Tolerence));
	shader.AddMacro("MAX_DIS", std::to_string(FLT_MAX));
	shader.AddMacro("FLT_MAX", std::to_string(FLT_MAX));

	shader.SetFilepath("eCompute", GetShaderDirectory() + "Wavefront/Intersection.glsl",
		OPTIMIZE_INTERSECTION == 1 ?
		vkEngine::OptimizerFlag::eO3 : vkEngine::OptimizerFlag::eNone);

	auto Errors = shader.CompileShaders();

	CompileErrorChecker checker("../vkEngineTester/Logging/ShaderFails/Shader.glsl");

	auto ErrorInfos = checker.GetErrors(Errors);
	checker.AssertOnError(ErrorInfos);

	return shader;
}

vkEngine::PShader AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::GetRaySortEpilogueShader(RaySortEvent sortEvent)
{
	vkEngine::PShader shader;

	shader.AddMacro("WORKGROUP_SIZE", std::to_string(mCreateInfo.IntersectionWorkgroupSize));

	std::string shaderPath;

	switch (sortEvent)
	{
		case RaySortEvent::ePrepare:
			shaderPath = GetShaderDirectory() + "Wavefront/PrepareRaySort.glsl";
			break;
		case RaySortEvent::eFinish:
			shaderPath = GetShaderDirectory() + "Wavefront/FinishRaySort.glsl";
			break;
		default:
			break;
	}

	shader.SetFilepath("eCompute", shaderPath);

	auto Errors = shader.CompileShaders();

	CompileErrorChecker checker("../vkEngineTester/Logging/ShaderFails/Shader.glsl");

	auto ErrorInfos = checker.GetErrors(Errors);
	checker.AssertOnError(ErrorInfos);

	return shader;
}

vkEngine::PShader AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::GetRayRefCounterShader()
{
	vkEngine::PShader shader;

	shader.AddMacro("WORKGROUP_SIZE", std::to_string(mCreateInfo.IntersectionWorkgroupSize));
	shader.AddMacro("PRIMITIVE_TYPE", "uint");

	shader.SetFilepath("eCompute", GetShaderDirectory() + "Utils/CountElements.glsl");

	auto Errors = shader.CompileShaders();

	CompileErrorChecker checker("../vkEngineTester/Logging/ShaderFails/Shader.glsl");

	auto ErrorInfos = checker.GetErrors(Errors);
	checker.AssertOnError(ErrorInfos);

	return shader;
}

vkEngine::PShader AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::GetPrefixSumShader()
{
	vkEngine::PShader shader;

	shader.AddMacro("WORKGROUP_SIZE", std::to_string(mCreateInfo.IntersectionWorkgroupSize));
	shader.SetFilepath("eCompute", GetShaderDirectory() + "Utils/PrefixSum.glsl");
	
	auto Errors = shader.CompileShaders();

	CompileErrorChecker checker("Logging/ShaderFails/Shader.glsl");

	auto ErrorInfos = checker.GetErrors(Errors);
	checker.AssertOnError(ErrorInfos);

	return shader;
}

vkEngine::PShader AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::GetLuminanceMeanShader()
{
	vkEngine::PShader shader;

	shader.AddMacro("WORKGROUP_SIZE", std::to_string(mCreateInfo.IntersectionWorkgroupSize));
	shader.SetFilepath("eCompute", GetShaderDirectory() + "Wavefront/LuminanceMean.glsl");
	
	auto Errors = shader.CompileShaders();

	CompileErrorChecker checker("Logging/ShaderFails/Shader.glsl");

	auto ErrorInfos = checker.GetErrors(Errors);
	checker.AssertOnError(ErrorInfos);

	return shader;
}

vkEngine::PShader AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::GetPostProcessImageShader()
{
	vkEngine::PShader shader;

	shader.AddMacro("WORKGROUP_SIZE_X", std::to_string(mCreateInfo.RayGenWorkgroupSize.x));
	shader.AddMacro("WORKGROUP_SIZE_Y", std::to_string(mCreateInfo.RayGenWorkgroupSize.y));

	shader.AddMacro("APPLY_TONE_MAP", std::to_string(static_cast<uint32_t>(PostProcessFlagBits::eToneMap)));

	shader.AddMacro("APPLY_GAMMA_CORRECTION",
		std::to_string(static_cast<uint32_t>(PostProcessFlagBits::eGammaCorrection)));

	shader.AddMacro("APPLY_GAMMA_CORRECTION_INV",
		std::to_string(static_cast<uint32_t>(PostProcessFlagBits::eGammaCorrectionInv)));

	shader.SetFilepath("eCompute", GetShaderDirectory() + "Wavefront/PostProcessImage.glsl");

	auto Errors = shader.CompileShaders();

	CompileErrorChecker checker("../vkEngineTester/Logging/ShaderFails/Shader.glsl");

	auto ErrorInfos = checker.GetErrors(Errors);
	checker.AssertOnError(ErrorInfos);

	return shader;
}

AQUA_NAMESPACE::PH_FLUX_NAMESPACE::MaterialShaderError AQUA_NAMESPACE::PH_FLUX_NAMESPACE::
	WavefrontEstimator::ImportShaders(std::string& shaderCode)
{
	// An import system to load pre-existing shaders...
	// Using a lexer for that as we will use custom key word for inclusion
	// The custom keyword and grammar for inclusion is:
	// import <<<ShaderName>>>

	MaterialShaderError error;
	error.State = MaterialPreprocessState::eSuccess;

	vkEngine::Lexer lexer(shaderCode);
	lexer.SetWhiteSpacesAndDelimiters(" \t\r", "\n");

	vkEngine::Token CurrToken = lexer.NextToken();
	vkEngine::Token PrevToken = CurrToken;

	auto ConstructError = [&error](MaterialPreprocessState state, 
		const vkEngine::Token& token, const std::string& errorString)
	{
		error.State = MaterialPreprocessState::eInvalidSyntax;
		error.Info = "line (" + std::to_string(token.LineNumber) + ", " +
			std::to_string(token.CharOffset) + ") -- " + errorString;
	};

	auto SanityCheck = [&shaderCode, &ConstructError](const vkEngine::Token& prevToken,
		const vkEngine::Token& importKeyword,
		const vkEngine::Token& shaderImport,
		const vkEngine::Token& endLine)->bool
	{
		if (shaderImport.Value.empty())
		{
			ConstructError(MaterialPreprocessState::eInvalidSyntax, importKeyword,
				"No shader has been provided");

			return false;
		}

		if (prevToken.Value != "\n" && prevToken.Value != importKeyword.Value)
		{
			ConstructError(MaterialPreprocessState::eInvalidSyntax, prevToken, 
				"The import statement must begin with a new line");

			return false;
		}

		if (endLine.Value != "\n" && endLine.Value[0] != '\0')
		{
			ConstructError(MaterialPreprocessState::eInvalidSyntax, endLine, 
				"Unexpected token \'" + endLine.Value + "\'");

			return false;
		}

		return true;
	};

	while (CurrToken.Value[0] != '\0')
	{
		vkEngine::Token ShaderNameToken = lexer.NextToken();

		if (CurrToken.Value == "import")
		{
			vkEngine::Token EndLineToken = lexer.NextToken();

			if (!SanityCheck(PrevToken, CurrToken, ShaderNameToken, EndLineToken))
				return error;

			const std::string& shaderName = ShaderNameToken.Value;

			size_t position = CurrToken.Position;
			size_t Size = CurrToken.Value.size() + ShaderNameToken.Value.size() + EndLineToken.Value.size();

			auto iter = mImportToShaders.find(shaderName);

			if (iter == mImportToShaders.end())
			{
				ConstructError(MaterialPreprocessState::eShaderNotFound, CurrToken,
				"Could not import the shader: " + shaderName);

				return error;
			}

			shaderCode.erase(position, Size);
			shaderCode.insert(position, iter->second);

			position += iter->second.size();

			lexer = vkEngine::Lexer(shaderCode);
			lexer.SetWhiteSpacesAndDelimiters(" \t\r", "\n");
			lexer.SetCursor(position);

			PrevToken = lexer.NextToken();
			CurrToken = PrevToken;

			continue;
		}

		PrevToken = CurrToken;
		CurrToken = ShaderNameToken;
	}

	return error;
}

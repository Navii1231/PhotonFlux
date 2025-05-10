#include "Wavefront/WavefrontEstimator.h"

#include "ShaderCompiler/Lexer.h"
#include "Wavefront/BVH_Factory.h"

AQUA_BEGIN

std::string GetShaderDirectory()
{
	return "../AquaFlow/Include/Shaders/";
}

AQUA_END

AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::WavefrontEstimator(
	const WavefrontEstimatorCreateInfo& createInfo)
	: mCreateInfo(createInfo)
{
	mPipelineBuilder = mCreateInfo.Context.MakePipelineBuilder();
	mMemoryManager = mCreateInfo.Context.MakeMemoryResourceManager();

	RetrieveFrontAndBackEndShaders();
	InitializePipelineContexts();
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
	MaterialCreateInfo pipelineCreation = createInfo;
	pipelineCreation.ShaderCode = StitchFrontAndBackShaders(createInfo.ShaderCode);

	MaterialPipelineContext context{};
	context.Prepare(pipelineCreation, mCreateInfo.Tolerence);

	MaterialPipeline pipeline;
	pipeline.mHandle = mPipelineBuilder.BuildComputePipeline(context);

	return pipeline;
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
	ExecutionPipelines pipelines;

	pipelines.SortRecorder = std::make_shared<SortRecorder<uint32_t>>(mPipelineBuilder, mMemoryManager);

	*pipelines.SortRecorder = *mPipelineContexts.SortRecorder;

	//mRayRefs = mPipelineResources.SortRecorder->GetBuffer();

	pipelines.RayGenerator = mPipelineBuilder.BuildComputePipeline
		<RayGenerationPipelineContext>(mPipelineContexts.RayGeneratorContext);

	pipelines.IntersectionPipeline = mPipelineBuilder.BuildComputePipeline
		<IntersectionPipelineContext>(mPipelineContexts.IntersectionContext);

	pipelines.RaySortPreparer = mPipelineBuilder.BuildComputePipeline
		<RaySortEpilogue>(mPipelineContexts.SortEpiloguePreparer);

	pipelines.RaySortFinisher = mPipelineBuilder.BuildComputePipeline
		<RaySortEpilogue>(mPipelineContexts.SortEpilogueFinisher);

	pipelines.RayRefCounter = mPipelineBuilder.BuildComputePipeline
		<RayRefCounterContext>(mPipelineContexts.RayRefCounter);

	pipelines.PrefixSummer = mPipelineBuilder.BuildComputePipeline
		<PrefixSumContext>(mPipelineContexts.PrefixSummerContext);

	pipelines.InactiveRayShader.mHandle = mPipelineBuilder.BuildComputePipeline
		<MaterialPipelineContext>(mPipelineContexts.InactiveRayShaderContext);

	pipelines.LuminanceMean = mPipelineBuilder.BuildComputePipeline
		<LuminanceMeanContext>(mPipelineContexts.LuminanceMeanCalculatorContext);

	pipelines.PostProcessor = mPipelineBuilder.BuildComputePipeline
		<PostProcessImageContext>(mPipelineContexts.PostProcessorContext);

	return pipelines;
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::InitializePipelineContexts()
{
	MaterialCreateInfo inactiveMaterialInfo{};
	inactiveMaterialInfo.PowerHeuristics = 2.0f;
	inactiveMaterialInfo.ShadingTolerence = 0.001f;
	inactiveMaterialInfo.WorkGroupSize = mCreateInfo.IntersectionWorkgroupSize;

	std::string emptyShader = "SampleInfo Evaluate(in Ray ray, in CollisionInfo collisionInfo)"
		"{ SampleInfo sampleInfo; sampleInfo.Weight = 1.0; sampleInfo.Luminance = vec3(0.0);"
		"return sampleInfo; }";

	inactiveMaterialInfo.ShaderCode = StitchFrontAndBackShaders(emptyShader);

	mPipelineContexts.SortRecorder = std::make_shared<SortRecorder<uint32_t>>(mPipelineBuilder, mMemoryManager);

	mPipelineContexts.SortRecorder->CompileSorterPipeline(mCreateInfo.IntersectionWorkgroupSize);
	mPipelineContexts.SortRecorder->ResizeBuffer(256);

	mPipelineContexts.RayGeneratorContext.Prepare(mCreateInfo.IntersectionWorkgroupSize);
	mPipelineContexts.IntersectionContext.Prepare(mCreateInfo.IntersectionWorkgroupSize, mCreateInfo.Tolerence);
	mPipelineContexts.SortEpiloguePreparer.Prepare(mCreateInfo.IntersectionWorkgroupSize, RaySortEvent::ePrepare);
	mPipelineContexts.SortEpilogueFinisher.Prepare(mCreateInfo.IntersectionWorkgroupSize, RaySortEvent::eFinish);
	mPipelineContexts.RayRefCounter.Prepare(mCreateInfo.IntersectionWorkgroupSize);
	mPipelineContexts.PrefixSummerContext.Prepare(mCreateInfo.IntersectionWorkgroupSize);
	mPipelineContexts.InactiveRayShaderContext.Prepare(inactiveMaterialInfo, mCreateInfo.Tolerence);
	mPipelineContexts.LuminanceMeanCalculatorContext.Prepare(mCreateInfo.IntersectionWorkgroupSize);
	mPipelineContexts.PostProcessorContext.Prepare(mCreateInfo.RayGenWorkgroupSize);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::CreateTraceBuffers(SessionInfo& session)
{
	vkEngine::BufferCreateInfo createInfo{};
	createInfo.Usage = vk::BufferUsageFlagBits::eStorageBuffer;
	createInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;

	session.SharedBuffers.Vertices = mMemoryManager.CreateBuffer<glm::vec4>(createInfo);
	session.SharedBuffers.Faces = mMemoryManager.CreateBuffer<Face>(createInfo);
	session.SharedBuffers.TexCoords = mMemoryManager.CreateBuffer<glm::vec2>(createInfo);
	session.SharedBuffers.Normals = mMemoryManager.CreateBuffer<glm::vec4>(createInfo);
	session.SharedBuffers.Nodes = mMemoryManager.CreateBuffer<Node>(createInfo);

	session.MeshInfos = mMemoryManager.CreateBuffer<MeshInfo>(createInfo);
	session.LightInfos = mMemoryManager.CreateBuffer<LightInfo>(createInfo);
	session.LightPropsInfos = mMemoryManager.CreateBuffer<LightProperties>(createInfo);

	createInfo.MemProps = vk::MemoryPropertyFlagBits::eDeviceLocal;

	session.LocalBuffers.Vertices = mMemoryManager.CreateBuffer<glm::vec4>(createInfo);
	session.LocalBuffers.Faces = mMemoryManager.CreateBuffer<Face>(createInfo);
	session.LocalBuffers.TexCoords = mMemoryManager.CreateBuffer<glm::vec2>(createInfo);
	session.LocalBuffers.Normals = mMemoryManager.CreateBuffer<glm::vec4>(createInfo);
	session.LocalBuffers.Nodes = mMemoryManager.CreateBuffer<Node>(createInfo);

	// SceneInfo and physical camera buffer is a uniform and should be host coherent...
	createInfo.Usage = vk::BufferUsageFlagBits::eUniformBuffer;
	createInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;

	// Intersection stage...
	session.CameraSpecsBuffer = mMemoryManager.CreateBuffer<PhysicalCamera>(createInfo);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::WavefrontEstimator::CreateExecutorBuffers(
	ExecutionInfo& executionInfo, const ExecutorCreateInfo& executorInfo)
{
	// TODO: Making them host coherent for now... But they really should be
	// local buffers in the final build...

	executionInfo.PipelineResources.SortRecorder->ResizeBuffer(2 * executorInfo.TileSize.x * executorInfo.TileSize.y);
	executionInfo.RayRefs = executionInfo.PipelineResources.SortRecorder->GetBuffer();

	vkEngine::BufferCreateInfo createInfo{};
	createInfo.Usage = vk::BufferUsageFlagBits::eStorageBuffer;
	createInfo.MemProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
	//createInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;
	// TODO: --^ Not necessary, in fact bad for performance

	executionInfo.Rays = mMemoryManager.CreateBuffer<Ray>(createInfo);
	executionInfo.RayInfos = mMemoryManager.CreateBuffer<RayInfo>(createInfo);
	executionInfo.CollisionInfos = mMemoryManager.CreateBuffer<CollisionInfo>(createInfo);

	executionInfo.RefCounts = mMemoryManager.CreateBuffer<uint32_t>(createInfo);

	uint32_t RayCount = executorInfo.TargetResolution.x * executorInfo.TargetResolution.y;

	executionInfo.Rays.Resize(2 * RayCount);
	executionInfo.RayInfos.Resize(2 * RayCount);
	executionInfo.CollisionInfos.Resize(2 * RayCount);

	createInfo.Usage = vk::BufferUsageFlagBits::eUniformBuffer;
	createInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;

	executionInfo.Scene = mMemoryManager.CreateBuffer<WavefrontSceneInfo>(createInfo);
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

	executionInfo.Target.PixelMean = mMemoryManager.CreateImage(imageInfo);
	executionInfo.Target.PixelVariance = mMemoryManager.CreateImage(imageInfo);

	imageInfo.Format = vk::Format::eR8G8B8A8Snorm;
	executionInfo.Target.Presentable = mMemoryManager.CreateImage(imageInfo);

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
	AddText(mShaderFrontEnd, ::AQUA_NAMESPACE::GetShaderDirectory() + "Wavefront/Common.glsl");
	AddText(mShaderFrontEnd, ::AQUA_NAMESPACE::GetShaderDirectory() + "BSDFs/CommonBSDF.glsl");
	AddText(mShaderFrontEnd, ::AQUA_NAMESPACE::GetShaderDirectory() + "BSDFs/BSDF_Samplers.glsl");
	AddText(mShaderFrontEnd, ::AQUA_NAMESPACE::GetShaderDirectory() + "MaterialShaders/ShaderFrontEnd.glsl");
	AddText(mShaderFrontEnd, ::AQUA_NAMESPACE::GetShaderDirectory() + "BSDFs/Utils.glsl");

	/* TODO: This is temporary, should be dealt by an import system */

#if 0
	AddText(mShaderFrontEnd, "Shaders/BSDFs/DiffuseBSDF.glsl");
	AddText(mShaderFrontEnd, "Shaders/BSDFs/GlossyBSDF.glsl");
	AddText(mShaderFrontEnd, "Shaders/BSDFs/RefractionBSDF.glsl");
	AddText(mShaderFrontEnd, "Shaders/BSDFs/CookTorranceBSDF.glsl");
	AddText(mShaderFrontEnd, "Shaders/BSDFs/GlassBSDF.glsl");
#else
	AddText(mImportToShaders["DiffuseBSDF"], ::AQUA_NAMESPACE::GetShaderDirectory() + "BSDFs/DiffuseBSDF.glsl");
	AddText(mImportToShaders["GlossyBSDF"], ::AQUA_NAMESPACE::GetShaderDirectory() + "BSDFs/GlossyBSDF.glsl");
	AddText(mImportToShaders["RefractionBSDF"], ::AQUA_NAMESPACE::GetShaderDirectory() + "BSDFs/RefractionBSDF.glsl");
	AddText(mImportToShaders["CookTorranceBSDF"], ::AQUA_NAMESPACE::GetShaderDirectory() + "BSDFs/CookTorranceBSDF.glsl");
	AddText(mImportToShaders["GlassBSDF"], ::AQUA_NAMESPACE::GetShaderDirectory() + "BSDFs/GlassBSDF.glsl");
#endif

	/****************************************************************/

	// Backend of the pipelines handling luminance calculations
	AddText(mShaderBackEnd, ::AQUA_NAMESPACE::GetShaderDirectory() + "MaterialShaders/ShaderBackEnd.glsl");
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

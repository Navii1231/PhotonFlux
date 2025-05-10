#pragma once
#include "Executor.h"

#include "SortRecorder.h"
#include "RayGenerationPipeline.h"
#include "WavefrontWorkflow.h"

#include "../Geometry3D/GeometryConfig.h"

#include <random>

AQUA_BEGIN
PH_BEGIN

// Thread safe...
class WavefrontEstimator
{
public:
	WavefrontEstimator(const WavefrontEstimatorCreateInfo& createInfo);

	WavefrontEstimator(const WavefrontEstimator&) = delete;
	WavefrontEstimator& operator=(const WavefrontEstimator&) = delete;

	TraceSession CreateTraceSession();
	Executor CreateExecutor(const ExecutorCreateInfo& createInfo);

	MaterialPipeline CreateMaterialPipeline(const MaterialCreateInfo& createInfo);

	static std::string GetShaderDirectory();

private:
	// Resources...
	vkEngine::PipelineBuilder mPipelineBuilder;
	vkEngine::MemoryResourceManager mMemoryManager;

	ExecutionPipelineContexts mPipelineContexts;
	
	std::string mShaderFrontEnd;
	std::string mShaderBackEnd;

	std::unordered_map<std::string, std::string> mImportToShaders;

	// Wavefront properties...
	WavefrontEstimatorCreateInfo mCreateInfo;

private:
	// Helpers...
	ExecutionPipelines CreatePipelines();
	void InitializePipelineContexts();

	void CreateTraceBuffers(SessionInfo& session);
	void CreateExecutorBuffers(ExecutionInfo& mExecutionInfo, const ExecutorCreateInfo& executorInfo);
	void CreateExecutorImages(ExecutionInfo& executionInfo, const ExecutorCreateInfo& executorInfo);

	void AddText(std::string& text, const std::string& filepath);
	void RetrieveFrontAndBackEndShaders();
	std::string StitchFrontAndBackShaders(const std::string& shaderCode);

	MaterialShaderError ImportShaders(std::string& shaderCode);
};

PH_END
AQUA_END

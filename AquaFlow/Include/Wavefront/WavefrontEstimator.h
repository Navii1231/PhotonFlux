#pragma once
#include "Executor.h"

#include "SortRecorder.h"
#include "RayGenerationPipeline.h"
#include "WavefrontWorkflow.h"

#include "../Geometry3D/GeometryConfig.h"

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

	static std::string GetShaderDirectory() { return "../AquaFlow/Include/Shaders/"; }

private:
	// Resources...
	vkEngine::PipelineBuilder mPipelineBuilder;
	vkEngine::ResourcePool mResourcePool;

	std::shared_ptr<RaySortRecorder> mSortRecorder;
	
	std::string mShaderFrontEnd;
	std::string mShaderBackEnd;

	std::unordered_map<std::string, std::string> mImportToShaders;

	// Wavefront properties...
	WavefrontEstimatorCreateInfo mCreateInfo;

private:
	// Helpers...
	ExecutionPipelines CreatePipelines();

	void CreateTraceBuffers(SessionInfo& session);
	void CreateExecutorBuffers(ExecutionInfo& mExecutionInfo, const ExecutorCreateInfo& executorInfo);
	void CreateExecutorImages(ExecutionInfo& executionInfo, const ExecutorCreateInfo& executorInfo);

	void AddText(std::string& text, const std::string& filepath);
	void RetrieveFrontAndBackEndShaders();
	std::string StitchFrontAndBackShaders(const std::string& shaderCode);

	MaterialShaderError ImportShaders(std::string& shaderCode);

	vkEngine::PShader GetRayGenerationShader();
	vkEngine::PShader GetIntersectionShader();
	vkEngine::PShader GetRaySortEpilogueShader(RaySortEvent sortEvent);
	vkEngine::PShader GetRayRefCounterShader();
	vkEngine::PShader GetPrefixSumShader();
	vkEngine::PShader GetLuminanceMeanShader();
	vkEngine::PShader GetPostProcessImageShader();
};

PH_END
AQUA_END

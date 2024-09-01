#pragma once
#include "ComputeSettings.h"
#include "VisualizerSettings.h"

#include <filesystem>

PH_BEGIN

// Hardware accelerated ray tracing support will be added soon

using ComputeEstimatorPipeline = vkEngine::ComputePipeline<PH_FLUX_NAMESPACE::ComputePipelineSettings>;

using BVH_VisualizerPipeline = vkEngine::GraphicsPipeline<PH_FLUX_NAMESPACE::BVH_VisualizerSettings>;
using SampleVisualizerPipeline = vkEngine::GraphicsPipeline<PH_FLUX_NAMESPACE::SampleVisualizerSettings>;
using NodeVisualizerPipeline = vkEngine::GraphicsPipeline<PH_FLUX_NAMESPACE::NodeVisualizerSettings>;

class ComputeEstimator
{
public:
	ComputeEstimator(const EstimatorCreateInfo& createInfo);

	void Begin(const TraceInfo& beginInfo);
	void SubmitRenderable(const MeshData& data, const Material& material, RenderableType type);
	void End();

	TraceResult Trace(vk::CommandBuffer buffer);

	void SetCamera(const Camera& camera);

	vkEngine::Image GetTargetImage() const { return mPresentable; }
	glm::ivec2 GetImageResolution() const { return mPresentableSize; }

private:
	// Resources...
	vkEngine::Device mDevice;
	vkEngine::PipelineBuilder mPipelineBuilder;
	vkEngine::MemoryResourceManager mMemoryManager;

	// Tiles...
	std::vector<Tile> mTiles;
	std::vector<vkEngine::Image> mTileImages;
	size_t mTileIndex;

	vkEngine::Image mCameraView;
	vkEngine::Image mPresentable;
	
	glm::ivec2 mTileSize;
	glm::ivec2 mPresentableSize;

	// Pipeline...
	ComputeEstimatorPipeline mEstimator;

	// Pipeline Settings...
	std::shared_ptr<ComputePipelineSettings> mSettings;
	std::filesystem::path mShaderDirectory;

	// Runtime info...
	TraceInfo mTraceInfo;

	bool mCameraSet = false;

private:
	void CheckErrors(const std::vector<vkEngine::CompileError>& Errors);
	void GenerateTiles(const glm::ivec2& tileSize);
	void CreateImageViews(const std::vector<Tile>& tiles);
};

PH_END

#pragma once
#include "ComputePipeline.h"

#include <filesystem>

PH_BEGIN

// Hardware accelerated ray tracing support will be added soon

class ComputeEstimator
{
public:
	ComputeEstimator(const EstimatorCreateInfo& createInfo);

	void Begin(const TraceInfo& beginInfo);
	void SubmitRenderable(const AquaFlow::MeshData& data, const Material& material, RenderableType type);
	void End();

	TraceResult Trace(vk::CommandBuffer buffer);

	void SetCamera(const Camera& camera);

	vkEngine::Image GetTargetImage() const { return mPresentable; }
	glm::ivec2 GetImageResolution() const { return mPresentableSize; }

private:
	// Resources...
	vkEngine::Context mCtx;
	vkEngine::PipelineBuilder mPipelineBuilder;
	vkEngine::ResourcePool mMemoryManager;

	// Tiles...
	std::vector<Tile> mTiles;
	std::vector<vkEngine::Image> mTileImages;
	size_t mTileIndex;

	vkEngine::Image mCameraView;
	vkEngine::Image mPresentable;
	
	glm::ivec2 mTileSize;
	glm::ivec2 mPresentableSize;

	// Pipeline...
	EstimatorComputePipeline mEstimator;

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

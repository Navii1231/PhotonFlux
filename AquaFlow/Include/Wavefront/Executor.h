#pragma once
#include "ExecutorConfig.h"

AQUA_BEGIN
PH_BEGIN

class Executor
{
public:
	Executor();

	TraceResult Trace(vk::CommandBuffer commandBuffer);

	void SetTraceSession(const TraceSession& traceSession);

	template<typename Iter>
	void SetMaterialPipelines(Iter Begin, Iter End);

	void SetSortingFlag(bool allowSort)
	{ mExecutorInfo->CreateInfo.AllowSorting = allowSort; }

	void SetCameraView(const glm::mat4& cameraView);

	// Getters...
	TraceSession GetTraceSession() const { return mExecutorInfo->TracingSession; }

	glm::ivec2 GetTargetResolution() const { return mExecutorInfo->Target.ImageResolution; }
	vkEngine::Image GetPresentable() const { return mExecutorInfo->Target.Presentable; }
	vkEngine::Buffer<WavefrontSceneInfo> GetSceneInfo() const { return mExecutorInfo->Scene; }

	// For debugging...
	RayBuffer GetRayBuffer() const { return mExecutorInfo->Rays; }
	CollisionInfoBuffer GetCollisionBuffer() const { return mExecutorInfo->CollisionInfos; }
	RayRefBuffer GetRayRefBuffer() const { return mExecutorInfo->RayRefs; }
	RayInfoBuffer GetRayInfoBuffer() const { return mExecutorInfo->RayInfos; }
	vkEngine::Buffer<uint32_t> GetMaterialRefCounts() const { return mExecutorInfo->RefCounts; }
	vkEngine::Image GetVariance() const { return mExecutorInfo->Target.PixelVariance; }
	vkEngine::Image GetMean() const { return mExecutorInfo->Target.PixelMean; }

private:
	std::shared_ptr<ExecutionInfo> mExecutorInfo;

private:
	Executor(const ExecutorCreateInfo& createInfo);

	uint32_t GetRandomNumber();

	void ExecuteRayGenerator(vk::CommandBuffer commandBuffer, uint32_t pActiveBuffer, glm::uvec3 workGroups);
	void ExecuteRaySortFinisher(vk::CommandBuffer commandBuffer, 
		uint32_t pRayCount, uint32_t pActiveBuffer, uint32_t pRayRefBuffer, glm::uvec3 workGroups);

	void ExecutePrefixSummer(vk::CommandBuffer commandBuffer, uint32_t pMaterialCount);
	void ExecuteRayCounter(vk::CommandBuffer commandBuffer, 
		uint32_t pRayCount, uint32_t pMaterialCount, glm::uvec3 workGroups);

	void ExecuteRaySortPreparer(vk::CommandBuffer commandBuffer, 
		uint32_t pRayCount, uint32_t pActiveBuffer, glm::uvec3 workGroups);

	void ExecuteIntersectionTester(vk::CommandBuffer commandBuffer, 
		uint32_t pRayCount, uint32_t pActiveBuffer, glm::uvec3 workGroups);

	void RecordLuminanceMean(vk::CommandBuffer commandBuffer, uint32_t pRayCount, uint32_t pActiveBuffer, uint32_t intersectionWorkgroups);

	void RecordPostProcess(vk::CommandBuffer commandBuffer, PostProcessFlags postProcess, glm::uvec3 workGroups);

	void UpdateSceneInfo();

	void InvalidateMaterialData();

	void RecordMaterialPipelines(vk::CommandBuffer commandBuffer,
		uint32_t pRayCount, uint32_t pBounceIdx, uint32_t pActiveBuffer, glm::uvec3 workGroups);

	void UpdateMaterialDescriptors();

	friend class WavefrontEstimator;
};

template<typename Iter>
inline void Executor::SetMaterialPipelines(Iter Begin, Iter End)
{
	mExecutorInfo->MaterialResources.clear();
	
	for (; Begin != End; Begin++)
	{
		mExecutorInfo->MaterialResources.emplace_back(*Begin);

		// For optimizations...
		if (mExecutorInfo->MaterialResources.size() == mExecutorInfo->MaterialResources.capacity())
			mExecutorInfo->MaterialResources.reserve(2 * mExecutorInfo->MaterialResources.size());
	}

	mExecutorInfo->RefCounts.Resize(glm::max(static_cast<uint32_t>(mExecutorInfo->MaterialResources.size()
		+ 2), static_cast<uint32_t>(32)));

	InvalidateMaterialData();
}

PH_END
AQUA_END

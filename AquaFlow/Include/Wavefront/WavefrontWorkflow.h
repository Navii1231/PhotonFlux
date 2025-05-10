#pragma once
#include "RayTracingStructures.h"

#include "../Utils/CompilerErrorChecker.h"
#include "MergeSorterPipeline.h"

AQUA_BEGIN
PH_BEGIN

using RayRef = typename MergeSorterPassContext<uint32_t>::ArrayRef;
using RayRefBuffer = vkEngine::Buffer<RayRef>;

enum class RaySortEvent
{
	ePrepare                    = 1,
	eFinish                     = 2
};

enum class PostProcessFlagBits
{
	eToneMap                    = 1,
	eGammaCorrection            = 2,
	eGammaCorrectionInv         = 4,
};

using PostProcessFlags = vk::Flags<PostProcessFlagBits>;

struct IntersectionPipelineContext : public vkEngine::ComputePipelineContext
{
	IntersectionPipelineContext() = default;

	void Prepare(uint32_t workGroupSize, float Tolerence);

	GeometryBuffers GetGeometryBuffers() const { return mGeometryBuffers; }

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override;

// Fields...
	RayBuffer mRays;

	GeometryBuffers mGeometryBuffers;
	CollisionInfoBuffer mCollisionInfos;

	MeshInfoBuffer mMeshInfos;
	LightInfoBuffer mLightInfos;
	LightPropsBuffer mLightProps;

	vkEngine::Buffer<WavefrontSceneInfo> mSceneInfo;

private:
	inline void UpdateGeometryBuffers(vkEngine::DescriptorWriter& writer);
};

struct RaySortEpilogue : public vkEngine::ComputePipelineContext
{
	RaySortEpilogue() = default;

	void Prepare(uint32_t workGroupSize, RaySortEvent sortEvent);

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override;

// Fields...
	RayBuffer mRays;
	RayInfoBuffer mRaysInfos;
	CollisionInfoBuffer mCollisionInfos;

	RayRefBuffer mRayRefs;

	RaySortEvent mSortingEvent = RaySortEvent::ePrepare;

private:
	std::string GetFilePath(RaySortEvent sortEvent);
};

struct RayRefCounterContext : public vkEngine::ComputePipelineContext
{
	RayRefCounterContext() = default;
	
	void Prepare(uint32_t workGroupSize);

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override;

// Fields...
	RayRefBuffer mRayRefs;
	vkEngine::Buffer<uint32_t> mRefCounts;
};

// TODO: Make a proper Prefix summer
struct PrefixSumContext : public vkEngine::ComputePipelineContext
{
	PrefixSumContext() = default;

	void Prepare(uint32_t workGroupSize);

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override;

// Fields...
	vkEngine::Buffer<uint32_t> mRefCounts;
};

struct LuminanceMeanContext : public vkEngine::ComputePipelineContext
{
	LuminanceMeanContext() = default;

	void Prepare(uint32_t workGroupSize);

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override;

	vkEngine::Image mPixelMean;
	vkEngine::Image mPixelVariance;
	vkEngine::Image mPresentable;

	RayBuffer mRays;
	RayInfoBuffer mRayInfos;
	vkEngine::Buffer<WavefrontSceneInfo> mSceneInfo;
};

struct PostProcessImageContext : public vkEngine::ComputePipelineContext
{
	PostProcessImageContext() = default;

	void Prepare(const glm::ivec2& workGroupSize);

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override;

	vkEngine::Image mPresentable;
};

using RaySortEpiloguePipeline = vkEngine::ComputePipeline<RaySortEpilogue>;
using IntersectionPipeline = vkEngine::ComputePipeline<IntersectionPipelineContext>;
using RayRefCounterPipeline = vkEngine::ComputePipeline<RayRefCounterContext>;
using PrefixSumPipeline = vkEngine::ComputePipeline<PrefixSumContext>;
using LuminanceMeanPipeline = vkEngine::ComputePipeline<LuminanceMeanContext>;
using PostProcessImagePipeline = vkEngine::ComputePipeline<PostProcessImageContext>;

PH_END
AQUA_END

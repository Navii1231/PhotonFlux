#pragma once
#include "RayTracingStructures.h"
#include "../Utils/CompilerErrorChecker.h"

#include "MergeSorterPipeline.h"

AQUA_BEGIN
PH_BEGIN

using RayRef = typename MergeSorterPass<uint32_t>::ArrayRef;
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

struct IntersectionPipeline : public vkEngine::ComputePipeline
{
	IntersectionPipeline() = default;
	IntersectionPipeline(const vkEngine::PShader& shader) { this->SetShader(shader); }

	GeometryBuffers GetGeometryBuffers() const { return mGeometryBuffers; }

	virtual void UpdateDescriptors() override;

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

struct RaySortEpiloguePipeline : public vkEngine::ComputePipeline
{
	RaySortEpiloguePipeline() = default;
	RaySortEpiloguePipeline(const vkEngine::PShader& shader) { this->SetShader(shader); }

	virtual void UpdateDescriptors() override;

// Fields...
	RayBuffer mRays;
	RayInfoBuffer mRaysInfos;
	CollisionInfoBuffer mCollisionInfos;

	RayRefBuffer mRayRefs;

	RaySortEvent mSortingEvent = RaySortEvent::ePrepare;

};

struct RayRefCounterPipeline : public vkEngine::ComputePipeline
{
	RayRefCounterPipeline() = default;
	RayRefCounterPipeline(const vkEngine::PShader& shader) { this->SetShader(shader); }
	
	virtual void UpdateDescriptors() override;

// Fields...
	RayRefBuffer mRayRefs;
	vkEngine::Buffer<uint32_t> mRefCounts;
};

// TODO: Make a proper Prefix summer
struct PrefixSumPipeline : public vkEngine::ComputePipeline
{
	PrefixSumPipeline() = default;
	PrefixSumPipeline(const vkEngine::PShader& shader) { this->SetShader(shader); }

	virtual void UpdateDescriptors() override;

// Fields...
	vkEngine::Buffer<uint32_t> mRefCounts;
};

struct LuminanceMeanPipeline : public vkEngine::ComputePipeline
{
	LuminanceMeanPipeline() = default;
	LuminanceMeanPipeline(const vkEngine::PShader& shader) { this->SetShader(shader); }

	virtual void UpdateDescriptors() override;

	vkEngine::Image mPixelMean;
	vkEngine::Image mPixelVariance;
	vkEngine::Image mPresentable;

	RayBuffer mRays;
	RayInfoBuffer mRayInfos;
	vkEngine::Buffer<WavefrontSceneInfo> mSceneInfo;
};

struct PostProcessImagePipeline : public vkEngine::ComputePipeline
{
	PostProcessImagePipeline() = default;
	PostProcessImagePipeline(const vkEngine::PShader& shader) { this->SetShader(shader); }

	virtual void UpdateDescriptors() override;

	vkEngine::Image mPresentable;
};

PH_END
AQUA_END

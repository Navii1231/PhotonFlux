#pragma once
#include "RayTracingStructures.h"

AQUA_BEGIN
PH_BEGIN

struct RayGenerationPipelineContext : public vkEngine::ComputePipelineContext
{
	RayGenerationPipelineContext() {}

	void Prepare(uint32_t workGroupSize);

	void SetSceneInfo(const WavefrontSceneInfo& sceneInfo);
	void SetCamera(const PhysicalCamera& camera);

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer);

	// Fields...
	//Buffers
	RayBuffer mRays;
	RayInfoBuffer mRayInfos;

	// Uniforms
	vkEngine::Buffer<PhysicalCamera> mCamera;
	vkEngine::Buffer<WavefrontSceneInfo> mSceneInfo;

	bool mCameraUpdated = false;
};

using RayGenerationPipeline = vkEngine::ComputePipeline<RayGenerationPipelineContext>;

PH_END
AQUA_END

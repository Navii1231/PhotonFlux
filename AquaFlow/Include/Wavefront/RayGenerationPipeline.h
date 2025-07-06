#pragma once
#include "RayTracingStructures.h"
#include "../Utils/CompilerErrorChecker.h"

AQUA_BEGIN
PH_BEGIN

// change required

struct RayGenerationPipeline : public vkEngine::ComputePipeline
{
	RayGenerationPipeline() = default;
	RayGenerationPipeline(const vkEngine::PShader& shader) { this->SetShader(shader); }

	void SetSceneInfo(const WavefrontSceneInfo& sceneInfo);
	void SetCamera(const PhysicalCamera& camera);

	virtual void UpdateDescriptors();

	

	RayBuffer GetRays() const { return mRays; }
	RayInfoBuffer GetRayInfos() const { return mRayInfos; }

	// Fields...
	//Buffers
	RayBuffer mRays;
	RayInfoBuffer mRayInfos;

	// Uniforms
	vkEngine::Buffer<PhysicalCamera> mCamera;
	vkEngine::Buffer<WavefrontSceneInfo> mSceneInfo;

	bool mCameraUpdated = false;
};

PH_END
AQUA_END

#pragma once
#include "../Application/Application.h"
#include "SampleGenerationTester.h"
#include "../PhotonFlux/ComputeEstimator.h"
#include "Wavefront/LocalRadixSortPipeline.h"
#include "Wavefront/SortRecorder.h"

#include "Wavefront/MergeSorterPipeline.h"

#include "Utils/EditorCamera.h"

#include "Wavefront/RayGenerationPipeline.h"
#include "Wavefront/WavefrontEstimator.h"

// Vulkan image loading stuff...
#include "Utils/STB_Image.h"

class ComputePipelineTester : public Application
{
public:
	ComputePipelineTester(const ApplicationCreateInfo& createInfo)
		: Application(createInfo) {}

	virtual bool OnStart() override;
	virtual bool OnUpdate(float ElaspedTime) override;

private:
	// Pipelines...
	vkEngine::PipelineBuilder mPipelineBuilder;

	AquaFlow::PhFlux::LocalRadixSortPipeline mPrefixSummer;

	// Executor...
	vkEngine::Core::Executor mComputeWorker;
	vkEngine::Core::Executor mGraphicsWorker;

	// Resource allocators...
	vkEngine::ResourcePool mResourcePool;

	vkEngine::RenderContextBuilder mRenderContextBuilder;
	vkEngine::RenderTargetContext mRenderContext;
	vkEngine::Framebuffer mRenderTarget;

	PhFlux::CameraData mCameraData;

	AquaFlow::EditorCamera mVisualizerCamera;

	PhFlux::TraceResult mComputeTraceResult;
	AquaFlow::PhFlux::TraceResult mTraceResult;

	// Monte Carlo estimator...
	std::shared_ptr<PhFlux::ComputeEstimator> mEstimator;

	std::vector<glm::vec3> mRaySamples;

	std::shared_ptr<vkEngine::Swapchain> mSwapchain;

	float mCurrTimeStep = 0.0f;

	// Wavefront path tracer

	AquaFlow::EditorCamera mEditorCamera;
	std::shared_ptr<AquaFlow::PhFlux::WavefrontEstimator> mWavefrontEstimator;

	AquaFlow::PhFlux::TraceSession mTraceSession;
	AquaFlow::PhFlux::Executor mExecutor;

	// For debugging...

	// Material pipelines...
	AquaFlow::PhFlux::MaterialPipeline mDiffuseMaterial;
	AquaFlow::PhFlux::MaterialPipeline mRefractionMaterial;
	AquaFlow::PhFlux::MaterialPipeline mGlossyMaterial;
	AquaFlow::PhFlux::MaterialPipeline mCookTorranceMaterial;
	AquaFlow::PhFlux::MaterialPipeline mGlassMaterial;

	std::vector<AquaFlow::PhFlux::Ray> mHostRayBuffer;
	std::vector<AquaFlow::PhFlux::CollisionInfo> mHostCollisionInfoBuffer;
	std::vector<AquaFlow::PhFlux::RayRef> mHostRayRefBuffer;
	std::vector<AquaFlow::PhFlux::RayInfo> mHostRayInfoBuffer;
	std::vector<uint32_t> mHostRefCounts;

	bool UpdateCamera(AquaFlow::EditorCamera& camera);

private:
	void RayTrace(vk::CommandBuffer commandBuffer, vkEngine::Framebuffer renderTarget);
	void RayTracerWithWavefront(vk::CommandBuffer commandBuffer, vkEngine::Framebuffer renderTarget);


	void PresentScreen(uint32_t FrameIndex, vkEngine::SwapchainFrame& ActiveFrame);

	void CheckErrors(const std::vector<vkEngine::CompileError>& Errors);

	AquaFlow::CameraMovementFlags GetMovementFlags() const;

	void SetupCameras();

	// Scene setup...
	void SetScene();
	void SetSceneWavefront();

	void CreateMaterialPipelines();

	// For wavefront debugging
	void FillWavefrontHostBuffers();

	void PrintWavefrontRayHostBuffer();
	void PrintWavefrontCollisionInfoHostBuffer();
	void PrintWavefrontRayRefHostBuffer();
	void PrintWavefrontMaterialRefCountHostBuffer();
	void PrintWavefrontRayInfoHostBuffer();
	void TestNodeSystem();
};

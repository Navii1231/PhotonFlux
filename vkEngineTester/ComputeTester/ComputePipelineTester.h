#pragma once
#include "../Application/Application.h"
#include "../GraphicsTester/FlatMaterialPipeline.h"
#include "SampleGenerationTester.h"
#include "../PhotonFlux/ComputeEstimator.h"

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

	// Executor...
	std::shared_ptr<const vkEngine::QueueManager> mQueueManager;
	vkEngine::Core::Executor mComputeWorker;
	vkEngine::Core::Executor mGraphicsWorker;

	// Resource allocators...
	vkEngine::MemoryResourceManager mMemoryResourceManager;

	vkEngine::RenderContextBuilder mRenderContextBuilder;
	vkEngine::RenderContext mRenderContext;
	vkEngine::Framebuffer mRenderTarget;

	PhFlux::CameraData mCameraData;

	// Scene elements...
	Geometry3D mCube;
	Geometry3D mCup;

	PhFlux::TraceResult mTraceResult;

	// Monte Carlo estimator...
	std::shared_ptr<PhFlux::ComputeEstimator> mEstimator;

	std::vector<glm::vec3> mRaySamples;

	std::shared_ptr<vkEngine::Swapchain> mSwapchain;

	float mCurrTimeStep = 0.0f;

	void MoveCamera();
	bool UpdateCamera(float elaspedTime);
private:
	void VisualizeBVH(vkEngine::SwapchainFrame& ActiveFrame, const vkEngine::SwapchainData& Data);
	void VisualizeMesh(vkEngine::SwapchainFrame& ActiveFrame, const vkEngine::SwapchainData& Data);
	void VisualizeSamples(vkEngine::SwapchainFrame& ActiveFrame, const vkEngine::SwapchainData& Data);

	void RayTrace(vkEngine::SwapchainFrame& ActiveFrame, vkEngine::SwapchainData& Data);
	void PresentScreen(uint32_t FrameIndex, vkEngine::SwapchainFrame& ActiveFrame);

	void CheckErrors(const std::vector<vkEngine::CompileError>& Errors);

	// Scene setup...
	void SetScene();
};

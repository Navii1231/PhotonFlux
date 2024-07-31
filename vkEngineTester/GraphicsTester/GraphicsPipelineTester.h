#pragma once
#include "GraphicsPipelineSettings.h"

class GraphicsPipelineTester : public Application
{
public:
	GraphicsPipelineTester(const ApplicationCreateInfo& createInfo)
		: Application(createInfo) {}

	virtual bool OnStart() override;
	virtual bool OnUpdate(float ElaspedTime) override;

private:
	// Graphics pipeline stuff...
	vkEngine::PipelineBuilder mPipelineBuilder;
	vkEngine::RenderContextBuilder mContextBuilder;
	vkEngine::GraphicsPipeline<MyPipelineConfig> mPipeline;
	vkEngine::RenderContext mContext;

	// Executor...
	std::shared_ptr<const vkEngine::QueueManager> mQueueManager;
	vkEngine::Core::Executor mGraphicsWorker;

	// Resource allocators...
	vkEngine::MemoryResourceManager mMemoryResourceManager;

	// Custom renderable...
	std::pair<std::vector<Vertex>, std::vector<uint32_t>> mQuad;
	std::pair<std::vector<Vertex>, std::vector<uint32_t>> mQuad1;

private:
	void PresentScreen(uint32_t FrameIndex, vkEngine::SwapchainFrame& ActiveFrame);

	uint32_t Render(vkEngine::SwapchainFrame& ActiveFrame, vkEngine::SwapchainData& Data);
};

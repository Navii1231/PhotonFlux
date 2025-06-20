#pragma once
#include "../Application/Application.h"

#include "DeferredRenderer/Pipelines/DeferredPipeline.h"
#include "DeferredRenderer/Pipelines/ShadingPipeline.h"

struct PipelineResources
{
	vkEngine::Framebuffer mGBuffer;
	vkEngine::Framebuffer mShadingBuffer;

	AquaFlow::RendererGeometry mGeometry;
};

class DeferredPipelinesTester : public Application
{
public:
	DeferredPipelinesTester(const ApplicationCreateInfo& createInfo);

	~DeferredPipelinesTester() = default;

	virtual bool OnStart() override;
	virtual bool OnUpdate(float ElaspedTime) override;

private:
	AquaFlow::GBufferPipeline mGBufferPipeline;
	AquaFlow::ShadingPipeline mShadingPipeline;

	PipelineResources mPipelineResources;

	std::vector<AquaFlow::DeferredRenderable> mRenderables;

	std::string mShaderDir = "D:\\Dev\\VulkanEngine\\AquaFlow\\Include\\DeferredRenderer\\Shaders\\";

private:
	void CreatePipelines();
	void CreateResources();
};

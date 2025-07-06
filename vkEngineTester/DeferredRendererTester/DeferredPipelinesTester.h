#pragma once
#include "../Application/Application.h"

#include "DeferredRenderer/Pipelines/DeferredPipeline.h"
#include "DeferredRenderer/Pipelines/ShadingPipeline.h"
#include "Utils/EditorCamera.h"

struct PipelineResources
{
	vkEngine::Framebuffer mGBuffer;
	vkEngine::Framebuffer mShadingBuffer;
};

class DeferredPipelinesTester : public Application
{
public:
	DeferredPipelinesTester(const ApplicationCreateInfo& createInfo);

	~DeferredPipelinesTester() = default;

	virtual bool OnStart() override;
	virtual bool OnUpdate(float ElaspedTime) override;

private:
	AquaFlow::DeferredPipeline mDeferPipeline;
	AquaFlow::ShadingPipeline mShadingPipeline;

	vkEngine::GenericBuffer mMaterials;
	vkEngine::GenericBuffer mDirLightSrcs;

	PipelineResources mPipelineResources;

	AquaFlow::EditorCamera mEditorCamera;

	std::vector<AquaFlow::DeferredRenderable> mRenderables;

	vkEngine::ResourcePool mMemoryManager;
	vkEngine::CommandBufferAllocator mCmdAlloc;
	vkEngine::Core::Executor mExec;

	std::string mShaderDir = "D:\\Dev\\VulkanEngine\\AquaFlow\\Include\\DeferredRenderer\\Shaders\\";

private:
	void CreatePipelines();
	void CreateResources();
};

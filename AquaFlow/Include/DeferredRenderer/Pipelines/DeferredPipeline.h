#pragma once
#include "PipelineConfig.h"
#include "../Renderable/DeferredRenderable.h"
#include "../Renderable/CopyIndices.h"
#include "../Renderable/RenderableBuilder.h"

AQUA_BEGIN

/* --> For most optimal rendering, we could
* Give each deferred renderable a vertex and index buffer instance and a compute pipeline
* When submitting a renderable to graphics pipeline, all vertices and indices can be
* copied entirely through the compute pipeline, therefore avoiding any CPU work
*/

/*
* --> There are three ways we can implement this thing
* --> We're currently utilizing the first one
* First method: to copy the vertex attributes at each frame
*	-- more efficient for frequently removing and adding stuff
* Second method: to copy the whole scene at once and copy from CPU only when the scene changes
*	-- more efficient when no frequent large meshes are added or removed
* Third method: use compute shaders to copy from one buffer to another
*	-- seems like a nice middle ground
*/

// It turns out that the vkEngine::GraphicsPipelineCtx<...>, the example implementation of
// Graphics Context Type in the vkEngine library, is way too restrictive for generic use 
// of Geometry Buffer Construction. Therefore, a better and more expressive implementation is needed

using DeferredRenderable = Renderable;

struct DeferredPipelineCreateInfo
{
	vkEngine::Context Ctx;
	vkEngine::PShader DeferShader;
	vkEngine::PShader IdxCpyShader;
	glm::vec2 ScrSize;
};

// TODO: IndexType is uint32_t by default
struct DeferredPipelineContext
{
	DeferredPipelineContext() = default;

	template <typename Iter>
	DeferredPipelineContext(Iter tagBegin, Iter tagEnd)
		: vLayout(tagBegin, tagEnd) {}

	~DeferredPipelineContext() { Alloc.Free(Cmds); }

	VertexLayout vLayout = 
	{ TAG_POSITION, TAG_NORMAL, TAG_TANGENT, TAG_BITANGENT, 
		TAG_TEXTURE_COORDS, TAG_MATERIAL_IDS, TAG_DEPTH_STENCIL };

	vkEngine::GraphicsPipelineConfig PipelineConfig;

	// Generic buffers are more expressive than templates at runtime
	
	VertexResourceMap VertexResources;

	CopyIdxPipeline CopyIdx;

	mutable vkEngine::CommandBufferAllocator Alloc;
	mutable vkEngine::Core::Executor Exec;
	mutable vk::CommandBuffer Cmds;

	mutable size_t MeshIdx = 0;
	mutable size_t VertexCount = 0;

	// readonly stuff, so we can afford them to be host coherent
	mutable Mat4Buf SharedModels; // Local buffer
	mutable Mat4Buf Models; // Bound at (set: 0, binding: 1)

	mutable vkEngine::Buffer<CameraInfo> Camera; // Bound at (set: 0, binding: 0)
};

// What layout we should pass from the CPU, AOS or SOA?
class DeferredPipeline : public vkEngine::GraphicsPipeline<DeferredRenderable>
{
public:
	using ParentPipeline = vkEngine::GraphicsPipeline<DeferredRenderable>;

public:
	DeferredPipeline() = default;

	// using default tags
	DeferredPipeline(const DeferredPipelineCreateInfo& createInfo);

	template <typename Iter>
	DeferredPipeline(const DeferredPipelineCreateInfo& createInfo, Iter tagBegin, Iter tagEnd)
		: mVulkanCtx(createInfo.Ctx), mDeferredCtx(tagBegin, tagEnd) { SetupPipeline(createInfo); }

	virtual void UpdateDescriptors() override;

	virtual void Cleanup() override;

	virtual void SubmitRenderable(const DeferredRenderable& renderable) const override;

	virtual size_t GetVertexCount() const override
	{ return mDeferredCtx->VertexCount; }

	virtual size_t GetIndexCount() const override
	{ return GetIndexBuffer().GetSize() / sizeof(uint32_t); }

	virtual const vkEngine::GraphicsPipelineConfig& GetConfig() const { return mDeferredCtx->PipelineConfig; }

	void SetSampler(const std::string& tag, vkEngine::Core::Ref<vk::Sampler> sampler);
	void SetIndexBuffer(const vkEngine::GenericBuffer& buf) { mDeferredCtx->CopyIdx.mIdxBuf = buf; }

	void SetCamera(const glm::mat4& projection, const glm::mat4& view)
	{ mDeferredCtx->Camera.Clear(); mDeferredCtx->Camera << CameraInfo(projection, view); }

	vkEngine::Buffer<CameraInfo> GetCamera() const { return mDeferredCtx->Camera; }

	vkEngine::GenericBuffer GetIndexBuffer() const { return mDeferredCtx->CopyIdx.mIdxBuf; }
	vkEngine::Buffer<CameraInfo> GetCameraBuffer() const { return mDeferredCtx->Camera; }

	// To set the buffer manually
	vkEngine::GenericBuffer& operator[](const std::string& tag) { return mDeferredCtx->VertexResources[tag].Buffer; }
	const vkEngine::GenericBuffer& operator[](const std::string& tag) const { return mDeferredCtx->VertexResources.at(tag).Buffer; }

	const VertexResourceMap& GetGeometry() const { return mDeferredCtx->VertexResources; }

private:
	std::shared_ptr<DeferredPipelineContext> mDeferredCtx;
	vkEngine::Context mVulkanCtx;

private:
	// Helper functions...

	void CreateFramebuffer(uint32_t width, uint32_t height);
	void SetupBasicConfig(const glm::uvec2& scrSize);

	vkEngine::RenderContextCreateInfo CreateRenderCtxInfo();

	void CreateCopyIdxPipeline(const vkEngine::PShader& shader);

	virtual void BindVertexBuffers(vk::CommandBuffer commandBuffer) const override;
	virtual void BindIndexBuffer(vk::CommandBuffer commandBuffer) const override;

	void SetupPipeline(const DeferredPipelineCreateInfo& createInfo);

	void GenerateBuffers();

	vkEngine::VertexInputDesc ConstructSOAVertexInput() const;

	void MakeHollow();
};

AQUA_END


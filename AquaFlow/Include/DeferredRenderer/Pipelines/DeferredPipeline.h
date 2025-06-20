#pragma once
#include "PipelineConfig.h"
#include "../Renderable/DeferredRenderable.h"
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

class DeferredPipelineContext : public vkEngine::PipelineContext
{
public:
	template <typename Iter>
	inline DeferredPipelineContext(vkEngine::Device ctx, const glm::uvec2& scrSize,
		Iter tagBegin, Iter tagEnd);

public:
	virtual vkEngine::Framebuffer GetFramebuffer() const { return mGeometryBuffer; }

	// The specified resources will be copied into the vertex buffer
	inline virtual void CopyVertices(const DeferredRenderable&) const;
	inline virtual void CopyIndices(const DeferredRenderable&, uint32_t) const;

	// Required by the GraphicsPipeline
	virtual size_t GetVertexCount(const DeferredRenderable& renderable) const
	{ return renderable.Info.Mesh.aPositions.size(); }
	// Required by the GraphicsPipeline
	virtual size_t GetIndexCount(const DeferredRenderable& renderable) const
	{ return renderable.Info.Mesh.aPositions.size(); }

	inline virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer);

	// To set the buffer manually
	vkEngine::GenericBuffer& operator()(const std::string& tag) { return mVertexResources[tag].Buffer; }
	const vkEngine::GenericBuffer& operator()(const std::string& tag) const { return mVertexResources.at(tag).Buffer; }

	std::unordered_map<std::string, VertexResource> GetResources() const { return mVertexResources; }

private:
	VertexLayout mVertexLayout = 
	{ TAG_POSITION, TAG_NORMAL, TAG_TANGENT_SPACE, TAG_TEXTURE_COORDS, TAG_MATERIAL_IDS, TAG_INDEX };

	vkEngine::BasicGraphicsPipelineSettings mBasicConfig;

	// Generic buffers are more expressive than templates at runtime
	
	// Entry '@index' contains the depth buffer
	VertexResourceMap mVertexResources;

	vkEngine::Framebuffer mGeometryBuffer;

	vkEngine::Device mCtx;

	mutable size_t mMeshIdx = 0;
	mutable vkEngine::Buffer<glm::mat4> mSharedModels; // Local buffer
	mutable vkEngine::Buffer<glm::mat4> mModels; // Bound at (set: 0, binding: 1)

	mutable vkEngine::Buffer<CameraInfo> mCamera; // Bound at (set: 0, binding: 0)

private:
	inline void CreateFramebuffer(uint32_t width, uint32_t height);
	inline void SetupBasicConfig(const glm::uvec2& scrSize);

	inline vkEngine::RenderContextCreateInfo CreateRenderCtxInfo();

	friend class DeferredPipeline;
};

template <typename Iter>
inline DeferredPipelineContext::DeferredPipelineContext(vkEngine::Device ctx, const glm::uvec2& scrSize,
	Iter tagBegin, Iter tagEnd)
	: PipelineContext(vkEngine::PipelineType::eGraphics), mCtx(ctx), mVertexLayout(tagBegin, tagEnd)
{
	SetupBasicConfig(scrSize);

	auto rcb = mCtx.FetchRenderContextBuilder(vk::PipelineBindPoint::eGraphics);

	mBasicConfig.TargetContext = rcb.MakeContext(CreateRenderCtxInfo());

	CreateFramebuffer(scrSize.x, scrSize.y);

#if 0
	auto memoryManager = mCtx.MakeMemoryResourceManager();

	vkEngine::BufferCreateInfo indexInfo
	{ 1, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eHostCoherent };

	vkEngine::BufferCreateInfo vertexInfo
	{ 1, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostCoherent };

	RecreateBuffers(indexInfo, vertexInfo);

#endif
}

inline void DeferredPipelineContext::CreateFramebuffer(uint32_t width, uint32_t height)
{
	mGeometryBuffer = mBasicConfig.TargetContext.CreateFramebuffer(width, height);

	auto cAttachs = mGeometryBuffer.GetColorAttachments();

	for (size_t i = 0; i < cAttachs.size(); i++)
	{
		mVertexResources[mVertexLayout[i]].Image = cAttachs[i];
	}

	mVertexResources[TAG_INDEX].Image = mGeometryBuffer.GetDepthStencilAttachment();
}

inline void DeferredPipelineContext::SetupBasicConfig(const glm::uvec2& scrSize)
{
	mBasicConfig.CanvasScissor = vk::Rect2D(scrSize.x, scrSize.y);
	mBasicConfig.CanvasView = vk::Viewport(0.0f, 0.0f, (float)scrSize.x, (float)scrSize.y, 0.0f, 1.0f);

	mBasicConfig.IndicesType = vk::IndexType::eUint32;

#if 0
	mBasicConfig.VertexInput.Bindings.emplace_back(0, (uint32_t)sizeof(GBufferVertexType));

	mBasicConfig.VertexInput.Attributes.emplace_back(0, 0, vk::Format::eR32G32B32A32Sfloat, 0);
	mBasicConfig.VertexInput.Attributes.emplace_back(1, 0, vk::Format::eR32G32B32A32Sfloat, static_cast<uint32_t>(1 * sizeof(glm::vec4)));
	mBasicConfig.VertexInput.Attributes.emplace_back(2, 0, vk::Format::eR32G32B32A32Sfloat, static_cast<uint32_t>(2 * sizeof(glm::vec4)));
	mBasicConfig.VertexInput.Attributes.emplace_back(3, 0, vk::Format::eR32G32B32A32Sfloat, static_cast<uint32_t>(3 * sizeof(glm::vec4)));
	mBasicConfig.VertexInput.Attributes.emplace_back(4, 0, vk::Format::eR32G32B32A32Sfloat, static_cast<uint32_t>(4 * sizeof(glm::vec4)));
	mBasicConfig.VertexInput.Attributes.emplace_back(5, 0, vk::Format::eR32G32Sfloat, static_cast<uint32_t>(4 * sizeof(glm::vec4) + 1 * sizeof(glm::vec2)));
#endif

	mBasicConfig.SubpassIndex = 0;

	mBasicConfig.DepthBufferingState.DepthBoundsTestEnable = false;
	mBasicConfig.DepthBufferingState.DepthCompareOp = vk::CompareOp::eLess;
	mBasicConfig.DepthBufferingState.DepthTestEnable = true;
	mBasicConfig.DepthBufferingState.DepthWriteEnable = true;
	mBasicConfig.DepthBufferingState.MaxDepthBounds = 1.0f;
	mBasicConfig.DepthBufferingState.MinDepthBounds = 0.0f;
	mBasicConfig.DepthBufferingState.StencilTestEnable = false;

	mBasicConfig.Rasterizer.CullMode = vk::CullModeFlagBits::eBack;
	mBasicConfig.Rasterizer.FrontFace = vk::FrontFace::eCounterClockwise;
	mBasicConfig.Rasterizer.LineWidth = 0.01f;
	mBasicConfig.Rasterizer.PolygonMode = vk::PolygonMode::eFill;
}

inline void DeferredPipelineContext::CopyVertices(const DeferredRenderable& renderable) const
{
	_STL_ASSERT(false, "calling an unimplemented function");

#if 0
	// Copying the model matrix to the shared buffer
	mSharedModels << renderable.ModelTransform;

	// Copying the vertex attributes
	auto vb = std::get<0>(mVertexBuffers);

	auto* mem = vb.MapMemory(GetVertexCount(renderable));

	for (size_t i = 0; i < GetVertexCount(renderable); i++)
	{
		mem->Position = glm::vec4(renderable.Info.Mesh.aPositions[i], 1.0f);
		mem->Normals = glm::vec4(renderable.Info.Mesh.aNormals[i], 1.0f);
		mem->Tangents = glm::vec4(renderable.Info.Mesh.aTangents[i], 1.0f);
		mem->BiTangents = glm::vec4(renderable.Info.Mesh.aBitangents[i], 1.0f);
		mem->TexCoords = glm::vec4(renderable.Info.Mesh.aTexCoords[i], 1.0f);
		mem->MaterialIDs = glm::vec2(0.0f, (float) mMeshIdx++); // TODO: Material ID = 0
	}

	vb.UnmapMemory();
#endif
}

inline void DeferredPipelineContext::CopyIndices(const DeferredRenderable& renderable, uint32_t offset) const
{
	_STL_ASSERT(false, "calling an unimplemented function");

#if 0
	// Assume all faces are arranged in triangles

	auto* mem = mIndexBuffer.MapMemory(3 * renderable.Info.Mesh.aFaces.size(), offset);

	for (auto face : renderable.Info.Mesh.aFaces)
	{
		mem[0] = face.Indices[0];
		mem[1] = face.Indices[1];
		mem[2] = face.Indices[2];

		mem++;
	}

	mIndexBuffer.UnmapMemory();
#endif
}

inline void DeferredPipelineContext::UpdateDescriptors(vkEngine::DescriptorWriter& writer)
{
	// For now, we only have camera and model matrices

	vkEngine::UniformBufferWriteInfo uniformInfo{};
	uniformInfo.Buffer = mCamera.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, uniformInfo);

	vkEngine::StorageBufferWriteInfo storageInfo{};
	storageInfo.Buffer = mModels.GetNativeHandles().Handle;

	writer.Update({ 0, 1, 0 }, storageInfo);
}

vkEngine::RenderContextCreateInfo DeferredPipelineContext::CreateRenderCtxInfo()
{
	vkEngine::RenderContextCreateInfo ctxCreateInfo{};

	vkEngine::ImageAttachmentInfo attachInfo{};
	attachInfo.Format = vk::Format::eR32G32B32A32Sfloat;
	attachInfo.Layout = vk::ImageLayout::eColorAttachmentOptimal;
	attachInfo.LoadOp = vk::AttachmentLoadOp::eLoad;
	attachInfo.StoreOp = vk::AttachmentStoreOp::eStore;
	attachInfo.Samples = vk::SampleCountFlagBits::e1;
	attachInfo.Usage = vk::ImageUsageFlagBits::eColorAttachment;
	attachInfo.StencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachInfo.StencilStoreOp = vk::AttachmentStoreOp::eDontCare;

	ctxCreateInfo.ColorAttachments.push_back(attachInfo);

	attachInfo.Format = vk::Format::eR16G16B16A16Sfloat;
	ctxCreateInfo.ColorAttachments.push_back(attachInfo);
	ctxCreateInfo.ColorAttachments.push_back(attachInfo);
	ctxCreateInfo.ColorAttachments.push_back(attachInfo);
	ctxCreateInfo.ColorAttachments.push_back(attachInfo);
	ctxCreateInfo.ColorAttachments.push_back(attachInfo);

	ctxCreateInfo.DepthStencilAttachment.Format = vk::Format::eD24UnormS8Uint;
	ctxCreateInfo.DepthStencilAttachment.Layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	ctxCreateInfo.DepthStencilAttachment.LoadOp = vk::AttachmentLoadOp::eLoad;
	ctxCreateInfo.DepthStencilAttachment.StoreOp = vk::AttachmentStoreOp::eStore;
	ctxCreateInfo.DepthStencilAttachment.Samples = vk::SampleCountFlagBits::e1;
	ctxCreateInfo.DepthStencilAttachment.Usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
	ctxCreateInfo.DepthStencilAttachment.StencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	ctxCreateInfo.DepthStencilAttachment.StencilStoreOp = vk::AttachmentStoreOp::eDontCare;

	ctxCreateInfo.UsingDepthAttachment = true;

	return ctxCreateInfo;
}

using DeferredPipeline = vkEngine::GraphicsPipeline<DeferredPipelineContext>;

AQUA_END

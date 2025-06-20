#pragma once
#include "DeferredPipeline.h"
#include <array>

AQUA_BEGIN

struct ShadingPipelineVertex
{
	glm::vec3 Vertex;
	glm::vec2 TexCoord;
};

using ShadingPipelineQuad = std::array<ShadingPipelineVertex, 4>;

struct Material
{
	glm::vec4 BaseColor;
	float Roughness;
	float Metallicness;
	float RefractiveIndex;
	float TransmissionWeight;
};

// NOTE: Since its a graphics pipeline, we will only be able to use the shading models
// instead of arbitrary materials with user defined shaders. The Material struct above 
// signifies the pbr, blinn phong and perhaps also toon shading models.

enum class ShadingModel
{
	ePBR                  = 1,
	BlinnPhong            = 2,
};

class ShadingPipelineCtx : public vkEngine::GraphicsPipelineContext<ShadingPipelineQuad, 
	uint32_t, ShadingPipelineVertex>
{
public:
	inline ShadingPipelineCtx(vkEngine::Device ctx, const glm::uvec2& scrSize, ShadingModel model);

public:
	~ShadingPipelineCtx() = default;

	virtual vkEngine::Framebuffer GetFramebuffer() const override { return mFramebuffer; }

	inline virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override;

private:
	ShadingModel mModel = ShadingModel::ePBR; // TODO: Not sure if we even this flag here

	ShadingPipelineQuad mQuad = 
	{
		ShadingPipelineVertex({0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}), ShadingPipelineVertex({1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}),
		ShadingPipelineVertex({1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}), ShadingPipelineVertex({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f})
	};

	vkEngine::Device mVulkanCtx;

	vkEngine::Framebuffer mFramebuffer;
	vkEngine::Image mOutImage;

	vkEngine::Buffer<Material> mMaterials;
	VertexResourceMap mGeometry;

	ResourceMap mResources;

private:

	inline void GenerateBuffers();

	inline void CreateFramebuffer(uint32_t width, uint32_t height);
	inline void SetupBasicConfig(const glm::uvec2& scrSize);

	inline virtual void CopyVertices(const MyRenderable& renderble) const override;
	inline virtual void CopyIndices(const MyRenderable& renderable, MyIndex offset) const override;

	virtual size_t GetVertexCount(const MyRenderable& renderable) const override { return 4; }
	virtual size_t GetIndexCount(const MyRenderable& renderable) const override { return 6; }
};

inline void ShadingPipelineCtx::CopyVertices(const MyRenderable& renderable) const
{
	_STL_ASSERT(false, "Not allowed to call ShadingPipeline::SubmitRenderable");

	auto vb = std::get<0>(mVertexBuffers);

	auto* mem = vb.MapMemory(4);

	for (const auto& vertex : renderable)
	{
		*(mem++) = vertex;
	}

	vb.UnmapMemory();
}

inline void ShadingPipelineCtx::CopyIndices(const MyRenderable& renderable, MyIndex offset) const
{
	_STL_ASSERT(false, "Not allowed to call ShadingPipeline::SubmitRenderable");

	auto ib = mIndexBuffer;

	std::array<MyIndex, 6> indices =
	{
		offset + 0, offset + 1, offset + 2,
		offset + 2, offset + 3, offset + 0
	};

	ib << indices;
}

inline void ShadingPipelineCtx::UpdateDescriptors(vkEngine::DescriptorWriter& writer)
{
	auto updateImage = [&writer](const vkEngine::DescriptorLocation& location, vkEngine::Image image)
	{
		vkEngine::SampledImageWriteInfo sampledImageInfo{};
		sampledImageInfo.ImageLayout = vk::ImageLayout::eReadOnlyOptimal;
		sampledImageInfo.ImageView = image.GetIdentityImageView();
		sampledImageInfo.Sampler = *image.GetSampler();

		writer.Update(location, sampledImageInfo);
	};

#if 0

	updateImage({ 0, 0, 0 }, mGeometry.Position);
	updateImage({ 0, 1, 0 }, mGeometry.Normals);
	updateImage({ 0, 2, 0 }, mGeometry.Tangents);
	updateImage({ 0, 3, 0 }, mGeometry.Bitangents);
	updateImage({ 0, 4, 0 }, mGeometry.TexCoords);
	updateImage({ 0, 5, 0 }, mGeometry.MaterialIDs);

#endif 
}

inline ShadingPipelineCtx::ShadingPipelineCtx(vkEngine::Device ctx, const glm::uvec2& scrSize, ShadingModel model)
	: mVulkanCtx(ctx), mModel(model)
{
	SetupBasicConfig(scrSize);

	auto rcb = mVulkanCtx.FetchRenderContextBuilder(vk::PipelineBindPoint::eGraphics);
	vkEngine::RenderContextCreateInfo ctxCreateInfo{};

	vkEngine::ImageAttachmentInfo attachInfo{};
	attachInfo.Format = vk::Format::eR8G8B8A8Snorm;
	attachInfo.Layout = vk::ImageLayout::eColorAttachmentOptimal;
	attachInfo.LoadOp = vk::AttachmentLoadOp::eLoad;
	attachInfo.StoreOp = vk::AttachmentStoreOp::eStore;
	attachInfo.Samples = vk::SampleCountFlagBits::e1;
	attachInfo.Usage = vk::ImageUsageFlagBits::eColorAttachment;
	attachInfo.StencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachInfo.StencilStoreOp = vk::AttachmentStoreOp::eDontCare;

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

	mBasicConfig.TargetContext = rcb.MakeContext(ctxCreateInfo);

	CreateFramebuffer(scrSize.x, scrSize.y);

	GenerateBuffers();

}

inline void ShadingPipelineCtx::CreateFramebuffer(uint32_t width, uint32_t height)
{
	mFramebuffer = mBasicConfig.TargetContext.CreateFramebuffer(width, height);

	auto cAttachs = mFramebuffer.GetColorAttachments();
	mOutImage = cAttachs[0];

	mGeometry[TAG_INDEX].Image = mFramebuffer.GetDepthStencilAttachment();
}

inline void ShadingPipelineCtx::SetupBasicConfig(const glm::uvec2& scrSize)
{
	mBasicConfig.CanvasScissor = vk::Rect2D(scrSize.x, scrSize.y);
	mBasicConfig.CanvasView = vk::Viewport(0.0f, 0.0f, (float)scrSize.x, (float)scrSize.y, 0.0f, 1.0f);

	mBasicConfig.IndicesType = vk::IndexType::eUint32;

	mBasicConfig.VertexInput.Bindings.emplace_back(0, static_cast<uint32_t>(sizeof(ShadingPipelineVertex)));

	mBasicConfig.VertexInput.Attributes.emplace_back(0, 0, vk::Format::eR32G32B32Sfloat, 0);
	mBasicConfig.VertexInput.Attributes.emplace_back(1, 0, vk::Format::eR32G32Sfloat, static_cast<uint32_t>(sizeof(glm::vec3)));

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

void ShadingPipelineCtx::GenerateBuffers()
{
	auto memoryManager = mVulkanCtx.MakeMemoryResourceManager();

	vkEngine::BufferCreateInfo indexInfo
	{ 1, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal };

	vkEngine::BufferCreateInfo vertexInfo
	{ 1, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal };

	RecreateBuffers(indexInfo, vertexInfo, memoryManager);

	vertexInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;
	indexInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;

	auto vertBuf = memoryManager.CreateBuffer<ShadingPipelineVertex>(vertexInfo);
	auto idxBuf = memoryManager.CreateBuffer<MyIndex>(vertexInfo);

	vertBuf << mQuad;
	idxBuf << std::initializer_list<MyIndex>({ 0, 1, 2, 2, 3, 0 });

	vkEngine::CopyBuffer(std::get<0>(mVertexBuffers), vertBuf);
	vkEngine::CopyBuffer(mIndexBuffer, idxBuf);
}

using ShadingPipeline = vkEngine::GraphicsPipeline<ShadingPipelineCtx>;

AQUA_END

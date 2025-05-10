#pragma once

#include "../../Core/AqCore.h"

AQUA_BEGIN

class DeferredRenderable;

struct RendererGeometry
{
	// TODO: There should be depth here, but for now we'll continue with position
	vkEngine::Image Position;
	vkEngine::Image Normals;
	vkEngine::Image Tangents;
	vkEngine::Image Bitangents;
	vkEngine::Image TexCoords;
	vkEngine::Image MaterialIDs;
	// You can find out the position from depth if you have projection matrix
	vkEngine::Image Depth;
};

class GBufferPipelineContext : public vkEngine::GraphicsPipelineContext<DeferredRenderable,
	uint32_t, glm::vec4, glm::vec4, glm::vec4, glm::vec4, glm::vec4, glm::vec2>
{
public:
	GBufferPipelineContext(vkEngine::Device ctx);

	virtual vkEngine::Framebuffer GetFramebuffer() const { return mGBuffer; }

	virtual void CopyVertices(const DeferredRenderable&) const override;
	virtual void CopyIndices(const DeferredRenderable&, uint32_t) const override;

	// Required by the GraphicsPipeline
	virtual size_t GetVertexCount(const DeferredRenderable& renderable) const;
	// Required by the GraphicsPipeline
	virtual size_t GetIndexCount(const DeferredRenderable& renderable) const;

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override;

private:
	RendererGeometry mGeometry;
	vkEngine::Framebuffer mGBuffer;
	vkEngine::RenderTargetContext mRenderTarget;

	vkEngine::Device mCtx;

private:
	void CreateFramebuffer(uint32_t width, uint32_t height);
};

GBufferPipelineContext::GBufferPipelineContext(vkEngine::Device ctx)
	: mCtx(ctx)
{
	auto rcb = mCtx.FetchRenderContextBuilder(vk::PipelineBindPoint::eGraphics);
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
	ctxCreateInfo.DepthStencilAttachment.Layout = vk::ImageLayout::eDepthAttachmentOptimal;
	ctxCreateInfo.DepthStencilAttachment.LoadOp = vk::AttachmentLoadOp::eLoad;
	ctxCreateInfo.DepthStencilAttachment.StoreOp = vk::AttachmentStoreOp::eStore;
	ctxCreateInfo.DepthStencilAttachment.Samples = vk::SampleCountFlagBits::e1;
	ctxCreateInfo.DepthStencilAttachment.Usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
	ctxCreateInfo.DepthStencilAttachment.StencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	ctxCreateInfo.DepthStencilAttachment.StencilStoreOp = vk::AttachmentStoreOp::eDontCare;

	ctxCreateInfo.UsingDepthAttachment = true;

	mRenderTarget = rcb.MakeContext(ctxCreateInfo);

	CreateFramebuffer(1600, 800);
}

void GBufferPipelineContext::CreateFramebuffer(uint32_t width, uint32_t height)
{
	mGBuffer = mRenderTarget.CreateFramebuffer(width, height);

	auto cAttachs = mGBuffer.GetColorAttachments();
	mGeometry.Position = cAttachs[0];
	mGeometry.Normals = cAttachs[1];
	mGeometry.Tangents = cAttachs[2];
	mGeometry.Bitangents = cAttachs[3];
	mGeometry.TexCoords = cAttachs[4];
	mGeometry.MaterialIDs = cAttachs[5];

	mGeometry.Depth = mGBuffer.GetDepthStencilAttachment();
}

using GBufferPipeline = vkEngine::GraphicsPipeline<GBufferPipelineContext>;

AQUA_END

#pragma once
#include "../Core/Config.h"
#include "../Core/Ref.h"
#include "PipelineConfig.h"
#include "../Memory/RenderTargetContext.h"
#include "GraphicsContext.h"
#include "BasicPipeline.h"

#include "../Descriptors/DescriptorsConfig.h"

#include "../Memory/Buffer.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <memory>

#include <vector>

VK_BEGIN

template <typename PipelineContextType, typename BasePipeline = BasicPipeline<PipelineContextType>>
class GraphicsPipeline : public BasePipeline
{
public:
	using MyBasePipeline = BasePipeline;
	using MyContext = typename MyBasePipeline::MyContext;
	using MyVertex = typename MyContext::MyVertex;
	using MyIndex = typename MyContext::MyIndex;
	using MyRenderable = typename MyContext::MyRenderable;

	using VertexBuffer = Buffer<MyVertex>;
	using IndexBuffer = Buffer<MyIndex>;

public:
	GraphicsPipeline() = default;

	void Begin(vk::CommandBuffer commandBuffer,
		const vk::ArrayProxyNoTemporaries<vk::ClearValue>& clearValues);

	void SubmitRenderable(const MyRenderable& renderable);
	void End();

	Framebuffer GetRenderTarget() const { return ((BasePipeline*)this)->GetPipelineContext().GetFramebuffer(); }

private:
	Core::Ref<GraphicsPipelineHandles> mData;
	RenderTargetContext mTargetContext;

	// Geometry memory...
	VertexBuffer mVertexBuffer;
	IndexBuffer mIndexBuffer;

	GraphicsPipelineState mState = GraphicsPipelineState::eInitialized;

	friend class PipelineBuilder;

private:
	// Helper methods...
	void Cleanup();
};

template <typename PipelineContextType, typename BasePipeline>
void GraphicsPipeline<PipelineContextType, BasePipeline>::Cleanup()
{
	mVertexBuffer.Clear();
	mIndexBuffer.Clear();
}

template <typename PipelineContextType, typename BasePipeline>
void GraphicsPipeline<PipelineContextType, BasePipeline>::End()
{
	_STL_ASSERT(mState == GraphicsPipelineState::eRecording, "GraphicsPipeline::Begin has never been called! "
		"You must begin the scope by calling GraphicsPipeline::Begin first before calling "
		"GraphicsPipeline::End!");

	vk::CommandBuffer commandBuffer = ((BasePipeline*) this)->GetCommandBuffer();

	Framebuffer renderTarget = GetRenderTarget();

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mData->Handle);

	if (!mData->SetCache.empty())
	{
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			mData->LayoutData.Layout, 0, mData->SetCache, nullptr);
	}

	commandBuffer.bindVertexBuffers(0, mVertexBuffer.GetNativeHandles().Handle, { 0 });
	commandBuffer.bindIndexBuffer(mIndexBuffer.GetNativeHandles().Handle, 0, vk::IndexType::eUint32);

	commandBuffer.drawIndexed((uint32_t) mIndexBuffer.GetSize(), 1, 0, 0, 0);

	commandBuffer.endRenderPass();

	renderTarget.EndCommands();

	mState = GraphicsPipelineState::eExecutable;

	this->EndPipeline();
}

template <typename PipelineContextType, typename BasePipeline>
void GraphicsPipeline<PipelineContextType, BasePipeline>::SubmitRenderable(const MyRenderable& renderable)
{
	MyContext& context = ((BasePipeline*) this)->GetPipelineContext();

	// Set up vertices and indices of the renderable...
	size_t CurrVertexCount = mVertexBuffer.GetSize();
	size_t CurrIndexCount = mIndexBuffer.GetSize();

	size_t VertexCount = context.GetVertexCount(renderable);
	size_t IndexCount = context.GetIndexCount(renderable);

	mVertexBuffer.Resize(CurrVertexCount + VertexCount);
	mIndexBuffer.Resize(CurrIndexCount + IndexCount);

	MyVertex* VertexBegin = mVertexBuffer.MapMemory(VertexCount, CurrVertexCount);
	MyIndex* IndexBegin = mIndexBuffer.MapMemory(IndexCount, CurrIndexCount);

	context.CopyVertices(VertexBegin, VertexBegin + VertexCount, renderable);
	context.CopyIndices(IndexBegin, IndexBegin + IndexCount, renderable, static_cast<MyIndex>(CurrVertexCount));

	mVertexBuffer.UnmapMemory();
	mIndexBuffer.UnmapMemory();
}

template <typename PipelineContextType, typename BasePipeline>
void GraphicsPipeline<PipelineContextType, BasePipeline>::Begin(vk::CommandBuffer commandBuffer,
	const vk::ArrayProxyNoTemporaries<vk::ClearValue>& clearValues)
{
	_STL_ASSERT(mState != GraphicsPipelineState::eRecording, "GraphicsPipeline::Begin has already been called! "
	"You must end the current scope by calling GraphicsPipeline::End before starting a new one");

	this->BeginPipeline(commandBuffer);

	MyContext& context = ((BasePipeline*) this)->GetPipelineContext();

	Cleanup();

	mState = GraphicsPipelineState::eRecording;

	Framebuffer renderTarget = GetRenderTarget();

	vk::Rect2D renderArea;
	renderArea.offset = vk::Offset2D(0, 0);
	renderArea.extent = vk::Extent2D(renderTarget.GetResolution().x, renderTarget.GetResolution().y);

	renderTarget.BeginCommands(commandBuffer);

	renderTarget.RecordTransitionColorAttachmentLayouts(vk::ImageLayout::eColorAttachmentOptimal,
		vk::PipelineStageFlagBits::eColorAttachmentOutput);

	vk::RenderPassBeginInfo beginRenderPass{};
	beginRenderPass.setClearValues(clearValues);
	beginRenderPass.setFramebuffer(renderTarget.GetNativeHandle());
	beginRenderPass.setRenderArea(renderArea);
	beginRenderPass.setRenderPass(context.GetRenderContext().GetData().RenderPass);

	commandBuffer.beginRenderPass(beginRenderPass, vk::SubpassContents::eInline);
}

VK_END

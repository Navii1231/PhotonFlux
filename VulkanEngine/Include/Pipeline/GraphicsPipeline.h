#pragma once
#include "../Core/Config.h"
#include "../Core/Ref.h"
#include "PipelineConfig.h"
#include "../Memory/RenderTargetContext.h"
#include "GraphicsContext.h"
#include "BasicPipeline.h"

#include "../Descriptors/DescriptorsConfig.h"

#include "../Memory/Buffer.h"

#include "../Core/Logger.h"

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
	using MyIndex = typename MyContext::MyIndex;
	using MyRenderable = typename MyContext::MyRenderable;

	using VertexBuffers = typename PipelineContextType::VertexBuffers;
	using IndexBuffer = typename PipelineContextType::IndexBuffer;

public:
	GraphicsPipeline() = default;

	void Begin(vk::CommandBuffer commandBuffer,
		const vk::ArrayProxyNoTemporaries<vk::ClearValue>& clearValues);

	template <typename T>
	void SetShaderConstant(const std::string& name, const T& constant);

	void DrawIndexed(uint32_t indexOffset, uint32_t vertexOffset, uint32_t firstInstance, 
		uint32_t instanceCount, uint32_t indexCount = std::numeric_limits<uint32_t>::max());

	void End();

	void SubmitRenderable(const MyRenderable& renderable);

	Framebuffer GetRenderTarget() const { return this->GetPipelineContext().GetFramebuffer(); }
	VertexBuffers GetVertexBuffers() const { return this->GetPipelineContext().GetVertexBuffers(); }
	IndexBuffer GetIndexBuffer() const { return this->GetPipelineContext().GetIndexBuffer(); }

private:
	Core::Ref<GraphicsPipelineHandles> mData;
	RenderTargetContext mTargetContext;

	GraphicsPipelineState mState = GraphicsPipelineState::eInitialized;

	MyIndex mCurrVertexCount = 0;

	friend class PipelineBuilder;

private:
	// Helper methods...
	void Cleanup();
};

template <typename PipelineContextType, typename BasePipeline>
void GraphicsPipeline<PipelineContextType, BasePipeline>::Cleanup()
{
	MyContext& context = this->GetPipelineContext();

	context.ForEachVertexBuffer([](size_t index, auto& buffer)
	{
		buffer.Clear();
	});

	GetIndexBuffer().Clear();

	mCurrVertexCount = MyIndex(0);
}

template <typename PipelineContextType, typename BasePipeline>
void GraphicsPipeline<PipelineContextType, BasePipeline>::DrawIndexed(uint32_t firstIndex, 
	uint32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount, uint32_t indexCount)
{
	_STL_ASSERT(mState == GraphicsPipelineState::eRecording, "GraphicsPipeline::Begin has never been called! "
		"You must begin the scope by calling GraphicsPipeline::Begin before calling "
		"GraphicsPipeline::End!");

	MyContext& context = this->GetPipelineContext();

	vk::CommandBuffer commandBuffer = this->GetCommandBuffer();

	Framebuffer renderTarget = GetRenderTarget();

	if (!mData->SetCache.empty())
	{
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			mData->LayoutData.Layout, 0, mData->SetCache, nullptr);
	}

	context.ForEachVertexBuffer([commandBuffer](size_t index, auto& buffer)
	{
		commandBuffer.bindVertexBuffers(static_cast<uint32_t>(index), buffer.GetNativeHandles().Handle, { 0 });
	});

	commandBuffer.bindIndexBuffer(GetIndexBuffer().GetNativeHandles().Handle, 0, context.GetIndexType());

	uint32_t idxCount = indexCount == std::numeric_limits<uint32_t>::max() ?
		(uint32_t) GetIndexBuffer().GetSize() : indexCount;

	commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);

	commandBuffer.endRenderPass();

	renderTarget.EndCommands();
}

template <typename PipelineContextType, typename BasePipeline>
void GraphicsPipeline<PipelineContextType, BasePipeline>::End()
{
	_STL_ASSERT(mState == GraphicsPipelineState::eRecording, "GraphicsPipeline::Begin has never been called! "
		"You must begin the scope by calling GraphicsPipeline::Begin before calling "
		"GraphicsPipeline::End!");

	mState = GraphicsPipelineState::eExecutable;

	this->EndPipeline();
}

template<typename PipelineContextType, typename BasePipeline>
template<typename T>
inline void GraphicsPipeline<PipelineContextType, BasePipeline>::SetShaderConstant(
	const std::string& name, const T& constant)
{
	_VK_ASSERT(this->GetPipelineState() == PipelineState::eRecording,
		"Pipeline must be in the recording state to set a shader constant (i.e within a Begin and End scope)!");

	vk::CommandBuffer commandBuffer = this->GetCommandBuffer();

	MyContext& Context = this->GetPipelineContext();

	const PushConstantSubrangeInfos& subranges = Context.GetPushConstantSubranges();

	_VK_ASSERT(subranges.find(name) != subranges.end(),
		"Failed to find the push constant field \"" << name << "\" in the shader source code\n"
		"Note: If you have turned on shader optimizations (vkEngine::OptimizerFlag::eO3) "
		"and not using the field in the shader, then the field will not appear in the reflections"
	);

	const vk::PushConstantRange range = subranges.at(name);

	_VK_ASSERT(range.size == sizeof(constant),
		"Input field size of the push constant does not match with the expected size!\n"
		"Possible causes might be:\n"
		"* Alignment mismatch between GPU and CPU structs\n"
		"* Data type mismatch between shader and C++ declarations\n"
		"* The constant has been optimized away in the shader\n"
	);

	commandBuffer.pushConstants(mData->LayoutData.Layout, range.stageFlags,
		range.offset, range.size, reinterpret_cast<const void*>(&constant));
}

template <typename PipelineContextType, typename BasePipeline>
void GraphicsPipeline<PipelineContextType, BasePipeline>::SubmitRenderable(const MyRenderable& renderable)
{
	MyContext& context = this->GetPipelineContext();

	context.CopyVertices(renderable);
	context.CopyIndices(renderable, static_cast<MyIndex>(mCurrVertexCount));

	mCurrVertexCount += static_cast<MyIndex>(context.GetVertexCount(renderable));
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

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mData->Handle);
}

VK_END

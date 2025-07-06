#pragma once
#include "../Core/Config.h"
#include "../Core/Ref.h"
#include "PipelineConfig.h"
#include "../Memory/RenderTargetContext.h"
#include "BasicPipeline.h"
#include "PShader.h"

#include "../Descriptors/DescriptorsConfig.h"

#include "../Memory/Buffer.h"

#include "../Core/Logger.h"

VK_BEGIN

struct GraphicsPipelineHandles : public PipelineHandles, public PipelineInfo
{
	RenderTargetContext TargetContext;
	GraphicsPipelineState State = GraphicsPipelineState::eInitialized;
};

// Not thread safe
// A bit shady
template <typename Renderable, typename BasePipeline>
class BasicGraphicsPipeline : public BasePipeline
{
public:
	using MyBasePipeline = BasePipeline;
	using MyRenderable = Renderable;
	using MyIndex = uint32_t; // For now, we will set IndexType to uint32_t only

public:
	BasicGraphicsPipeline() = default;
	virtual ~BasicGraphicsPipeline() = default;

	// Methods inside the begin/end scope
	virtual void Begin(vk::CommandBuffer commandBuffer,
		const vk::ArrayProxyNoTemporaries<vk::ClearValue>& clearValues);

	template <typename T>
	void SetShaderConstant(const std::string& name, const T& constant);

	void DrawIndexed(uint32_t indexOffset, uint32_t vertexOffset, uint32_t firstInstance,
		uint32_t instanceCount, uint32_t indexCount = std::numeric_limits<uint32_t>::max());

	// Ends the scope
	virtual void End();

	virtual void UpdateDescriptors() = 0;

	// methods outside the scope
	virtual void SubmitRenderable(const MyRenderable& renderable) const = 0;

	virtual const GraphicsPipelineConfig& GetConfig() const = 0;
	virtual const PShader& GetShader() const override { return mShader; }

	// Required by the GraphicsPipeline
	Framebuffer GetFramebuffer() const { return mFramebuffer; }
	void SetFramebuffer(vkEngine::Framebuffer target) { mFramebuffer = target; }

	vkEngine::RenderTargetContext GetRenderTargetContext() const
	{ return mHandles->TargetContext; }

private:
	Core::Ref<GraphicsPipelineHandles> mHandles;
	vkEngine::Framebuffer mFramebuffer;
	vkEngine::PShader mShader; // TODO: Could be stored inside the vkEngine::DescriptorWriter

	friend class PipelineBuilder;

protected:
	// User defined behavior...

	void SetShader(const PShader& shader) { mShader = shader; }

	virtual void Cleanup() = 0;
	virtual void BindVertexBuffers(vk::CommandBuffer commandBuffer) const = 0;
	virtual void BindIndexBuffer(vk::CommandBuffer commandBuffer) const = 0;

	virtual size_t GetVertexCount() const = 0;
	virtual size_t GetIndexCount() const = 0;
};

template <typename Renderable>
using GraphicsPipeline = BasicGraphicsPipeline<Renderable, BasicPipeline>;

template <typename Renderable, typename BasePipeline>
void BasicGraphicsPipeline<Renderable, BasePipeline>::Begin(vk::CommandBuffer commandBuffer,
	const vk::ArrayProxyNoTemporaries<vk::ClearValue>& clearValues)
{
	_STL_ASSERT(mHandles->State != GraphicsPipelineState::eRecording, "GraphicsPipeline::Begin has already been called! "
		"You must end the current scope by calling GraphicsPipeline::End before starting a new one");

	this->BeginPipeline(commandBuffer);

	mHandles->State = GraphicsPipelineState::eRecording;

	Framebuffer renderTarget = GetFramebuffer();

	vk::Rect2D renderArea;
	renderArea.offset = vk::Offset2D(0, 0);
	renderArea.extent = vk::Extent2D(renderTarget.GetResolution().x, renderTarget.GetResolution().y);

	renderTarget.BeginCommands(commandBuffer);

	// TODO: what about the depth attachment layout?
	renderTarget.RecordTransitionColorAttachmentLayouts(vk::ImageLayout::eColorAttachmentOptimal,
		vk::PipelineStageFlagBits::eColorAttachmentOutput);

	vk::RenderPassBeginInfo beginRenderPass{};
	beginRenderPass.setClearValues(clearValues);
	beginRenderPass.setFramebuffer(renderTarget.GetNativeHandle());
	beginRenderPass.setRenderArea(renderArea);
	beginRenderPass.setRenderPass(GetRenderTargetContext().GetData().RenderPass);

	commandBuffer.beginRenderPass(beginRenderPass, vk::SubpassContents::eInline);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mHandles->Handle);
}

template<typename Renderable, typename BasePipeline>
template<typename T>
inline void BasicGraphicsPipeline<Renderable, BasePipeline>::SetShaderConstant(
	const std::string& name, const T& constant)
{
	_VK_ASSERT(this->GetPipelineState() == PipelineState::eRecording,
		"Pipeline must be in the recording state to set a shader constant (i.e within a Begin and End scope)!");

	vk::CommandBuffer commandBuffer = this->GetCommandBuffer();

	const PushConstantSubrangeInfos& subranges = GetShader().GetPushConstantSubranges();

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

	commandBuffer.pushConstants(mHandles->LayoutData.Layout, range.stageFlags,
		range.offset, range.size, reinterpret_cast<const void*>(&constant));
}

template <typename Renderable, typename BasePipeline>
void BasicGraphicsPipeline<Renderable, BasePipeline>::DrawIndexed(uint32_t firstIndex, 
	uint32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount, uint32_t indexCount)
{
	_STL_ASSERT(mHandles->State == GraphicsPipelineState::eRecording, "GraphicsPipeline::Begin has never been called! "
		"You must begin the scope by calling GraphicsPipeline::Begin before calling "
		"GraphicsPipeline::End!");

	vk::CommandBuffer commandBuffer = this->GetCommandBuffer();

	Framebuffer renderTarget = GetFramebuffer();

	if (!mHandles->SetCache.empty())
	{
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			mHandles->LayoutData.Layout, 0, mHandles->SetCache, nullptr);
	}

	BindVertexBuffers(commandBuffer);
	BindIndexBuffer(commandBuffer);

	uint32_t idxCount = indexCount == std::numeric_limits<uint32_t>::max() ?
		(uint32_t) GetIndexCount() : indexCount;

	commandBuffer.drawIndexed(idxCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

template <typename Renderable, typename BasePipeline>
void BasicGraphicsPipeline<Renderable, BasePipeline>::End()
{
	_STL_ASSERT(mHandles->State == GraphicsPipelineState::eRecording, "GraphicsPipeline::Begin has never been called! "
		"You must begin the scope by calling GraphicsPipeline::Begin before calling "
		"GraphicsPipeline::End!");

	mHandles->State = GraphicsPipelineState::eExecutable;

	this->GetCommandBuffer().endRenderPass();

	GetFramebuffer().EndCommands();

	this->EndPipeline();
}

VK_END

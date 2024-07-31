#pragma once
#include "../Core/Config.h"
#include "../Core/Ref.h"
#include "PipelineConfig.h"
#include "../Memory/RenderContext.h"
#include "GraphicsSettings.h"

#include "../Descriptors/DescriptorsConfig.h"

#include "../Memory/Buffer.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <memory>

#include <vector>

VK_BEGIN

// Derive the class GraphicsConfigBase and override the virtual functions to pass the template argument
template <typename PipelineSettingsType>
class GraphicsPipeline
{
public:
	using PipelineSettings = PipelineSettingsType;
	using Vertex = typename PipelineSettingsType::Vertex;
	using Index = typename PipelineSettingsType::Index;
	using Renderable = typename PipelineSettingsType::Renderable;

	using VertexBuffer = Buffer<Vertex>;
	using IndexBuffer = Buffer<Index>;

public:
	GraphicsPipeline() = default;

	void Begin(vk::CommandBuffer commandBuffer,
		const vk::ArrayProxyNoTemporaries<vk::ClearValue>& clearValues);

	void SubmitRenderable(const Renderable& renderable);
	void End();

	Framebuffer GetRenderTarget() const { return mConfig->GetFramebuffer(); }

	PipelineSettings& GetConfigSettings() { return *mConfig; }
	const PipelineSettings& GetConfigSettings() const { return *mConfig; }

private:
	Core::Ref<GraphicsPipelineHandles> mData;
	RenderContext mContext;

	std::shared_ptr<PipelineSettings> mConfig;
	DescriptorWriter mDescWriter;

	// Geometry memory...
	VertexBuffer mVertexBuffer;
	IndexBuffer mIndexBuffer;

	GraphicsPipelineState mState = GraphicsPipelineState::eInitialized;

	// TODO: Should part of the shared state...
	vk::CommandBuffer mWorkingCommandBuffer = nullptr;

	friend class PipelineBuilder;

private:
	// Helper methods...
	void Cleanup();
};

template <typename PipelineSettingsType>
void VK_NAMESPACE::GraphicsPipeline<PipelineSettingsType>::Cleanup()
{
	mWorkingCommandBuffer = nullptr;
	mVertexBuffer.Clear();
	mIndexBuffer.Clear();
}

template <typename PipelineSettingsType>
void VK_NAMESPACE::GraphicsPipeline<PipelineSettingsType>::End()
{
	_STL_ASSERT(mState == GraphicsPipelineState::eRecording, "GraphicsPipeline::Begin has never been called! "
		"You must begin the scope by calling GraphicsPipeline::Begin first before calling "
		"GraphicsPipeline::End!");

	Framebuffer renderTarget = mConfig->GetFramebuffer();

	mConfig->UpdateDescriptors(mDescWriter);

	if (!mData->SetCache.empty())
	{
		mWorkingCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			mData->LayoutData.Layout, 0, mData->SetCache, nullptr);
	}

	mWorkingCommandBuffer.bindVertexBuffers(0, mVertexBuffer.GetNativeHandles().Handle, { 0 });
	mWorkingCommandBuffer.bindIndexBuffer(mIndexBuffer.GetNativeHandles().Handle, 0, vk::IndexType::eUint32);

	mWorkingCommandBuffer.drawIndexed((uint32_t) mIndexBuffer.GetSize(), 1, 0, 0, 0);

	mWorkingCommandBuffer.endRenderPass();

	renderTarget.EndCommands();

	mState = GraphicsPipelineState::eExecutable;
}

template <typename PipelineSettingsType>
void VK_NAMESPACE::GraphicsPipeline<PipelineSettingsType>::SubmitRenderable(const Renderable& renderable)
{
	// Set up vertices and indices of the renderable...
	size_t CurrVertexCount = mVertexBuffer.GetSize();
	size_t CurrIndexCount = mIndexBuffer.GetSize();

	size_t VertexCount = mConfig->GetVertexCount(renderable);
	size_t IndexCount = mConfig->GetIndexCount(renderable);

	mVertexBuffer.Resize(CurrVertexCount + VertexCount);
	mIndexBuffer.Resize(CurrIndexCount + IndexCount);

	Vertex* VertexBegin = mVertexBuffer.MapMemory(VertexCount, CurrVertexCount);
	Index* IndexBegin = mIndexBuffer.MapMemory(IndexCount, CurrIndexCount);

	mConfig->CopyVertices(VertexBegin, VertexBegin + VertexCount, renderable);
	mConfig->CopyIndices(IndexBegin, IndexBegin + IndexCount, renderable, static_cast<Index>(CurrVertexCount));

	mVertexBuffer.UnmapMemory();
	mIndexBuffer.UnmapMemory();

	// Set up textures...
}

template <typename PipelineSettingsType>
void VK_NAMESPACE::GraphicsPipeline<PipelineSettingsType>::Begin(vk::CommandBuffer commandBuffer,
	const vk::ArrayProxyNoTemporaries<vk::ClearValue>& clearValues)
{
	_STL_ASSERT(mState != GraphicsPipelineState::eRecording, "GraphicsPipeline::Begin has already been called! "
	"You must end the current scope by calling GraphicsPipeline::End before starting a new one");

	Cleanup();

	mState = GraphicsPipelineState::eRecording;

	Framebuffer renderTarget = mConfig->GetFramebuffer();

	mWorkingCommandBuffer = commandBuffer;

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
	beginRenderPass.setRenderPass(mConfig->GetRenderContext().GetData().RenderPass);

	commandBuffer.beginRenderPass(beginRenderPass, vk::SubpassContents::eInline);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mData->Handle);

}

VK_END

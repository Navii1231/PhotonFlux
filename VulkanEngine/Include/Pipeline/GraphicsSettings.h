#pragma once
#include "PipelineConfig.h"
#include "PipelineSettingsBase.h"
#include "Descriptors/DescriptorSetAllocator.h"

VK_BEGIN

struct BasicGraphicsPipelineSettings
{
	vk::PrimitiveTopology Topology = vk::PrimitiveTopology::eTriangleList;
	VertexInputDesc VertexInput;

	RasterizerInfo Rasterizer;

	DepthStencilBufferingInfo DepthBufferingState;

	vk::Viewport CanvasView;
	vk::Rect2D CanvasScissor;

	RenderContext TargetContext;

	uint32_t SubpassIndex = 0;
};

// The pipeline settings must be configured by this class alone!
template <typename RenderableType, typename VertexType, typename IndexType>
class GraphicsPipelineSettingsBase : public PipelineSettingsBase
{
public:
	using Renderable = RenderableType;
	using Vertex = VertexType;
	using Index = IndexType;

public:
	GraphicsPipelineSettingsBase()
		: PipelineSettingsBase(PipelineType::eGraphics) {}

	explicit GraphicsPipelineSettingsBase(const BasicGraphicsPipelineSettings& basicConfig)
		: PipelineSettingsBase(PipelineType::eGraphics), mBasicConfig(basicConfig) {}

	// user dependent stuff...
	virtual Framebuffer GetFramebuffer() const = 0;

	virtual void CopyVertices(Vertex* Begin, Vertex* End, const Renderable& renderable) const = 0;
	virtual void CopyIndices(Index* Begin, Index* End, const Renderable& renderable, Index offset) const = 0;

	virtual size_t GetVertexCount(const Renderable& renderable) const = 0;
	virtual size_t GetIndexCount(const Renderable& renderable) const = 0;

	virtual void UpdateDescriptors(DescriptorWriter& writer) override = 0;

	// Non-virtual getters
	vk::PrimitiveTopology GetPrimitiveTopology() const { return mBasicConfig.Topology; }

	RasterizerInfo GetRasterizerStateInfo() const { return mBasicConfig.Rasterizer; }
	DepthStencilBufferingInfo GetDepthStencilStateInfo() const { return mBasicConfig.DepthBufferingState; }

	std::pair<vk::Viewport, vk::Rect2D> GetViewportAndScissorInfo() const 
	{ return { mBasicConfig.CanvasView, mBasicConfig.CanvasScissor }; }

	RenderContext GetRenderContext() const { return mBasicConfig.TargetContext; }
	VertexInputDesc GetVertexInputStateInfo() const { return mBasicConfig.VertexInput; }

	uint32_t GetSubpassIndex() const { return mBasicConfig.SubpassIndex; }

protected:
	BasicGraphicsPipelineSettings mBasicConfig;

private:
	// The fields below are going to be set by the CompileShaders method...
	DescSetLayoutBindingMap mPipelineSetLayoutInfo;
	std::vector<vk::PushConstantRange> mPushConstantRangeInfo;
	std::vector<CompileResult> mCompileResults;

	template <typename Config>
	friend class GraphicsPipeline;

	friend class Device;
};

VK_END

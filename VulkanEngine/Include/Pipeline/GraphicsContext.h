#pragma once
#include "PipelineConfig.h"
#include "PipelineContext.h"
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

	RenderTargetContext TargetContext;

	uint32_t SubpassIndex = 0;
};

// The pipeline settings must be configured by this class alone!
template <typename RenderableType, typename VertexType, typename IndexType>
class GraphicsPipelineContext : public PipelineContext
{
public:
	using MyRenderable = RenderableType;
	using MyVertex = VertexType;
	using MyIndex = IndexType;

public:
	GraphicsPipelineContext()
		: PipelineContext(PipelineType::eGraphics) {}

	explicit GraphicsPipelineContext(const BasicGraphicsPipelineSettings& basicConfig)
		: PipelineContext(PipelineType::eGraphics), mBasicConfig(basicConfig) {}

	// user dependent stuff...
	
	// Required by the GraphicsPipeline
	virtual Framebuffer GetFramebuffer() const = 0;

	// Required by the GraphicsPipeline
	virtual void CopyVertices(MyVertex* Begin, MyVertex* End, const MyRenderable& renderable) const = 0;
	// Required by the GraphicsPipeline
	virtual void CopyIndices(MyIndex* Begin, MyIndex* End, const MyRenderable& renderable, MyIndex offset) const = 0;

	// Required by the GraphicsPipeline
	virtual size_t GetVertexCount(const MyRenderable& renderable) const = 0;
	// Required by the GraphicsPipeline
	virtual size_t GetIndexCount(const MyRenderable& renderable) const = 0;

	virtual void UpdateDescriptors(DescriptorWriter& writer) override = 0;

	// Non-virtual getters
	vk::PrimitiveTopology GetPrimitiveTopology() const { return mBasicConfig.Topology; }

	RasterizerInfo GetRasterizerStateInfo() const { return mBasicConfig.Rasterizer; }
	DepthStencilBufferingInfo GetDepthStencilStateInfo() const { return mBasicConfig.DepthBufferingState; }

	std::pair<vk::Viewport, vk::Rect2D> GetViewportAndScissorInfo() const 
	{ return { mBasicConfig.CanvasView, mBasicConfig.CanvasScissor }; }

	RenderTargetContext GetRenderContext() const { return mBasicConfig.TargetContext; }
	VertexInputDesc GetVertexInputStateInfo() const { return mBasicConfig.VertexInput; }

	uint32_t GetSubpassIndex() const { return mBasicConfig.SubpassIndex; }

protected:
	BasicGraphicsPipelineSettings mBasicConfig;

private:
	// The fields below are going to be set by the CompileShaders method...
	DescSetLayoutBindingMap mPipelineSetLayoutInfo;
	std::vector<vk::PushConstantRange> mPushConstantsInfos;
	std::vector<CompileResult> mCompileResults;

	template <typename PipelineContextType, typename BasePipeline>
	friend class GraphicsPipeline;

	friend class Device;
};

VK_END

#pragma once
#include "PipelineConfig.h"
#include "PipelineContext.h"
#include "Descriptors/DescriptorSetAllocator.h"

#include <tuple>

VK_BEGIN

enum class GraphicsBufferTypeBits
{
	eNone      = 0,
	eVertex    = 1,
	eIndex     = 2,
};

using GraphicsBufferTypes = vk::Flags<GraphicsBufferTypeBits>;

struct BasicGraphicsPipelineSettings
{
	vk::PrimitiveTopology Topology = vk::PrimitiveTopology::eTriangleList;
	VertexInputDesc VertexInput;

	RasterizerInfo Rasterizer;

	DepthStencilBufferingInfo DepthBufferingState;

	vk::Viewport CanvasView;
	vk::Rect2D CanvasScissor;

	RenderTargetContext TargetContext;

	vk::IndexType IndicesType = vk::IndexType::eUint32;

	uint32_t SubpassIndex = 0;
};

template <typename RenderableType, typename IndexType, typename ...VertexTypes>
class GraphicsPipelineContext : public PipelineContext
{
public:
	using MyRenderable = RenderableType;
	using MyIndex = IndexType;

	using IndexBuffer = vkEngine::Buffer<MyIndex>;
	using VertexBuffers = std::tuple<vkEngine::Buffer<VertexTypes>...>;

public:
	GraphicsPipelineContext()
		: PipelineContext(PipelineType::eGraphics) {}

	explicit GraphicsPipelineContext(const BasicGraphicsPipelineSettings& basicConfig)
		: PipelineContext(PipelineType::eGraphics), mBasicConfig(basicConfig) {}

	// user dependent stuff begins here...

	void SetMemoryProperties(GraphicsBufferTypes bufferType, vk::MemoryPropertyFlags flags);
	void SetBufferUsage(GraphicsBufferTypes bufferType, vk::BufferUsageFlags flags);

	// Required by the GraphicsPipeline
	virtual Framebuffer GetFramebuffer() const = 0;

	// Required by the GraphicsPipeline
	virtual void CopyVertices(const MyRenderable& renderble) const = 0;

	// Required by the GraphicsPipeline
	virtual void CopyIndices(const MyRenderable& renderable, MyIndex offset) const = 0;

	// Required by the GraphicsPipeline
	virtual size_t GetVertexCount(const MyRenderable& renderable) const = 0;
	// Required by the GraphicsPipeline
	virtual size_t GetIndexCount(const MyRenderable& renderable) const = 0;

	virtual void UpdateDescriptors(DescriptorWriter& writer) override = 0;

	template <size_t Index>
	decltype(auto) GetVertexBuffer() const { return std::get<Index>(mVertexBuffers); }

	VertexBuffers GetVertexBuffers() const { return mVertexBuffers; }
	IndexBuffer GetIndexBuffer() const { return mIndexBuffer; }

	vk::IndexType GetIndexType() const { return mBasicConfig.IndicesType; }

	// Non-virtual getters
	vk::PrimitiveTopology GetPrimitiveTopology() const { return mBasicConfig.Topology; }

	RasterizerInfo GetRasterizerStateInfo() const { return mBasicConfig.Rasterizer; }
	DepthStencilBufferingInfo GetDepthStencilStateInfo() const { return mBasicConfig.DepthBufferingState; }

	std::pair<vk::Viewport, vk::Rect2D> GetViewportAndScissorInfo() const
	{ return { mBasicConfig.CanvasView, mBasicConfig.CanvasScissor }; }

	RenderTargetContext GetRenderContext() const { return mBasicConfig.TargetContext; }
	VertexInputDesc GetVertexInputStateInfo() const { return mBasicConfig.VertexInput; }

	uint32_t GetSubpassIndex() const { return mBasicConfig.SubpassIndex; }

public:

	template <typename Func, size_t Index = 0>
	void ForEachVertexBuffer(Func&& func);

protected:
	BasicGraphicsPipelineSettings mBasicConfig;

	VertexBuffers mVertexBuffers;
	IndexBuffer mIndexBuffer;

	vk::MemoryPropertyFlags mIndexMemProperties{ 0 };
	vk::MemoryPropertyFlags mVertexMemProperties{ 0 };

	vk::BufferUsageFlags mIndexUsage = vk::BufferUsageFlagBits::eIndexBuffer;
	vk::BufferUsageFlags mVertexUsage = vk::BufferUsageFlagBits::eVertexBuffer;

private:
	template <typename PipelineContextType, typename BasePipeline>
	friend class ::VK_NAMESPACE::GraphicsPipeline;

	friend class ::VK_NAMESPACE::PipelineBuilder;
	friend class ::VK_NAMESPACE::Device;

private:
	// Helpers...

	void CreateIndexBuffer(vkEngine::MemoryResourceManager manager);

	template <size_t Index = 0>
	void CreateVertexBuffers(vkEngine::MemoryResourceManager manager);
};

template <typename RenderableType, typename IndexType, typename ...VertexTypes>
template <typename Func, size_t Index /*= 0*/>
inline void GraphicsPipelineContext<RenderableType, IndexType, VertexTypes...>::
	ForEachVertexBuffer(Func&& func)
{
	// Recursive tuple iteration
	if constexpr (Index < std::tuple_size<VertexBuffers>::value)
	{
		// Apply the lambda to the buffer at this index
		auto& buffer = std::get<Index>(mVertexBuffers);
		func(Index, buffer);  // Invoke the lambda with the buffer

		// Recurse to the next index
		ForEachVertexBuffer<Func, Index + 1>(std::forward<Func>(func));
	}
}

template <typename RenderableType, typename IndexType, typename ...VertexTypes>
inline void GraphicsPipelineContext<RenderableType, IndexType, VertexTypes...>::
	CreateIndexBuffer(vkEngine::MemoryResourceManager manager)
{
	vkEngine::BufferCreateInfo indexInfo{};
	indexInfo.MemProps = mIndexMemProperties;
	indexInfo.Usage = mIndexUsage;

	mIndexBuffer = manager.CreateBuffer<MyIndex>(indexInfo);
}

template <typename RenderableType, typename IndexType, typename ...VertexTypes>
template <size_t Index /*= 0*/>
inline void GraphicsPipelineContext<RenderableType, IndexType, VertexTypes...>::
	CreateVertexBuffers(vkEngine::MemoryResourceManager manager)
{
	vkEngine::BufferCreateInfo vertexInfo{};
	vertexInfo.MemProps = mVertexMemProperties;
	vertexInfo.Usage = mVertexUsage;

	// Recursive tuple iteration
	if constexpr (Index < std::tuple_size<VertexBuffers>::value)
	{
		// Get the buffer at this index
		auto& buffer = std::get<Index>(mVertexBuffers);
		buffer = manager.CreateBuffer<typename std::decay_t<decltype(buffer)>::Type>(vertexInfo);

		// Recurse to the next index
		CreateVertexBuffers<Index + 1>(manager);
	}
}

template <typename RenderableType, typename IndexType, typename ...VertexTypes>
inline void GraphicsPipelineContext<RenderableType, IndexType, VertexTypes...>::
	SetBufferUsage(GraphicsBufferTypes bufferType, vk::BufferUsageFlags flags)
{
	auto SetFlags = [bufferType, flags](GraphicsBufferTypes type, 
		vk::BufferUsageFlags& outFlags, vk::BufferUsageFlagBits bufferFlag)
	{
		if ((bufferType & type) != GraphicsBufferTypes(0))
			outFlags = flags | bufferFlag;
	};

	SetFlags(GraphicsBufferTypeBits::eIndex, mIndexUsage, vk::BufferUsageFlagBits::eIndexBuffer);
	SetFlags(GraphicsBufferTypeBits::eVertex, mVertexUsage, vk::BufferUsageFlagBits::eVertexBuffer);
}

template<typename RenderableType, typename IndexType, typename ...VertexTypes>
inline void GraphicsPipelineContext<RenderableType, IndexType, VertexTypes...>::
	SetMemoryProperties(GraphicsBufferTypes bufferType, vk::MemoryPropertyFlags flags)
{
	auto SetFlags = [bufferType, flags](GraphicsBufferTypes type, vk::MemoryPropertyFlags& outFlags)
	{
		if ((bufferType & type) != GraphicsBufferTypes(0))
			outFlags = flags;
	};

	SetFlags(GraphicsBufferTypeBits::eIndex, mIndexMemProperties);
	SetFlags(GraphicsBufferTypeBits::eVertex, mVertexMemProperties);
}

VK_END

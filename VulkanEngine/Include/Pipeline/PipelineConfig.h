#pragma once
#include "../Core/Config.h"
#include "../Core/Ref.h"
#include "../Core/Utils/FramebufferUtils.h"

#include "../Memory/Image.h"
#include "../Descriptors/DescriptorsConfig.h"

#include "../ShaderCompiler/ShaderCompiler.h"

VK_BEGIN

class RenderContext;

enum class GraphicsPipelineState
{
	eInitialized       = 0,
	eRecording         = 1,
	eExecutable        = 2,
};

struct RasterizerInfo
{
	vk::CullModeFlags CullMode = vk::CullModeFlagBits::eNone;
	vk::FrontFace FrontFace = vk::FrontFace::eClockwise;
	float LineWidth = 1.0f;
	vk::PolygonMode PolygonMode = vk::PolygonMode::eFill;
};

struct DepthStencilBufferingInfo
{
	bool DepthTestEnable = false;
	bool DepthWriteEnable = false;
	bool DepthBoundsTestEnable = true;
	vk::CompareOp DepthCompareOp = vk::CompareOp::eLess;
	float MinDepthBounds = 0.0f;
	float MaxDepthBounds = 1.0f;
	bool StencilTestEnable = false;
	vk::StencilOpState StencilFrontOp{};
	vk::StencilOpState StencilBackOp{};
};

struct VertexInputDesc
{
	std::vector<vk::VertexInputAttributeDescription> Attributes;
	std::vector<vk::VertexInputBindingDescription> Bindings;
};

struct PipelineLayoutData
{
	vk::PipelineLayout Layout;
	std::vector<Core::Ref<DescriptorResource>> DescResources;
	std::vector<vk::PushConstantRange> PushConstantRanges;
};

struct PipelineInfo
{
	std::vector<ShaderSPIR_V> ShaderStages;
};

struct GraphicsPipelineInfo : public PipelineInfo
{
	vk::PrimitiveTopology Topology = vk::PrimitiveTopology::eTriangleList;
	VertexInputDesc VertexInputState;

	RasterizerInfo Rasterizer;
	DepthStencilBufferingInfo DepthBufferingInfo;

	vk::Viewport Viewport;
	vk::Rect2D Scissor;
};

struct ComputePipelineInfo : public PipelineInfo
{

};

struct PipelineHandles
{
	vk::Pipeline Handle;
	PipelineLayoutData LayoutData;
	//std::vector<vk::DescriptorSetLayout> SetLayouts;
	std::vector<vk::DescriptorSet> SetCache;
	std::vector<uint32_t> DynamicOffsets;

	Core::Ref<vk::Device> Device;
};

struct GraphicsPipelineHandles : public PipelineHandles
{
	GraphicsPipelineInfo Info;
};

struct ComputePipelineHandles : public PipelineHandles
{
	ComputePipelineInfo Info;
};

struct PipelineBuilderData
{
	vk::PipelineCache Cache;
};

struct GraphicsPipelineHandlesDeleter
{
	void operator ()(const GraphicsPipelineHandles& handles) const
	{
		//for (auto setLayout : handles.SetLayouts)
		//	handles.Device->destroyDescriptorSetLayout(setLayout);

		handles.Device->destroyPipelineLayout(handles.LayoutData.Layout);
		handles.Device->destroyPipeline(handles.Handle);
	}
};

VK_END

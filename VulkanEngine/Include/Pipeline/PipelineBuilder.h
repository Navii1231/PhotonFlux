#pragma once
#include "../Core/Config.h"
#include "../Core/Ref.h"

#include "GraphicsPipeline.h"
#include "ComputePipeline.h"

#include "../Descriptors/DescriptorWriter.h"
#include "../Descriptors/DescriptorPoolManager.h"

#include "../Memory/MemoryResourceManager.h"

VK_BEGIN

class PipelineBuilder
{
public:
	template <typename PipelineContextType>
	GraphicsPipeline<PipelineContextType, BasicPipeline<PipelineContextType>>
		BuildGraphicsPipeline(const PipelineContextType& config) const;

	template <typename PipelineContextType>
	ComputePipeline<PipelineContextType, BasicPipeline<PipelineContextType>>
		BuildComputePipeline(const PipelineContextType& config) const;

	// TODO: Implement ray tracing pipeline creation

	/*template <typename PipelineContextType>
	RayTracingPipeline<PipelineContextType, BasicPipeline<PipelineContextType>>
		BuildRayTracingPipeline(const PipelineContextType& config) const;*/

private:
	Core::Ref<vk::Device> mDevice;

	Core::Ref<PipelineBuilderData> mData;
	MemoryResourceManager mMemoryManager;
	DescriptorPoolManager mDescPoolManager;

	friend class Device;

private:
	// Helper methods...
	template <typename PipelineContextType>
	vk::PipelineInputAssemblyStateCreateInfo GetAssemblyStateInfo(
		GraphicsPipelineInfo& Info, const PipelineContextType& config) const;

	template <typename PipelineContextType> 
	vk::PipelineVertexInputStateCreateInfo GetVertexInputStateInfo(
		GraphicsPipelineInfo& Info, const PipelineContextType& config) const;

	template <typename PipelineContextType>
	std::vector<vk::PipelineShaderStageCreateInfo> GetPipelineStages(
		PipelineInfo& Info, const PipelineContextType& config) const;

	template <typename PipelineContextType> 
	vk::PipelineRasterizationStateCreateInfo GetRasterizerStateInfo(
		GraphicsPipelineInfo& Info, const PipelineContextType& config) const;

	template <typename PipelineContextType>
	vk::PipelineDepthStencilStateCreateInfo GetDepthStencilStateInfo(
		GraphicsPipelineInfo& Info, const PipelineContextType& config) const;

	template <typename PipelineContextType> 
	vk::PipelineMultisampleStateCreateInfo GetSampleStateInfo(
		GraphicsPipelineInfo& Info, const PipelineContextType& config) const;

	template <typename PipelineContextType> 
	vk::PipelineColorBlendStateCreateInfo GetBlendStateInfo(
		GraphicsPipelineInfo& Info, const PipelineContextType& config) const;

	template <typename PipelineContextType> 
	void GetViewportStateInfo(
		GraphicsPipelineInfo& Info, const PipelineContextType& config) const;

	template <typename PipelineContextType> 
	PipelineLayoutData CreatePipelineLayout(const PipelineContextType& config) const;

	void FreeShaderModules(const std::vector<vk::PipelineShaderStageCreateInfo>& shaders) const
	{
		for (const auto& shader : shaders)
			mDevice->destroyShaderModule(shader.module);
	}

};

template <typename PipelineContextType>
ComputePipeline<PipelineContextType, BasicPipeline<PipelineContextType>> VK_NAMESPACE::PipelineBuilder::
	BuildComputePipeline(const PipelineContextType& context) const
{
	ComputePipelineHandles handles;

	std::vector<vk::PipelineShaderStageCreateInfo> pipelineStages = GetPipelineStages(handles.Info, context);

	// TODO: Directly create the DescriptorWriter...
	auto layoutData = CreatePipelineLayout(context);

	vk::ComputePipelineCreateInfo createInfo{};
	createInfo.setLayout(layoutData.Layout);
	createInfo.setStage(pipelineStages[0]);

	handles.Device = mDevice;
	handles.Handle = mDevice->createComputePipeline(mData->Cache, createInfo).value;
	handles.LayoutData = layoutData;

	for (const auto& resource : layoutData.DescResources)
	{
		handles.SetCache.push_back(resource->Set);
		handles.DynamicOffsets.push_back(0);
	}

	auto Data = mData;

	ComputePipeline<PipelineContextType> pipeline;
	pipeline.mPipelineHandles = Core::CreateRef(handles, [Data](const ComputePipelineHandles& info)
	{
		info.Device->destroyPipeline(info.Handle);
		info.Device->destroyPipelineLayout(info.LayoutData.Layout);
	});

	// Init the Base Pipeline...
	auto DescWriter = DescriptorWriter(mDevice, handles.SetCache);

	pipeline.mPipelineSpecs = std::make_shared<BasicPipelineSpec<PipelineContextType>>(
		std::move(context), std::move(DescWriter));

	FreeShaderModules(pipelineStages);
	return pipeline;
}

template <typename PipelineContextType>
GraphicsPipeline<PipelineContextType, BasicPipeline<PipelineContextType>>
VK_NAMESPACE::PipelineBuilder::BuildGraphicsPipeline(const PipelineContextType& context) const
{
	GraphicsPipelineHandles handles;

	vk::PipelineInputAssemblyStateCreateInfo assemblyInfo = GetAssemblyStateInfo(handles.Info, context);
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo = GetVertexInputStateInfo(handles.Info, context);
	std::vector<vk::PipelineShaderStageCreateInfo> pipelineStages = GetPipelineStages(handles.Info, context);
	vk::PipelineRasterizationStateCreateInfo rasterizer = GetRasterizerStateInfo(handles.Info, context);
	vk::PipelineDepthStencilStateCreateInfo depthStencilState = GetDepthStencilStateInfo(handles.Info, context);
	vk::PipelineMultisampleStateCreateInfo sampleState = GetSampleStateInfo(handles.Info, context);
	vk::PipelineColorBlendStateCreateInfo blendState = GetBlendStateInfo(handles.Info, context);

	GetViewportStateInfo(handles.Info, context);
	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.setViewports(handles.Info.Viewport);
	viewportState.setScissors(handles.Info.Scissor);

	// TODO: Directly create a descriptor writer...
	auto layoutData = CreatePipelineLayout(context);

	vk::GraphicsPipelineCreateInfo createInfo{};

	createInfo.setPInputAssemblyState(&assemblyInfo);
	createInfo.setPVertexInputState(&vertexInputInfo);
	createInfo.setStages(pipelineStages);
	createInfo.setPRasterizationState(&rasterizer);
	createInfo.setPDepthStencilState(&depthStencilState);
	createInfo.setPMultisampleState(&sampleState);
	createInfo.setPColorBlendState(&blendState);
	createInfo.setPViewportState(&viewportState);
	createInfo.setRenderPass(context.GetRenderContext().GetData().RenderPass);
	createInfo.setLayout(layoutData.Layout);
	createInfo.setSubpass(context.GetSubpassIndex());

	handles.Device = mDevice;
	handles.Handle = mDevice->createGraphicsPipeline(mData->Cache, createInfo).value;
	handles.LayoutData = layoutData;

	for (const auto& resource : layoutData.DescResources)
	{
		handles.SetCache.push_back(resource->Set);
		handles.DynamicOffsets.push_back(0);
	}

	auto Data = mData;

	GraphicsPipeline<PipelineContextType> pipeline;
	pipeline.mData = Core::CreateRef(handles, [Data](const GraphicsPipelineHandles& info) 
	{ 
		info.Device->destroyPipeline(info.Handle);
		info.Device->destroyPipelineLayout(info.LayoutData.Layout);
	});

	pipeline.mTargetContext = context.GetRenderContext();

	// Init the Base Pipeline...
	auto DescWriter = DescriptorWriter(mDevice, handles.SetCache);

	pipeline.mPipelineSpecs = std::make_shared<BasicPipelineSpec<PipelineContextType>>(
		context, std::move(DescWriter));

	BufferCreateInfo vertexBufferInfo{};
	vertexBufferInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;
	vertexBufferInfo.Size = 100;
	vertexBufferInfo.Usage = vk::BufferUsageFlagBits::eVertexBuffer;

	pipeline.mVertexBuffer = mMemoryManager.CreateBuffer<typename PipelineContextType::MyVertex>(vertexBufferInfo);

	BufferCreateInfo indexBufferInfo{};
	indexBufferInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;
	indexBufferInfo.Size = 100;
	indexBufferInfo.Usage = vk::BufferUsageFlagBits::eIndexBuffer;

	pipeline.mIndexBuffer = mMemoryManager.CreateBuffer<typename PipelineContextType::MyIndex>(indexBufferInfo);

	FreeShaderModules(pipelineStages);
	delete[] blendState.pAttachments;

	return pipeline;
}

template <typename PipelineContextType> vk::PipelineInputAssemblyStateCreateInfo 
	PipelineBuilder::GetAssemblyStateInfo(
	GraphicsPipelineInfo& Info, const PipelineContextType& config) const
{
	Info.Topology = config.GetPrimitiveTopology();
	vk::PipelineInputAssemblyStateCreateInfo result;
	result.setTopology(Info.Topology);
	return result;
}

template <typename PipelineContextType> 
vk::PipelineVertexInputStateCreateInfo PipelineBuilder::GetVertexInputStateInfo(
	GraphicsPipelineInfo& Info, const PipelineContextType& config) const
{
	Info.VertexInputState = config.GetVertexInputStateInfo();

	vk::PipelineVertexInputStateCreateInfo result;
	result.setVertexAttributeDescriptions(Info.VertexInputState.Attributes);
	result.setVertexBindingDescriptions(Info.VertexInputState.Bindings);

	return result;
}

template <typename PipelineContextType> std::vector <vk::PipelineShaderStageCreateInfo> 
	PipelineBuilder::GetPipelineStages(PipelineInfo& Info, const PipelineContextType& config) const
{
	Info.ShaderStages = config.GetShaderByteCodes();

	std::vector<vk::PipelineShaderStageCreateInfo> result;
	result.reserve(Info.ShaderStages.size());

	for (const auto& stage : Info.ShaderStages)
	{
		vk::ShaderModule Module = Core::Utils::CreateShaderModule(*mDevice, stage);
		result.push_back({ {}, stage.Stage, Module, "main", nullptr, nullptr });
	}

	return result;
}

template <typename PipelineContextType> 
vk::PipelineRasterizationStateCreateInfo PipelineBuilder::GetRasterizerStateInfo(
	GraphicsPipelineInfo& Info, const PipelineContextType& config) const
{
	Info.Rasterizer = config.GetRasterizerStateInfo();
	vk::PipelineRasterizationStateCreateInfo result;

	result.setCullMode(Info.Rasterizer.CullMode);
	result.setLineWidth(Info.Rasterizer.LineWidth);
	result.setFrontFace(Info.Rasterizer.FrontFace);
	result.setPolygonMode(Info.Rasterizer.PolygonMode);

	result.setDepthBiasClamp(0.0f);
	result.setDepthBiasConstantFactor(0.0f);
	result.setDepthBiasEnable(false);
	result.setDepthBiasSlopeFactor(0.0f);
	result.setDepthClampEnable(VK_FALSE);

	return result;
}

template <typename PipelineContextType>
vk::PipelineDepthStencilStateCreateInfo VK_NAMESPACE::PipelineBuilder::GetDepthStencilStateInfo(
	GraphicsPipelineInfo& Info, const PipelineContextType& config) const
{
	Info.DepthBufferingInfo = config.GetDepthStencilStateInfo();

	vk::PipelineDepthStencilStateCreateInfo state{};
	state.setDepthTestEnable(Info.DepthBufferingInfo.DepthTestEnable);
	state.setDepthCompareOp(Info.DepthBufferingInfo.DepthCompareOp);
	state.setDepthWriteEnable(Info.DepthBufferingInfo.DepthWriteEnable);
	state.setDepthBoundsTestEnable(Info.DepthBufferingInfo.DepthBoundsTestEnable);
	state.setMinDepthBounds(Info.DepthBufferingInfo.MinDepthBounds);
	state.setMaxDepthBounds(Info.DepthBufferingInfo.MaxDepthBounds);
	state.setStencilTestEnable(Info.DepthBufferingInfo.StencilTestEnable);
	state.setFront(Info.DepthBufferingInfo.StencilFrontOp);
	state.setBack(Info.DepthBufferingInfo.StencilBackOp);
	state.setStencilTestEnable(Info.DepthBufferingInfo.StencilTestEnable);

	return state;
}

template <typename PipelineContextType> 
vk::PipelineMultisampleStateCreateInfo PipelineBuilder::GetSampleStateInfo(
	GraphicsPipelineInfo& Info, const PipelineContextType& config) const
{
	vk::PipelineMultisampleStateCreateInfo result;
	result.setSampleShadingEnable(VK_TRUE);
	result.setAlphaToCoverageEnable(VK_FALSE);
	result.setAlphaToOneEnable(VK_FALSE);
	result.setMinSampleShading(0.0f);
	result.setRasterizationSamples(vk::SampleCountFlagBits::e1);
	result.setPSampleMask(nullptr);

	return result;
}

template <typename PipelineContextType> 
vk::PipelineColorBlendStateCreateInfo PipelineBuilder::GetBlendStateInfo(
	GraphicsPipelineInfo& Info, const PipelineContextType& config) const
{
	vk::PipelineColorBlendAttachmentState* attachmentState =
		new vk::PipelineColorBlendAttachmentState[1];
	attachmentState->setAlphaBlendOp(vk::BlendOp::eAdd);
	attachmentState->setBlendEnable(false);
	attachmentState->setColorBlendOp(vk::BlendOp::eAdd);
	attachmentState->setColorWriteMask(
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

	attachmentState->setDstAlphaBlendFactor(vk::BlendFactor::eDstAlpha);
	attachmentState->setDstColorBlendFactor(vk::BlendFactor::eDstColor);
	attachmentState->setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha);
	attachmentState->setSrcColorBlendFactor(vk::BlendFactor::eSrcColor);

	vk::PipelineColorBlendStateCreateInfo result;
	result.setPAttachments(attachmentState);
	result.setAttachmentCount(1);
	result.setLogicOp(vk::LogicOp::eCopy);
	result.setLogicOpEnable(false);
	result.setBlendConstants({ 1.0f, 1.0f, 1.0f, 1.0f });

	return result;
}

template <typename PipelineContextType> 
void PipelineBuilder::GetViewportStateInfo(
	GraphicsPipelineInfo& Info, const PipelineContextType& config) const
{
	auto [Viewport, Scissor] = config.GetViewportAndScissorInfo();
	Info.Viewport = Viewport;
	Info.Scissor = Scissor;
}

template <typename PipelineContextType> 
PipelineLayoutData PipelineBuilder::CreatePipelineLayout(const PipelineContextType& config) const
{
	Core::DescriptorSetAllocator DescAllocator = mDescPoolManager.FetchAllocator({});
	std::vector<Core::Ref<DescriptorResource>> resources;

	std::vector<vk::DescriptorSetLayout> setLayouts;

	auto [setLayoutInfos, pushConstantInfos] = config.GetPipelineLayoutInfo();

	setLayouts.resize(setLayoutInfos.size());
	resources.resize(setLayoutInfos.size());

	for (auto& setInfo : setLayoutInfos)
	{
		Core::Ref<DescriptorResource> setResource = DescAllocator.Allocate(setInfo.second);
		resources[setInfo.first] = setResource;
		setLayouts[setInfo.first] = setResource->Layout;
	}

	vk::PipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.setSetLayouts(setLayouts);
	layoutInfo.setPushConstantRanges(pushConstantInfos);

	vk::PipelineLayout layout = mDevice->createPipelineLayout(layoutInfo);

	return { layout, resources, pushConstantInfos };
}

VK_END

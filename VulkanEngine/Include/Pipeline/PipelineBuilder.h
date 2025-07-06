#pragma once
#include "../Core/Config.h"
#include "../Core/Ref.h"

#include "PShader.h"
#include "BasicPipeline.h"
#include "GraphicsPipeline.h"
#include "ComputePipeline.h"

#include "../Descriptors/DescriptorWriter.h"
#include "../Descriptors/DescriptorPoolManager.h"

#include "../Memory/ResourcePool.h"

VK_BEGIN

// NOTE: This Builder only works with BasePipeline = BasicPipeline<PipelineContextType>
// TODO: we might need to invoke a callback function at the time of pipeline creation

class PipelineBuilder
{
public:
	template <typename UGraphicsPipeline, typename ...Args>
	UGraphicsPipeline BuildGraphicsPipeline(Args&&... args) const;

	template <typename UComputePipeline, typename ...Args>
	UComputePipeline BuildComputePipeline(Args&&... args) const;

	// TODO: Implement ray tracing pipeline creation

	/*template <typename URayTracingPipeline, typename ...Args>
	URayTracingPipeline BuildRayTracingPipeline(const RayTracingPipelineConfig& shader, Args&&... config) const;*/

private:
	Core::Ref<vk::Device> mDevice;

	Core::Ref<PipelineBuilderData> mData;
	ResourcePool mMemoryManager;
	DescriptorPoolManager mDescPoolManager;

private:
	// Helper functions...

	//template <typename UComputePipeline, typename ...Args>
	//void BuildComputePipeline(UComputePipeline& pipeline, const PShader& shader, Args&&... args) const;

	friend class Context;

private:
	// Helper methods...
	inline vk::PipelineInputAssemblyStateCreateInfo GetAssemblyStateInfo(const GraphicsPipelineConfig& config) const;
	inline vk::PipelineVertexInputStateCreateInfo GetVertexInputStateInfo(const GraphicsPipelineConfig& config) const;

	inline std::vector<vk::PipelineShaderStageCreateInfo> GetPipelineStages(
		PipelineInfo& Info, const PShader& shader) const;

	inline vk::PipelineRasterizationStateCreateInfo GetRasterizerStateInfo(const GraphicsPipelineConfig& config) const;
	inline vk::PipelineDepthStencilStateCreateInfo GetDepthStencilStateInfo(const GraphicsPipelineConfig& config) const;
	inline vk::PipelineMultisampleStateCreateInfo GetSampleStateInfo(const GraphicsPipelineConfig& config) const;
	inline vk::PipelineColorBlendStateCreateInfo GetBlendStateInfo(const GraphicsPipelineConfig& config) const;
	inline PipelineLayoutData CreatePipelineLayout(const PShader& shader) const;

	inline void FreeShaderModules(const std::vector<vk::PipelineShaderStageCreateInfo>& shaders) const;

};

template <typename UComputePipeline, typename... Args>
UComputePipeline VK_NAMESPACE::PipelineBuilder::BuildComputePipeline(Args&& ...args) const
{
	UComputePipeline pipeline(std::forward<Args>(args)...);

	const PShader& shader = pipeline.GetShader();

	ComputePipelineHandles handles;
	handles.WorkGroupSize = shader.mWorkGroupSize;

	std::vector<vk::PipelineShaderStageCreateInfo> pipelineStages = GetPipelineStages(handles, shader);

	auto layoutData = CreatePipelineLayout(shader);

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

	pipeline.mHandles = Core::CreateRef(handles, [Data](const ComputePipelineHandles& info)
	{
		info.Device->destroyPipeline(info.Handle);
		info.Device->destroyPipelineLayout(info.LayoutData.Layout);
	});

	// Init the Base Pipeline...
	auto DescWriter = DescriptorWriter(mDevice, handles.SetCache);

	pipeline.mPipelineSpecs = std::make_shared<BasicPipelineSpec>(std::move(DescWriter));

	FreeShaderModules(pipelineStages);
	return pipeline;
}

template <typename UGraphicsPipeline, typename ...Args>
UGraphicsPipeline VK_NAMESPACE::PipelineBuilder::
	BuildGraphicsPipeline(Args&&... args) const
{
	UGraphicsPipeline pipeline(std::forward<Args>(args)...);

	GraphicsPipelineHandles handles;

	const GraphicsPipelineConfig& pConfig = pipeline.GetConfig();
	const PShader& pShader = pipeline.GetShader();

	vk::PipelineInputAssemblyStateCreateInfo assemblyInfo = GetAssemblyStateInfo(pConfig);
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo = GetVertexInputStateInfo(pConfig);
	std::vector<vk::PipelineShaderStageCreateInfo> pipelineStages = GetPipelineStages(handles, pShader);
	vk::PipelineRasterizationStateCreateInfo rasterizer = GetRasterizerStateInfo(pConfig);
	vk::PipelineDepthStencilStateCreateInfo depthStencilState = GetDepthStencilStateInfo(pConfig);
	vk::PipelineMultisampleStateCreateInfo sampleState = GetSampleStateInfo(pConfig);
	vk::PipelineColorBlendStateCreateInfo blendState = GetBlendStateInfo(pConfig);

	vk::PipelineViewportStateCreateInfo viewportState{};
	viewportState.setViewports(pConfig.CanvasView);
	viewportState.setScissors(pConfig.CanvasScissor);

	auto layoutData = CreatePipelineLayout(pShader);

	vk::GraphicsPipelineCreateInfo hRawCreateInfo{};

	hRawCreateInfo.setPInputAssemblyState(&assemblyInfo);
	hRawCreateInfo.setStages(pipelineStages);
	hRawCreateInfo.setPVertexInputState(&vertexInputInfo);
	hRawCreateInfo.setPRasterizationState(&rasterizer);
	hRawCreateInfo.setPDepthStencilState(&depthStencilState);
	hRawCreateInfo.setPMultisampleState(&sampleState);
	hRawCreateInfo.setPColorBlendState(&blendState);
	hRawCreateInfo.setPViewportState(&viewportState);
	hRawCreateInfo.setRenderPass(pConfig.TargetContext.GetData().RenderPass);
	hRawCreateInfo.setLayout(layoutData.Layout);
	hRawCreateInfo.setSubpass(pConfig.SubpassIndex);

	handles.Device = mDevice;
	handles.Handle = mDevice->createGraphicsPipeline(mData->Cache, hRawCreateInfo).value;
	handles.LayoutData = layoutData;
	handles.TargetContext = pConfig.TargetContext;

	for (const auto& resource : layoutData.DescResources)
	{
		handles.SetCache.push_back(resource->Set);
		handles.DynamicOffsets.push_back(0);
	}

	auto hData = mData;

	pipeline.mHandles = Core::CreateRef(handles, [hData](const GraphicsPipelineHandles& info) 
	{ 
		info.Device->destroyPipeline(info.Handle);
		info.Device->destroyPipelineLayout(info.LayoutData.Layout);
	});

	// Init the Base Pipeline...
	auto DescWriter = DescriptorWriter(mDevice, handles.SetCache);

	pipeline.mPipelineSpecs = std::make_shared<BasicPipelineSpec>(std::move(DescWriter));

	FreeShaderModules(pipelineStages);
	delete[] blendState.pAttachments;

	return pipeline;
}

vk::PipelineInputAssemblyStateCreateInfo 
	PipelineBuilder::GetAssemblyStateInfo(const GraphicsPipelineConfig& config) const
{
	vk::PipelineInputAssemblyStateCreateInfo result;
	result.setTopology(config.Topology);
	return result;
}

vk::PipelineVertexInputStateCreateInfo PipelineBuilder::GetVertexInputStateInfo(const GraphicsPipelineConfig& config) const
{
	vk::PipelineVertexInputStateCreateInfo result;
	result.setVertexAttributeDescriptions(config.VertexInput.Attributes);
	result.setVertexBindingDescriptions(config.VertexInput.Bindings);

	return result;
}

std::vector <vk::PipelineShaderStageCreateInfo>
	PipelineBuilder::GetPipelineStages(PipelineInfo& Info, const PShader& shader) const
{
	Info.ShaderStages = shader.GetShaderByteCodes();

	std::vector<vk::PipelineShaderStageCreateInfo> result;
	result.reserve(Info.ShaderStages.size());

	for (const auto& stage : Info.ShaderStages)
	{
		vk::ShaderModule Module = Core::Utils::CreateShaderModule(*mDevice, stage);
		result.push_back({ {}, stage.Stage, Module, "main", nullptr, nullptr });
	}

	return result;
}

vk::PipelineRasterizationStateCreateInfo PipelineBuilder::GetRasterizerStateInfo(const GraphicsPipelineConfig& config) const
{
	vk::PipelineRasterizationStateCreateInfo result;

	result.setCullMode(config.Rasterizer.CullMode);
	result.setLineWidth(config.Rasterizer.LineWidth);
	result.setFrontFace(config.Rasterizer.FrontFace);
	result.setPolygonMode(config.Rasterizer.PolygonMode);

	result.setDepthBiasClamp(0.0f);
	result.setDepthBiasConstantFactor(0.0f);
	result.setDepthBiasEnable(false);
	result.setDepthBiasSlopeFactor(0.0f);
	result.setDepthClampEnable(VK_FALSE);

	return result;
}

vk::PipelineDepthStencilStateCreateInfo VK_NAMESPACE::PipelineBuilder::GetDepthStencilStateInfo(
	const GraphicsPipelineConfig& Info) const
{
	vk::PipelineDepthStencilStateCreateInfo state{};
	state.setDepthTestEnable(Info.DepthBufferingState.DepthTestEnable);
	state.setDepthCompareOp(Info.DepthBufferingState.DepthCompareOp);
	state.setDepthWriteEnable(Info.DepthBufferingState.DepthWriteEnable);
	state.setDepthBoundsTestEnable(Info.DepthBufferingState.DepthBoundsTestEnable);
	state.setMinDepthBounds(Info.DepthBufferingState.MinDepthBounds);
	state.setMaxDepthBounds(Info.DepthBufferingState.MaxDepthBounds);
	state.setStencilTestEnable(Info.DepthBufferingState.StencilTestEnable);
	state.setFront(Info.DepthBufferingState.StencilFrontOp);
	state.setBack(Info.DepthBufferingState.StencilBackOp);
	state.setStencilTestEnable(Info.DepthBufferingState.StencilTestEnable);

	return state;
}

vk::PipelineMultisampleStateCreateInfo PipelineBuilder::GetSampleStateInfo(const GraphicsPipelineConfig& config) const
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

vk::PipelineColorBlendStateCreateInfo PipelineBuilder::GetBlendStateInfo(const GraphicsPipelineConfig& config) const
{
	// NOTE: For now, the user has no control over blending functions

	uint32_t count = config.TargetContext.GetColorAttachmentCount();

	vk::PipelineColorBlendAttachmentState attachmentState{};
	attachmentState.setAlphaBlendOp(vk::BlendOp::eAdd);
	attachmentState.setBlendEnable(false);
	attachmentState.setColorBlendOp(vk::BlendOp::eAdd);
	attachmentState.setColorWriteMask(
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

	attachmentState.setDstAlphaBlendFactor(vk::BlendFactor::eDstAlpha);
	attachmentState.setDstColorBlendFactor(vk::BlendFactor::eDstColor);
	attachmentState.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha);
	attachmentState.setSrcColorBlendFactor(vk::BlendFactor::eSrcColor);

	vk::PipelineColorBlendStateCreateInfo result;

	// Setting the attachments...

	vk::PipelineColorBlendAttachmentState* attachments = new vk::PipelineColorBlendAttachmentState[count];

	for (uint32_t i = 0; i < count; i++)
		attachments[i] = attachmentState;

	result.setPAttachments(attachments);
	result.setAttachmentCount(count);
	result.setLogicOp(vk::LogicOp::eCopy);
	result.setLogicOpEnable(false);
	result.setBlendConstants({ 1.0f, 1.0f, 1.0f, 1.0f });

	return result;
}

PipelineLayoutData PipelineBuilder::CreatePipelineLayout(const PShader& shader) const
{
	Core::DescriptorSetAllocator DescAllocator = mDescPoolManager.FetchAllocator({});
	std::vector<Core::Ref<DescriptorResource>> resources;

	std::vector<vk::DescriptorSetLayout> setLayouts;

	auto [setLayoutInfos, pushConstantInfos] = shader.GetPipelineLayoutInfo();

	setLayouts.resize(setLayoutInfos.size());
	resources.resize(setLayoutInfos.size());

	for (auto& setInfo : setLayoutInfos)
	{
		// FIX ME: Doesn't work if sets aren't in consecutive order...
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

void PipelineBuilder::FreeShaderModules(const std::vector<vk::PipelineShaderStageCreateInfo>& shaders) const
{
	for (const auto& shader : shaders)
		mDevice->destroyShaderModule(shader.module);
}

VK_END

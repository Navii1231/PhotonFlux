#include "Core/Aqpch.h"
#include "DeferredRenderer/Pipelines/DeferredPipeline.h"

#define FIX_VERTEX_INPUTS   1

using TagImageAttachmentMap = std::unordered_map<std::string, std::tuple<vk::Format, vk::ImageLayout, vk::ImageUsageFlagBits>>;
using ImageAttachmentTagMap = std::unordered_map<vk::Format, std::string>;

static const TagImageAttachmentMap sTypeToFormat =
{
	{"r8i", { vk::Format::eR8Sint, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rg8i", { vk::Format::eR8G8Sint, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgb8i", { vk::Format::eR8G8B8Sint, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgba8i", { vk::Format::eR8G8B8A8Sint, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"r8un", { vk::Format::eR8Unorm, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rg8un", { vk::Format::eR8G8Unorm, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgb8un", { vk::Format::eR8G8B8Unorm, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgba8un", { vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},

	{"r16i", {vk::Format::eR16Sint, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rg16i", {vk::Format::eR16G16Sint, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgb16i", {vk::Format::eR16G16B16Sint, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgba16i", {vk::Format::eR16G16B16A16Sint, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"r16f", {vk::Format::eR16Sfloat, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rg16f", {vk::Format::eR16G16Sfloat, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgb16f", {vk::Format::eR16G16B16Sfloat, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgba16f", {vk::Format::eR16G16B16A16Sfloat, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"r16un", {vk::Format::eR16Unorm, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rg16un", {vk::Format::eR16G16Unorm, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgb16un", {vk::Format::eR16G16B16Unorm, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgba16un", {vk::Format::eR16G16B16A16Unorm, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},

	{"r32i", {vk::Format::eR32Sint, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rg32i", {vk::Format::eR32G32Sint, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgb32i", {vk::Format::eR32G32B32Sint, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgba32i", {vk::Format::eR32G32B32A32Sint, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"r32f", {vk::Format::eR32Sfloat, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rg32f", {vk::Format::eR32G32Sfloat, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgb32f", {vk::Format::eR32G32B32Sfloat, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},
	{"rgba32f", {vk::Format::eR32G32B32A32Sfloat, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment}},

	{"d16un", {vk::Format::eD16Unorm, vk::ImageLayout::eDepthAttachmentOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment}},
	{"x8d24un_pack32", {vk::Format::eX8D24UnormPack32, vk::ImageLayout::eDepthAttachmentOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment}},
	{"d32f", {vk::Format::eD32Sfloat, vk::ImageLayout::eDepthAttachmentOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment}},
	{"d16un_s8u", {vk::Format::eD16UnormS8Uint, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment}},
	{"d24un_s8u", {vk::Format::eD24UnormS8Uint, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment}},
	{"d32f_s8u", {vk::Format::eD32SfloatS8Uint, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment}},
	{"s8u", {vk::Format::eS8Uint, vk::ImageLayout::eStencilAttachmentOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment} },
};

ImageAttachmentTagMap CreateInverseMap(const TagImageAttachmentMap& map)
{
	ImageAttachmentTagMap ret;
	for (const TagImageAttachmentMap::value_type& value : map)
		ret[std::get<0>(value.second)] = value.first;
	return ret;
}

static const ImageAttachmentTagMap sFormatToString = CreateInverseMap(sTypeToFormat);

std::tuple<vk::Format, vk::ImageLayout, vk::ImageUsageFlags> AQUA_NAMESPACE::ConvertIntoImageAttachment(const std::string& tag)
{
	// TODO: may need to write a parser as the tag grows more complex
	// Tag layout: [name][precision][type]

	if (tag.empty())
		return { vk::Format(), vk::ImageLayout(), vk::ImageUsageFlags() };

	int idx = 0;

	while (tag[idx] != 0)
	{
		if (tag[idx++] == '_' /*color attachment*/)
		{
			std::string remaining = tag.substr(idx);
			return sTypeToFormat.at(remaining);
		}
	}

	return { vk::Format(), vk::ImageLayout(), vk::ImageUsageFlags() };
}

std::string AQUA_NAMESPACE::GetTagName(const std::string& tag)
{
	auto found = std::find(tag.begin() + 1, tag.end(), '_');

	if (found != tag.end())
		return tag.substr(1, found - tag.begin() - 1);

	return "";
}

std::string AQUA_NAMESPACE::GetImageAttachmentInfoString(vk::Format format)
{
	return sFormatToString.at(format);
}

AQUA_NAMESPACE::DeferredPipeline::DeferredPipeline(const DeferredPipelineCreateInfo& createInfo) 
	: mVulkanCtx(createInfo.Ctx)
{
	mDeferredCtx = std::make_shared<DeferredPipelineContext>();

	SetupPipeline(createInfo);

	auto mma = mVulkanCtx.CreateResourcePool();

	mDeferredCtx->SharedModels = mma.CreateBuffer<glm::mat4>(
		vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostCoherent);

	mDeferredCtx->Models = mma.CreateBuffer<glm::mat4>(
		vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

	mDeferredCtx->Camera = mma.CreateBuffer<CameraInfo>(
		vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostCoherent);

	// Also, setting the default samplers
	vkEngine::SamplerInfo info{};

	auto defaultSampler = mma.CreateSampler(info);

	for (auto& [name, resource] : mDeferredCtx->VertexResources)
	{
		if (name == TAG_DEPTH_STENCIL)
			continue;

		resource.Image.SetSampler(defaultSampler);
	}
}

void AQUA_NAMESPACE::DeferredPipeline::CreateFramebuffer(uint32_t width, uint32_t height)
{
	SetFramebuffer(mDeferredCtx->PipelineConfig.TargetContext.CreateFramebuffer(width, height));

	auto cAttachs = GetFramebuffer().GetColorAttachments();

	for (size_t i = 0; i < cAttachs.size(); i++)
	{
		mDeferredCtx->VertexResources[mDeferredCtx->vLayout[i]].Image = cAttachs[i];
	}

	// TODO: the name could be different than TAG_DEPTH_STENCIL
	//if(mDeferredCtx->VertexResources.find(TAG_DEPTH_STENCIL) != mDeferredCtx->VertexResources.end())
	mDeferredCtx->VertexResources[TAG_DEPTH_STENCIL].Image = GetFramebuffer().GetDepthStencilAttachment();
}

void AQUA_NAMESPACE::DeferredPipeline::SetupBasicConfig(const glm::uvec2& scrSize)
{
	mDeferredCtx->PipelineConfig.CanvasScissor = vk::Rect2D(scrSize.x, scrSize.y);
	mDeferredCtx->PipelineConfig.CanvasView = vk::Viewport(0.0f, 0.0f, (float) scrSize.x, (float) scrSize.y, 0.0f, 1.0f);

	mDeferredCtx->PipelineConfig.IndicesType = vk::IndexType::eUint32;

	mDeferredCtx->PipelineConfig.VertexInput = ConstructSOAVertexInput();

	mDeferredCtx->PipelineConfig.SubpassIndex = 0;

	mDeferredCtx->PipelineConfig.DepthBufferingState.DepthBoundsTestEnable = false;
	mDeferredCtx->PipelineConfig.DepthBufferingState.DepthCompareOp = vk::CompareOp::eLess;
	mDeferredCtx->PipelineConfig.DepthBufferingState.DepthTestEnable = true;
	mDeferredCtx->PipelineConfig.DepthBufferingState.DepthWriteEnable = true;
	mDeferredCtx->PipelineConfig.DepthBufferingState.MaxDepthBounds = 1.0f;
	mDeferredCtx->PipelineConfig.DepthBufferingState.MinDepthBounds = 0.0f;
	mDeferredCtx->PipelineConfig.DepthBufferingState.StencilTestEnable = false;

	mDeferredCtx->PipelineConfig.Rasterizer.CullMode = vk::CullModeFlagBits::eBack;
	mDeferredCtx->PipelineConfig.Rasterizer.FrontFace = vk::FrontFace::eCounterClockwise;
	mDeferredCtx->PipelineConfig.Rasterizer.LineWidth = 0.01f;
	mDeferredCtx->PipelineConfig.Rasterizer.PolygonMode = vk::PolygonMode::eFill;
}

vkEngine::RenderContextCreateInfo AQUA_NAMESPACE::DeferredPipeline::CreateRenderCtxInfo()
{
	vkEngine::RenderContextCreateInfo ctxCreateInfo{};

	vkEngine::ImageAttachmentInfo attachInfo{};
	attachInfo.LoadOp = vk::AttachmentLoadOp::eLoad;
	attachInfo.StoreOp = vk::AttachmentStoreOp::eStore;
	attachInfo.Samples = vk::SampleCountFlagBits::e1;
	attachInfo.Usage = vk::ImageUsageFlags(); // Can take user parameters
	attachInfo.StencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachInfo.StencilStoreOp = vk::AttachmentStoreOp::eDontCare;

	for (const auto& tag : mDeferredCtx->vLayout)
	{
		auto[format, layout, usage] = ConvertIntoImageAttachment(tag);

		attachInfo.Format = format;
		attachInfo.Layout = layout;
		attachInfo.Usage = usage | (tag != TAG_DEPTH_STENCIL ?
			vk::ImageUsageFlagBits::eSampled : vk::ImageUsageFlags());

		ctxCreateInfo.Attachments.push_back(attachInfo);
	}

	ctxCreateInfo.UsingDepthAttachment = true;
	ctxCreateInfo.UsingStencilAttachment = true;

	return ctxCreateInfo;
}

void AQUA_NAMESPACE::DeferredPipeline::CreateCopyIdxPipeline(const vkEngine::PShader& shader)
{
	auto pBuilder = mVulkanCtx.MakePipelineBuilder();

	mDeferredCtx->CopyIdx = pBuilder.BuildComputePipeline<CopyIdxPipeline>(shader);

	uint32_t queueIdx = 0;

	mDeferredCtx->Exec = mVulkanCtx.GetQueueManager()->FetchExecutor(queueIdx, vkEngine::QueueAccessType::eWorker);
	mDeferredCtx->Alloc = mVulkanCtx.CreateCommandPools()[queueIdx];

	mDeferredCtx->Cmds = mDeferredCtx->Alloc.Allocate();
}

void AQUA_NAMESPACE::DeferredPipeline::SubmitRenderable(const DeferredRenderable& renderable) const
{
	// vertex copying
	mDeferredCtx->SharedModels << renderable.ModelTransform;

	// SoA vertex input
	for (auto& [name, resource] : mDeferredCtx->VertexResources)
	{
		if (name == TAG_DEPTH_STENCIL)
			continue;

		auto buffer = resource.Buffer;
		auto tagName = GetTagName(name);

		buffer.Resize(buffer.GetSize() + renderable.mVertexBuffers.at(tagName).GetSize());

		// using GPU to copy the memory...
		vkEngine::CopyBuffer(buffer, renderable.mVertexBuffers.at(tagName));
	}

	// Index copying...
	vk::CommandBuffer cmd = mDeferredCtx->Cmds;
	CopyIdxPipeline copyPipeline = mDeferredCtx->CopyIdx;

	size_t idxCount = renderable.Info.Mesh.GetIndexCount();

	glm::uvec3 workGrps = glm::uvec3(idxCount, 1, 1) / copyPipeline.GetWorkGroupSize();

	copyPipeline.mIdxBuf.Resize(copyPipeline.mIdxBuf.GetSize() + renderable.mIndexBuffer.GetSize());
	copyPipeline.mSrcIdx = renderable.mIndexBuffer;

	copyPipeline.UpdateDescriptors();

	cmd.reset();
	cmd.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

	copyPipeline.Begin(mDeferredCtx->Cmds);

	copyPipeline.BindPipeline();
	copyPipeline.SetShaderConstant("eCompute.ShaderConstants.Index_0", static_cast<uint32_t>(mDeferredCtx->VertexCount));
	copyPipeline.SetShaderConstant("eCompute.ShaderConstants.Index_1", static_cast<uint32_t>(idxCount));

	copyPipeline.Dispatch(workGrps);

	copyPipeline.End();

	cmd.end({});

	uint32_t queueIdx = mDeferredCtx->Exec.SubmitWork(cmd);
	mDeferredCtx->Exec[queueIdx]->WaitIdle();
}

void AQUA_NAMESPACE::DeferredPipeline::SetSampler(const std::string& tag, vkEngine::Core::Ref<vk::Sampler> sampler)
{
	if (mDeferredCtx->VertexResources.find(tag) != mDeferredCtx->VertexResources.end())
		mDeferredCtx->VertexResources[tag].Image.SetSampler(sampler);
}

void AQUA_NAMESPACE::DeferredPipeline::UpdateDescriptors()
{
	// For now, we only have camera and model matrices

	// NOTE: Optional
	mDeferredCtx->Models.Clear();
	mDeferredCtx->Models.Resize(mDeferredCtx->SharedModels.GetSize());
	
	vkEngine::CopyBuffer(mDeferredCtx->Models, mDeferredCtx->SharedModels);

	vkEngine::DescriptorWriter& writer = this->GetDescriptorWriter();

	vkEngine::UniformBufferWriteInfo uniformInfo{};
	uniformInfo.Buffer = mDeferredCtx->Camera.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, uniformInfo);

	vkEngine::StorageBufferWriteInfo storageInfo{};
	storageInfo.Buffer = mDeferredCtx->Models.GetNativeHandles().Handle;

	writer.Update({ 0, 1, 0 }, storageInfo);
}

void AQUA_NAMESPACE::DeferredPipeline::Cleanup()
{
	mDeferredCtx->VertexCount = 0;

	for (auto& [name, resource] : mDeferredCtx->VertexResources)
	{
		auto buffer = resource.Buffer;
		buffer.Clear();
	}

	GetIndexBuffer().Clear();
}

void AQUA_NAMESPACE::DeferredPipeline::BindVertexBuffers(vk::CommandBuffer commandBuffer) const
{
	std::vector<vk::DeviceSize> offsets;
	std::vector<vk::Buffer> buffers;

	offsets.reserve(mDeferredCtx->VertexResources.size());
	buffers.reserve(mDeferredCtx->VertexResources.size());

	for (auto& [name, resource] : mDeferredCtx->VertexResources)
	{
		if (name == TAG_DEPTH_STENCIL)
			continue;

		auto buffer = resource.Buffer;
		buffers.push_back(buffer.GetNativeHandles().Handle);
		offsets.push_back(0);
	}

	commandBuffer.bindVertexBuffers(static_cast<uint32_t>(0), buffers, offsets);
}

void AQUA_NAMESPACE::DeferredPipeline::BindIndexBuffer(vk::CommandBuffer commandBuffer) const
{
	commandBuffer.bindIndexBuffer(GetIndexBuffer().GetNativeHandles().Handle, 0, vk::IndexType::eUint32);
}

void AQUA_NAMESPACE::DeferredPipeline::SetupPipeline(const DeferredPipelineCreateInfo& createInfo)
{
	this->SetShader(createInfo.DeferShader);

	// Generating pipeline config
	SetupBasicConfig(createInfo.ScrSize);

	auto rcb = mVulkanCtx.FetchRenderContextBuilder(vk::PipelineBindPoint::eGraphics);

	mDeferredCtx->PipelineConfig.TargetContext = rcb.MakeContext(CreateRenderCtxInfo());

	CreateFramebuffer(static_cast<uint32_t>(createInfo.ScrSize.x), static_cast<uint32_t>(createInfo.ScrSize.y));

	CreateCopyIdxPipeline(createInfo.IdxCpyShader);
}

VK_NAMESPACE::VertexInputDesc AQUA_NAMESPACE::DeferredPipeline::ConstructSOAVertexInput() const
{
	vkEngine::VertexInputDesc desc;

	// TODO: for now vertex bindings are fixed
	_STL_ASSERT(FIX_VERTEX_INPUTS, "Vertex inputs are fixed!");

	desc.Bindings.emplace_back(0, (uint32_t) sizeof(glm::vec4));
	desc.Bindings.emplace_back(1, (uint32_t) sizeof(glm::vec4));
	desc.Bindings.emplace_back(2, (uint32_t) sizeof(glm::vec4));
	desc.Bindings.emplace_back(3, (uint32_t) sizeof(glm::vec4));
	desc.Bindings.emplace_back(4, (uint32_t) sizeof(glm::vec3));
	desc.Bindings.emplace_back(5, (uint32_t) sizeof(glm::vec2));

	desc.Attributes.emplace_back(0, 0, vk::Format::eR32G32B32A32Sfloat, 0);
	desc.Attributes.emplace_back(1, 1, vk::Format::eR32G32B32A32Sfloat, 0);
	desc.Attributes.emplace_back(2, 2, vk::Format::eR32G32B32A32Sfloat, 0);
	desc.Attributes.emplace_back(3, 3, vk::Format::eR32G32B32A32Sfloat, 0);
	desc.Attributes.emplace_back(4, 4, vk::Format::eR32G32B32Sfloat, 0);
	desc.Attributes.emplace_back(5, 5, vk::Format::eR32G32Sfloat, 0);

	return desc;
}


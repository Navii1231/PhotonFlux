#pragma once
#include "../Application/Application.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <unordered_map>

#include "Device/Device.h"
#include "Process/Executor.h"
#include "ShaderCompiler/ShaderCompiler.h"

#include "Window/GLFW_Window.h"

#include <array>

struct Vertex
{
	glm::vec3 aPosition;
	glm::vec3 aColor;

	static std::vector<vk::VertexInputAttributeDescription> GetAttribs()
	{
		std::vector<vk::VertexInputAttributeDescription> Descs;

		Descs.emplace_back(0, 0, vk::Format::eR32G32B32A32Sfloat, 0);
		Descs.emplace_back(1, 0, vk::Format::eR32G32B32A32Sfloat, (uint32_t) sizeof(glm::vec3));

		return Descs;
	}

	static std::vector<vk::VertexInputBindingDescription> GetBindings()
	{
		std::vector<vk::VertexInputBindingDescription> Descs;
		Descs.emplace_back(0, (uint32_t) sizeof(Vertex));

		return Descs;
	}
};

struct CameraUniform
{
	glm::mat4 Projection;
	glm::mat4 View;
};

struct MyPipelineConfig : public vkEngine::GraphicsPipelineSettingsBase
	<std::pair<std::vector<Vertex>, std::vector<uint32_t>>, Vertex, uint32_t>
{
	MyPipelineConfig(vkEngine::RenderContext context, float width, float height)
	{
		mBasicConfig.CanvasView.setWidth(width);
		mBasicConfig.CanvasView.setHeight(height);
		mBasicConfig.CanvasView.setX(0.0f);
		mBasicConfig.CanvasView.setY(0.0f);
		mBasicConfig.CanvasView.setMinDepth(0.0f);
		mBasicConfig.CanvasView.setMaxDepth(1.0f);

		mBasicConfig.CanvasScissor.setOffset(vk::Offset2D(0, 0));
		mBasicConfig.CanvasScissor.setExtent(vk::Extent2D((uint32_t) width, (uint32_t) height));

		mBasicConfig.TargetContext = context;

		mBasicConfig.VertexInput.Attributes = Vertex::GetAttribs();
		mBasicConfig.VertexInput.Bindings = Vertex::GetBindings();

		mBasicConfig.DepthBufferingState.DepthTestEnable = true;
		mBasicConfig.DepthBufferingState.DepthWriteEnable = true;
		mBasicConfig.DepthBufferingState.DepthCompareOp = vk::CompareOp::eLess;

		mRenderTarget = context.CreateFramebuffer((uint32_t) width, (uint32_t) height);
	}

	virtual vkEngine::Framebuffer GetFramebuffer() const override
	{
		return mRenderTarget;
	}

	virtual void CopyVertices(Vertex* Begin, Vertex* End, const Renderable& renderable) const override
	{
		for (size_t i = 0; i < renderable.first.size(); i++)
		{
			Begin[i] = renderable.first[i];
		}
	}

	virtual void CopyIndices(Index* Begin, Index* End, const Renderable& renderable, uint32_t offset) const override
	{
		for (size_t i = 0; i < renderable.second.size(); i++)
		{
			Begin[i] = renderable.second[i] + offset;
		}
	}

	virtual size_t GetVertexCount(const Renderable& renderable) const override { return renderable.first.size(); }
	virtual size_t GetIndexCount(const Renderable& renderable) const override { return renderable.second.size(); }

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override
	{
		vkEngine::UniformBufferWriteInfo writeInfo{};
		writeInfo.Buffer = mCameraUniform.GetNativeHandles().Handle;

		writer.Update({ 0, 0, 0 }, writeInfo);
	}

	// Fields...
	vkEngine::Framebuffer mRenderTarget;
	vkEngine::Buffer<CameraUniform> mCameraUniform;
	vkEngine::Core::Ref<vk::Device> mDevice;
};


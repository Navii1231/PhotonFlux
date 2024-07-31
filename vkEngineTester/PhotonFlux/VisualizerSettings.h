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
#include "../Geometry3D/Geometry.h"

#include <array>

#include "RaytracingStructures.h"

PH_BEGIN

struct CameraUniform
{
	alignas(16) glm::mat4 Projection;
	alignas(16) glm::mat4 View;
};

struct BVH_VisualizerSettings :
		public vkEngine::GraphicsPipelineSettingsBase<BVH, glm::vec4, uint32_t>
{
public:
	BVH_VisualizerSettings(const vkEngine::BasicGraphicsPipelineSettings& basicSettings)
		: vkEngine::GraphicsPipelineSettingsBase<BVH, glm::vec4, uint32_t>(basicSettings) {}

	virtual vkEngine::Framebuffer GetFramebuffer() const override { return mRenderTarget; }

public:
	vkEngine::Buffer<CameraUniform> mCameraUniform;
	vkEngine::Framebuffer mRenderTarget;

public:

	virtual void CopyVertices(glm::vec4* Begin, glm::vec4* End, const BVH& renderable) const
	{
		for (size_t i = 0; i < renderable.Nodes.size(); i++)
		{
			Begin[0] = glm::vec4(renderable.Nodes[i].MinBound, 1.0f);
			Begin[1] = glm::vec4(renderable.Nodes[i].MaxBound, 1.0f);
			Begin += 2;
		}
	}

	virtual void CopyIndices(uint32_t* Begin, uint32_t* End, const BVH& renderable, uint32_t offset) const
	{
		uint32_t index = 0;

		for (const auto& node : renderable.Nodes)
		{
			Begin[0] = index + offset;
			Begin[1] = index + 1 + offset;
			Begin += 2;
			index += 2;
		}
	}

	virtual size_t GetVertexCount(const BVH& renderable) const
	{
		return 2 * renderable.Nodes.size();
	}

	virtual size_t GetIndexCount(const BVH& renderable) const
	{
		return 2 * renderable.Nodes.size();
	}

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override
	{
		vkEngine::UniformBufferWriteInfo uniformWriteInfo;
		uniformWriteInfo.Buffer = mCameraUniform.GetNativeHandles().Handle;

		writer.Update({ 0, 0, 0 }, uniformWriteInfo);
	}
};

struct NodeVisualizerSettings : public
	vkEngine::GraphicsPipelineSettingsBase<Node, glm::vec4, uint32_t>
{
public:
	NodeVisualizerSettings(const vkEngine::BasicGraphicsPipelineSettings& basicSettings)
		: vkEngine::GraphicsPipelineSettingsBase<Node, glm::vec4, uint32_t>(basicSettings) {}

public:
	vkEngine::Buffer<CameraUniform> mCameraUniform;
	vkEngine::Buffer<glm::mat4> mModelMatrices;
	vkEngine::Framebuffer mRenderTarget;

	BVH mBVH;

public:
	virtual vkEngine::Framebuffer GetFramebuffer() const override { return mRenderTarget; }

	virtual void CopyVertices(glm::vec4* Begin, glm::vec4* End, const Node& renderable) const override
	{
		for (size_t i = 0; i < mBVH.Vertices.size(); i++)
		{
			*Begin = glm::vec4(mBVH.Vertices[i], 1.0f);
			Begin++;
		}
	}

	virtual void CopyIndices(uint32_t* Begin, uint32_t* End, const Node& renderable, uint32_t offset) const override
	{
		for (size_t i = renderable.BeginIndex; i < renderable.EndIndex; i++)
		{
			Begin[0] = mBVH.Faces[i].x;
			Begin[1] = mBVH.Faces[i].y;
			Begin[2] = mBVH.Faces[i].z;
			Begin += 3;
		}
	}

	virtual size_t GetVertexCount(const Node& renderable) const override
	{
		return mBVH.Vertices.size();
	}

	virtual size_t GetIndexCount(const Node& renderable) const override
	{
		return 3 * (renderable.EndIndex - renderable.BeginIndex);
	}

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override
	{
		vkEngine::StorageBufferWriteInfo modelMatrixWrite;
		modelMatrixWrite.Buffer = mModelMatrices.GetNativeHandles().Handle;

		writer.Update({ 0, 1, 0 }, modelMatrixWrite);

		vkEngine::UniformBufferWriteInfo uniformWriteInfo;
		uniformWriteInfo.Buffer = mCameraUniform.GetNativeHandles().Handle;

		writer.Update({ 0, 0, 0 }, uniformWriteInfo);
	}
};

struct SampleVisualizerSettings : public vkEngine::GraphicsPipelineSettingsBase
	<std::vector<glm::vec3>, glm::vec3, uint32_t>
{
public:
	SampleVisualizerSettings(const vkEngine::BasicGraphicsPipelineSettings& basicSettings)
		: vkEngine::GraphicsPipelineSettingsBase<std::vector<glm::vec3>, glm::vec3, uint32_t>(basicSettings) {}

public:
	vkEngine::Buffer<CameraUniform> mCameraUniform;
	vkEngine::Framebuffer mRenderTarget;

public:

	virtual vkEngine::Framebuffer GetFramebuffer() const override { return mRenderTarget; }

	virtual void CopyVertices(glm::vec3* Begin, glm::vec3* End,
		const std::vector<glm::vec3>& renderable) const override
	{
		for (const auto& point : renderable)
		{
			*Begin = point;
			Begin++;
		}
	}

	virtual void CopyIndices(uint32_t* Begin, uint32_t* End,
		const std::vector<glm::vec3>& renderable, uint32_t offset) const override
	{
		uint32_t count = 0;

		for (const auto& point : renderable)
		{
			Begin[count] = count + offset;
			count++;
		}
	}

	virtual size_t GetVertexCount(const std::vector<glm::vec3>& renderable) const override
	{
		return renderable.size();
	}

	virtual size_t GetIndexCount(const std::vector<glm::vec3>& renderable) const override
	{
		return renderable.size();
	}

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override
	{
		vkEngine::UniformBufferWriteInfo uniformWriteInfo;
		uniformWriteInfo.Buffer = mCameraUniform.GetNativeHandles().Handle;

		writer.Update({ 0, 0, 0 }, uniformWriteInfo);
	}
};

PH_END

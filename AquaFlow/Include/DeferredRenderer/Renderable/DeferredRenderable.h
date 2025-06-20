#pragma once
#include "../../Core/AqCore.h"
#include "../../Geometry3D/GeometryConfig.h"

#include <tuple>

AQUA_BEGIN

struct RenderableInfo
{
	MeshData Mesh;
	vk::BufferUsageFlags Usage = vk::BufferUsageFlagBits::eVertexBuffer;
};

struct Renderable
{
public:
	RenderableInfo Info;

	glm::mat4 ModelTransform = glm::mat4(1.0f);
	size_t MaterialID = 0;

	std::vector<vkEngine::GenericBuffer> mVertexBuffers;
	vkEngine::GenericBuffer mIndexBuffer;

	Renderable(const Renderable&) = delete;
	Renderable& operator=(const Renderable&) = delete;

private:
	Renderable() = default;

	friend class RenderableBuilder;
};

AQUA_END

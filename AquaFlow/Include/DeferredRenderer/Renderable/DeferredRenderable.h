#pragma once
#include "../../Core/AqCore.h"
#include "../../Geometry3D/GeometryConfig.h"

AQUA_BEGIN

struct RenderableInfo
{
	MeshData Mesh;
	vk::BufferUsageFlags Usage = vk::BufferUsageFlagBits::eStorageBuffer;
};

using VertexMap = std::unordered_map<std::string, vkEngine::GenericBuffer>;

struct Renderable
{
public:
	Renderable() = default;
	~Renderable() = default;

	RenderableInfo Info;

	glm::mat4 ModelTransform = glm::mat4(1.0f);
	size_t MaterialID = 0;

	VertexMap mVertexBuffers;
	vkEngine::GenericBuffer mIndexBuffer;

	Renderable(const Renderable&) = delete;
	Renderable& operator=(const Renderable&) = delete;

private:

	friend class RenderableBuilder;
};

AQUA_END

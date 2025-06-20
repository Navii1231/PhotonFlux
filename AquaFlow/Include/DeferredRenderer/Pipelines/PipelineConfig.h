#pragma once
#include "../../Core/AqCore.h"

AQUA_BEGIN

#define TAG_POSITION              "@position"
#define TAG_NORMAL                "@normal"
#define TAG_TANGENT_SPACE         "@tangent_space"
#define TAG_TEXTURE_COORDS        "@texture_coords"
#define TAG_MATERIAL_IDS          "@material_ids"
#define TAG_INDEX                 "@index"

// The will specify the resource layout
// Entries: "@position", "@normal", "@tangent_space", "@texture_coords", 
// "@material_ids", "@index"
using VertexLayout = std::vector<std::string>;

struct CameraInfo
{
	glm::mat4 uProjection;
	glm::mat4 uView;
};

struct VertexResource
{
	std::string Tag;
	vkEngine::GenericBuffer Buffer;
	vkEngine::Image Image;
};

// NOTE: not much to do here, will only be used in shading phase
struct Resource
{
	std::string Tag;
	vkEngine::DescriptorLocation mLocation;

	union
	{
		vkEngine::GenericBuffer Buffer;
		vkEngine::Image Image;
	};
};

using VertexResourceMap = std::unordered_map<std::string, VertexResource>;
using ResourceMap = std::unordered_map<std::string, Resource>;

AQUA_END

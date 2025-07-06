#pragma once
#include "../../Core/AqCore.h"

AQUA_BEGIN

// Syntax to specify an attachment --> [@/$][name]_[type]
// Given that the vkEngine::Framebuffer separates out the color attachments
// from the depth attachments, all the depth tags must appear after the color attachments

#define ENTRY_POSITION            "position"
#define ENTRY_NORMAL              "normal"
#define ENTRY_TANGENT             "tangent"
#define ENTRY_BITANGENT           "bitangent"
#define ENTRY_TEXTURE_COORDS      "texcoords"
#define ENTRY_MATERIAL_IDS        "materialids"
#define ENTRY_DEPTH               "depth"

#define TAG_POSITION              "@position_rgba32f" 
#define TAG_NORMAL                "@normal_rgba16f"
#define TAG_TANGENT               "@tangent_rgba16f"
#define TAG_BITANGENT             "@bitangent_rgba16f"
#define TAG_TEXTURE_COORDS        "@texcoords_rg8un"
#define TAG_MATERIAL_IDS          "@materialids_rg8un"
#define TAG_DEPTH_STENCIL         "$depth_d24un_s8u"

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
	vkEngine::DescriptorLocation Location;

	vkEngine::GenericBuffer Buffer;
	vkEngine::Image Image;
};

struct Resource
{
	vkEngine::DescriptorLocation Location;

	vkEngine::Core::BufferChunk Buffer;
	vkEngine::Image Image;
};

/* An example of a shading pipeline */
struct Material
{
	glm::vec4 BaseColor;
	float Roughness;
	float Metallic;
	float RefractiveIndex;
	float TransmissionWeight;
};

struct DirectionalLightSrc
{
	glm::vec4 Direction;
	glm::vec4 Color;
};

struct PointLightSrc
{
	glm::vec4 Position;
	glm::vec4 Intensity;
	glm::vec4 Color;
	glm::vec2 DropRate;
};

using VertexResourceMap = std::unordered_map<std::string, VertexResource>;
using ResourceMap = std::unordered_map<std::string, Resource>;
using Mat4Buf = vkEngine::Buffer<glm::mat4>;

std::tuple<vk::Format, vk::ImageLayout, vk::ImageUsageFlags> ConvertIntoImageAttachment(const std::string&);
std::string GetImageAttachmentInfoString(vk::Format);

std::string GetTagName(const std::string& tag);

AQUA_END

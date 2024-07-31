#pragma once
#include "glm/glm.hpp"

#include <vector>
#include <array>
#include <unordered_map>
#include <algorithm>

#include <iostream>
#include <thread>
#include <mutex>

class Geometry3D;
struct GeometryNode;

struct aiFace;

enum class TextureType
{
	eNone                        = 0,
	eAmbient                     = 1,
	eDiffuse                     = 2,
	eNormal                      = 3,
	eHeight                      = 4,
	eDisplacement                = 5,
	eMetallic                    = 6,
	eRoughness                   = 7,
	eLight                       = 8,
	eEmissive                    = 9,
	eAmbientOcclusion            = 10,
	eUnkown,
};

enum class FacePrimitive
{
	ePoint                       = 0,
	eLine                        = 1,
	eTriangle                    = 2,
	eQuad                        = 3,
};

struct FlatMaterialPars
{
	glm::vec4 DiffuseColor = glm::vec4(1.0f);
	glm::vec3 AmbientColor = glm::vec3(0.15f);
	glm::vec3 EmissiveColor = glm::vec3(0.15f);
	float Metallicness = 0.2f;
	float Roughness = 0.6f;
};

struct MaterialInfo
{
	std::string Name;
	FlatMaterialPars FlatMaterialVals;

	std::unordered_map<TextureType, std::string> TextureFilepaths;
};

struct MeshData
{
	std::vector<glm::vec3> aPositions;
	std::vector<glm::vec3> aTexCoords;
	std::vector<glm::vec3> aNormals;
	std::vector<glm::vec3> aTangents;
	std::vector<glm::vec3> aBitangents;

	std::vector<glm::uvec4> aFaces;

	FacePrimitive mPrimitive = FacePrimitive::eTriangle;

	size_t MaterialRef = 0;

	void AssignPositions(const glm::vec3* positions, uint32_t count);
	void AssignTexCoords(const glm::vec3* coords, uint32_t count);
	void AssignNormals(const glm::vec3* normals, uint32_t count);
	void AssignTangentsAndBitangents(const glm::vec3* tangents, const glm::vec3* bitangents, uint32_t count);

	void AssignFaces(const aiFace* faces, uint32_t count);
};

struct GeometryNode
{
	std::string Name;
	glm::mat4 Transformation;
	std::vector<size_t> MeshRefs;

	GeometryNode* Parent = nullptr;

	std::vector<GeometryNode*> Children;
};

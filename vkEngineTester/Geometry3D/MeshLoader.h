#pragma once
#include "Geometry.h"
#include "Assimp/Importer.hpp"
#include "Assimp/postprocess.h"
#include "Assimp/Scene.h"

#include <unordered_map>

// Loads 3D models from files (thread safe)
// 'MeshLoader' doesn't create any internal state while loading
// The instance holds only the Assimp configuration that it was created with
// as a const field, making it optimal for multi threaded environments
class MeshLoader
{
public:
	struct Config
	{
		aiPostProcessSteps PostProcessFlag{};

		bool operator ==(const Config&) const = default;
		bool operator !=(const Config&) const = default;
	};

public:
	explicit MeshLoader(uint32_t PostProcessing)
		: mConfig({ static_cast<aiPostProcessSteps>(PostProcessing) }) {}

	~MeshLoader() = default;

	Geometry3D LoadModel(const std::string& filepath) const;

private:
	const Config mConfig;

private:
	// Helper Methods...
	FlatMaterialPars GetFlatMaterialParameters(aiMaterial* OtherMat) const;
	void AssignMaterials(std::vector<MaterialInfo>& Materials, aiMaterial** mMaterials, uint32_t mNumMaterials) const;
	void AssignMeshes(std::vector<MeshData>& MyMeshes, aiMesh** OtherMeshes, uint32_t count) const;
	void FillTextures(MaterialInfo& MyMat, aiMaterial* OtherMat) const;
};

namespace std
{
	template <>
	struct hash<MeshLoader::Config>
	{
		size_t operator()(const MeshLoader::Config& config) const
		{
			return hash<uint32_t>()(config.PostProcessFlag);
		}
	};
}

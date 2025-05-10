#pragma once
#include "GeometryConfig.h"

AQUA_BEGIN

class Geometry3D
{
public:
	Geometry3D() = default;
	~Geometry3D();

	Geometry3D(const Geometry3D&);
	Geometry3D(Geometry3D&&) noexcept;

	Geometry3D& operator =(const Geometry3D&);
	Geometry3D& operator =(Geometry3D&&) noexcept;

	MeshData& operator [](size_t index) { return mMeshes[index]; }
	const MeshData& operator [](size_t index) const { return mMeshes[index]; }

	auto begin() const { return mMeshes.begin(); }
	auto end() const { return mMeshes.end(); }

	const GeometryNode* GetRootNode() const { return mRootNode; }

	const std::vector<MeshData>& GetMeshData() const { return mMeshes; }
	const std::vector<MaterialInfo>& GetMaterials() const { return mMaterials; }

private:
	GeometryNode* mRootNode = nullptr;
	
	std::vector<MeshData> mMeshes;
	std::vector<MaterialInfo> mMaterials;

	friend class MeshLoader;

private:
	// Helper methods...
	void CopyNodeRecursive(GeometryNode* MyNode, GeometryNode* OtherNode);
	void DeleteNodeRecursive(GeometryNode* mRootNode);
	void MakeHollow();
};

AQUA_END

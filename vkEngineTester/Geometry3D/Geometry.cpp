#include "Geometry.h"
#include "Assimp/Scene.h"

Geometry3D::~Geometry3D()
{
	DeleteNodeRecursive(mRootNode);

	MakeHollow();
}

void Geometry3D::CopyNodeRecursive(GeometryNode* MyNode, GeometryNode* OtherNode)
{
	*MyNode = *OtherNode;

	for (size_t i = 0; i < OtherNode->Children.size(); i++)
	{
		MyNode->Children[i] = new GeometryNode;
		CopyNodeRecursive(MyNode->Children[i], OtherNode->Children[i]);
		MyNode->Children[i]->Parent = MyNode;
	}
}

Geometry3D::Geometry3D(const Geometry3D& Other)
{
	if (!Other.mRootNode)
		return;

	mRootNode = new GeometryNode;

	CopyNodeRecursive(mRootNode, Other.mRootNode);

	mMeshes = Other.mMeshes;
	mMaterials = Other.mMaterials;
}

Geometry3D::Geometry3D(Geometry3D&& Other) noexcept
{
	mRootNode = Other.mRootNode;
	mMeshes = std::move(Other.mMeshes);
	mMaterials = std::move(Other.mMaterials);

	Other.MakeHollow();
}

Geometry3D& Geometry3D::operator=(const Geometry3D& Other)
{
	DeleteNodeRecursive(mRootNode);

	if (!Other.mRootNode)
		return *this;

	mRootNode = new GeometryNode;

	CopyNodeRecursive(mRootNode, Other.mRootNode);

	mMeshes = Other.mMeshes;
	mMaterials = Other.mMaterials;

	return *this;
}

Geometry3D& Geometry3D::operator=(Geometry3D&& Other) noexcept
{
	DeleteNodeRecursive(mRootNode);

	mRootNode = Other.mRootNode;

	mMeshes = std::move(Other.mMeshes);
	mMaterials = std::move(Other.mMaterials);

	Other.MakeHollow();

	return *this;
}

void Geometry3D::DeleteNodeRecursive(GeometryNode* node)
{
	if (!node) return;

	// Post order traversal for deletion
	for (auto child : node->Children)
		DeleteNodeRecursive(child);

	delete node;
}

void Geometry3D::MakeHollow()
{
	mRootNode = nullptr;

	mMeshes.clear();
	mMaterials.clear();
}

void MeshData::AssignPositions(const glm::vec3* positions, uint32_t count)
{
	aPositions.clear();

	aPositions.resize(count);

	for (uint32_t i = 0; i < count; i++)
	{
		aPositions[i] = positions[i];
	}
}

void MeshData::AssignTexCoords(const glm::vec3* coords, uint32_t count)
{
	aTexCoords.clear();

	aTexCoords.resize(count);

	for (uint32_t i = 0; i < count; i++)
	{
		aTexCoords[i] = coords[i];
	}
}

void MeshData::AssignNormals(const glm::vec3* normals, uint32_t count)
{
	aNormals.clear();

	aNormals.resize(count);

	for (uint32_t i = 0; i < count; i++)
	{
		aNormals[i] = normals[i];
	}
}

void MeshData::AssignTangentsAndBitangents(const glm::vec3* tangents, const glm::vec3* bitangents, uint32_t count)
{
	aTangents.clear();
	aBitangents.clear();

	aTangents.resize(count);
	aBitangents.resize(count);

	for (uint32_t i = 0; i < count; i++)
	{
		aTangents[i] = tangents[i];
		aBitangents[i] = bitangents[i];
	}
}

void MeshData::AssignFaces(const aiFace* faces, uint32_t count)
{
	// TODO: Do any general type of face primitive...

	aFaces.clear();
	aFaces.resize(count);

	mPrimitive = FacePrimitive::eTriangle;

	for (uint32_t i = 0; i < count; i++)
	{
		_STL_ASSERT(faces->mNumIndices == 3, "Error while loading a scene: Faces are not triangulated!");

		aFaces[i].x = faces->mIndices[0];
		aFaces[i].y = faces->mIndices[1];
		aFaces[i].z = faces->mIndices[2];

		faces++;
	}
}

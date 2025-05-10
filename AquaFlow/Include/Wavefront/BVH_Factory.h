#pragma once
#include "RaytracingStructures.h"
#include "Core.h"

AQUA_BEGIN
PH_BEGIN

// TODO: Constructor should take the depth of the BVH tree not the MakeBVH routine!
class BVH_Factory
{
public:
	BVH_Factory(const std::vector<glm::vec3>& Vertices, const std::vector<Face>& Indices)
		: mVertices(Vertices), mFaces(Indices) {}

	BVH MakeBVH(int depth)
	{
		_STL_ASSERT(depth >= 0, "The depth of the BVH structure can't be negative!");

		Clear();

		Node& rootNode = mCurrent.Nodes.emplace_back();

		rootNode.BeginIndex = 0;
		rootNode.EndIndex = static_cast<uint32_t>(mFaces.size());

		EncloseIntoBoundingBox(rootNode);
		SplitRecursive(rootNode, depth);

		mCurrent.Vertices = std::move(mVertices);
		mCurrent.Faces = std::move(mFaces);

		return mCurrent;
	}

	void SplitRecursive(Node& parentNode, int depth);
	void EncloseIntoBoundingBox(Node& rootNode);

private:
	BVH mCurrent;
	std::vector<glm::vec3> mVertices;
	std::vector<Face> mFaces;

	float mTolerence = 0.001f;

	glm::vec3 FindTriangleCentre(uint32_t i);
	void Clear();
	std::pair<Box, Box> SplitBox(const Box& box);
	std::pair<Node, Node> MakeChildNodes(const Node& parentNode, const Box& leftBox, const Box& rightBox);
};

PH_END
AQUA_END

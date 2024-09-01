#include "BVH_Factory.h"

PH_BEGIN

void BVH_Factory::SplitRecursive(Node& parentNode, int depth)
{
	if (parentNode.EndIndex - parentNode.BeginIndex < 2)
		return;

	if (depth == 0)
		return;

	auto [leftBox, rightBox] = SplitBox(Box(parentNode.MinBound, parentNode.MaxBound));
	auto [leftChild, rightChild] = MakeChildNodes(parentNode, leftBox, rightBox);

	if (leftChild.BeginIndex == leftChild.EndIndex || rightChild.BeginIndex == rightChild.EndIndex)
		return;

	parentNode.FirstChildIndex = (uint32_t) mCurrent.Nodes.size();
	parentNode.SecondChildIndex = parentNode.FirstChildIndex + 1;

	uint32_t leftBoxIndex = parentNode.FirstChildIndex;
	uint32_t secondBoxIndex = parentNode.SecondChildIndex;

	mCurrent.Nodes.emplace_back(leftChild);
	mCurrent.Nodes.emplace_back(rightChild);

	// Split the left and right box recursively
	SplitRecursive(mCurrent.Nodes[leftBoxIndex], depth - 1);
	SplitRecursive(mCurrent.Nodes[secondBoxIndex], depth - 1);
}

void BVH_Factory::EncloseIntoBoundingBox(Node& node)
{
	glm::vec3 MinBound = { FLT_MAX, FLT_MAX, FLT_MAX };
	glm::vec3 MaxBound = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	auto ReplaceMinBound = [](float& MinBound, float VertexComponent)
	{
		if (MinBound > VertexComponent)
			MinBound = VertexComponent;
	};

	auto ReplaceMaxBound = [](float& MaxBound, float VertexComponent)
	{
		if (MaxBound < VertexComponent)
			MaxBound = VertexComponent;
	};

	std::for_each(mFaces.begin() + node.BeginIndex, mFaces.begin() + node.EndIndex,
		[this, &MinBound, &MaxBound, ReplaceMaxBound, ReplaceMinBound](const glm::uvec4& index)
	{
		for (int i = 0; i < 3; i++)
		{
			glm::vec3 Vertex = mVertices[index[i]];

			ReplaceMinBound(MinBound.x, Vertex.x);
			ReplaceMinBound(MinBound.y, Vertex.y);
			ReplaceMinBound(MinBound.z, Vertex.z);

			ReplaceMaxBound(MaxBound.x, Vertex.x);
			ReplaceMaxBound(MaxBound.y, Vertex.y);
			ReplaceMaxBound(MaxBound.z, Vertex.z);
		}
	});

	node.MinBound = MinBound - glm::vec3(mTolerence);
	node.MaxBound = MaxBound + glm::vec3(mTolerence);
}

glm::vec3 BVH_Factory::FindTriangleCentre(uint32_t i)
{
	glm::vec3 A = mVertices[mFaces[i].x];
	glm::vec3 B = mVertices[mFaces[i].y];
	glm::vec3 C = mVertices[mFaces[i].z];

	return (A + B + C) / 3.0f;
}

void BVH_Factory::Clear()
{
	mCurrent.Nodes.clear();
}

std::pair<Box, Box> BVH_Factory::SplitBox(const Box& box)
{
	glm::vec3 BoxSpan = box.Max - box.Min;

	float MaxSpan = 0.0f;
	int LargetSpanIndex = 0;

	for (int i = 0; i < 3; i++)
	{
		if (MaxSpan < BoxSpan[i])
		{
			LargetSpanIndex = i;
			MaxSpan = BoxSpan[i];
		}
	}

	float HalfMaxSpan = MaxSpan / 2.0f;

	const glm::vec3 Offset = glm::vec3(mTolerence);

	glm::vec3 MaxBoundForLeftBox = box.Max + Offset;
	MaxBoundForLeftBox[LargetSpanIndex] -= HalfMaxSpan;

	glm::vec3 MinBoundForRightBox = box.Min - Offset;
	MinBoundForRightBox[LargetSpanIndex] += HalfMaxSpan;

	return { Box(box.Min - Offset, MaxBoundForLeftBox), Box(MinBoundForRightBox, box.Max + Offset) };
}

std::pair<Node, Node> BVH_Factory::MakeChildNodes(const Node& parentNode, const Box& leftBox, const Box& rightBox)
{
	uint32_t LeftIndex = parentNode.BeginIndex;

	for (uint32_t i = parentNode.BeginIndex; i < parentNode.EndIndex; i++)
	{
		glm::vec3 Centre = FindTriangleCentre(i);

		if (leftBox.IsInside(Centre))
		{
			if (i != LeftIndex)
				std::swap(mFaces[i], mFaces[LeftIndex]);

			LeftIndex++;
		}
		else
		{
			_STL_ASSERT(rightBox.IsInside(Centre), "Point found neither in the left or right bounding box!");
		}
	}

	Node leftChild, rightChild;
	leftChild.BeginIndex = parentNode.BeginIndex;
	leftChild.EndIndex = LeftIndex;
	leftChild.FirstChildIndex = 0;
	leftChild.SecondChildIndex = 0;
	leftChild.MinBound = leftBox.Min;
	leftChild.MaxBound = leftBox.Max;

	rightChild.BeginIndex = LeftIndex;
	rightChild.EndIndex = parentNode.EndIndex;
	rightChild.FirstChildIndex = 0;
	rightChild.SecondChildIndex = 0;
	rightChild.MinBound = rightBox.Min;
	rightChild.MaxBound = rightBox.Max;

	EncloseIntoBoundingBox(leftChild);
	EncloseIntoBoundingBox(rightChild);

	return { leftChild, rightChild };
}

PH_END

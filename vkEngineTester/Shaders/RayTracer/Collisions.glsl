#ifndef COLLISIONS_GLSL
#define COLLISIONS_GLSL

#include "RayTracerConfig.glsl"

const float FLT_MAX = 3.4028235e+38;
const float TOLERENCE = 0.00001;

struct CollisionInfo
{
	// Values set by the collision solver...
	vec3 Normal;
	float RayDis;
	vec3 IntersectionPoint;
	float NormalInverted;

	// Values set by the collider...
	Material ObjectMaterial;
	LightProperties LightProps;
	uint PrimitiveID;

	// Booleans...
	bool HitOccured;
	bool IsLightSrc;
};

struct StackEntry
{
	CollisionInfo HitInfo;

	Ray IncomingRay;
	Ray OutgoingRay;

	vec3 HalfVec;
	float InterfaceRefractiveIndex;

	float IncomingSampleWeight;
};

struct AABB_CollisionInfo
{
	bool HitOccured;
	float RayDis;
};

void SwapWithCondition(inout float a, inout float b, bool condition)
{
	float Temp = b;
	b = condition ? a : b;
	a = condition ? Temp : a;
}

void CheckRayTriangleCollision(inout CollisionInfo hitInfo, in Ray ray, in vec3 A, in vec3 B, in vec3 C)
{
	hitInfo.HitOccured = false;

	vec3 Edge1 = B - A;
	vec3 Edge2 = C - B;
	vec3 Edge3 = A - C;

	vec3 Normal = normalize(cross(Edge1, Edge2));

	float Determinant = dot(Normal, ray.Direction);

	float Alpha = dot(Normal, (A - ray.Origin)) / Determinant;

	vec3 PlaneHitPoint = GetPoint(ray, Alpha);

	vec3 Cross1 = cross(Edge1, PlaneHitPoint - A);
	vec3 Cross2 = cross(Edge2, PlaneHitPoint - B);
	vec3 Cross3 = cross(Edge3, PlaneHitPoint - C);

	float b1 = dot(Cross1, Normal);
	float b2 = dot(Cross2, Normal);
	float b3 = dot(Cross3, Normal);

	hitInfo.IntersectionPoint = PlaneHitPoint;
	hitInfo.Normal = Normal;
	hitInfo.RayDis = Alpha;
	hitInfo.HitOccured = (b1 >= 0.0 && b2 >= 0.0 && b3 >= 0.0) &&
		Alpha > TOLERENCE && abs(Determinant) > TOLERENCE;

	hitInfo.NormalInverted = dot(Normal, ray.Direction) < 0 ? 1.0 : -1.0;
	hitInfo.Normal *= hitInfo.NormalInverted;
}

void CheckRayAABB_Collision(inout AABB_CollisionInfo hitInfo, 
	in Ray ray, in vec3 minCorner, in vec3 maxCorner)
{
	hitInfo.HitOccured = false;

	float tMin = (minCorner.x - ray.Origin.x) / ray.Direction.x;
	float tMax = (maxCorner.x - ray.Origin.x) / ray.Direction.x;

	SwapWithCondition(tMin, tMax, tMin > tMax);

	float tyMin = (minCorner.y - ray.Origin.y) / ray.Direction.y;
	float tyMax = (maxCorner.y - ray.Origin.y) / ray.Direction.y;

	SwapWithCondition(tyMin, tyMax, tyMin > tyMax);

	float txMin = tMin;
	float txMax = tMax;

	tMin = max(tMin, tyMin);
	tMax = min(tMax, tyMax);

	float tzMin = (minCorner.z - ray.Origin.z) / ray.Direction.z;
	float tzMax = (maxCorner.z - ray.Origin.z) / ray.Direction.z;

	SwapWithCondition(tzMin, tzMax, tzMin > tzMax);

	tMin = max(tMin, tzMin);
	tMax = min(tMax, tzMax);

	hitInfo.HitOccured = 
		(tMin < tzMax) && (tzMin < tMax) && 
		(tMin < tyMax) && (tyMin < tMax) &&
		((tMin < tMax) && (tMax > 0.0));

	hitInfo.RayDis = tMin > 0.0 ? tMin : tMax;
}

bool FindCollisionNode(inout CollisionInfo ClosestHit, in Ray ray, in uint rootIndex)
{
	bool FoundCloser = false;

	CollisionInfo hitInfo;

#if 0
	for (uint j = sNodeBuffer.Nodes[rootIndex].BeginIndex;
		j < sNodeBuffer.Nodes[rootIndex].EndIndex; j++)
	{
		CheckRayTriangleCollision(hitInfo, ray, 
			sVertexBuffer.Positions[sIndexBuffer.Indices[j].x], 
			sVertexBuffer.Positions[sIndexBuffer.Indices[j].y], 
			sVertexBuffer.Positions[sIndexBuffer.Indices[j].z]);

		hitInfo.PrimitiveID = j;

		bool Replaced = hitInfo.HitOccured &&
			(hitInfo.RayDis < ClosestHit.RayDis);

		FoundCloser = FoundCloser || Replaced;

		ClosestHit = Replaced ? hitInfo : ClosestHit;
	}

#else
	AABB_CollisionInfo hitInfoAABB;

	uint NodeStackIndices[64];
	uint StackPtr = 0;

	NodeStackIndices[StackPtr++] = rootIndex;

	while (StackPtr != 0)
	{
		uint CurrentIndex = NodeStackIndices[--StackPtr];

		CheckRayAABB_Collision(hitInfoAABB, ray,
			sNodeBuffer.Nodes[CurrentIndex].MinBound, sNodeBuffer.Nodes[CurrentIndex].MaxBound);

		NodeStackIndices[StackPtr++] = sNodeBuffer.Nodes[CurrentIndex].FirstChildIndex;
		NodeStackIndices[StackPtr++] = sNodeBuffer.Nodes[CurrentIndex].SecondChildIndex;

		bool Revert = !hitInfoAABB.HitOccured || hitInfoAABB.RayDis > ClosestHit.RayDis ||
			sNodeBuffer.Nodes[CurrentIndex].FirstChildIndex == rootIndex;

		StackPtr -= Revert ? 2 : 0;

		if (sNodeBuffer.Nodes[CurrentIndex].FirstChildIndex == rootIndex && hitInfoAABB.HitOccured)
		{
			for (uint j = sNodeBuffer.Nodes[CurrentIndex].BeginIndex;
				j < sNodeBuffer.Nodes[CurrentIndex].EndIndex; j++)
			{
				CheckRayTriangleCollision(hitInfo, ray, 
					sVertexBuffer.Positions[sIndexBuffer.Indices[j].x], 
					sVertexBuffer.Positions[sIndexBuffer.Indices[j].y], 
					sVertexBuffer.Positions[sIndexBuffer.Indices[j].z]);

				hitInfo.PrimitiveID = j;

				bool Replaced = hitInfo.HitOccured &&
					(hitInfo.RayDis < ClosestHit.RayDis);

				FoundCloser = FoundCloser || Replaced;

				ClosestHit = Replaced ? hitInfo : ClosestHit;
			}
		}
	}
#endif

	return FoundCloser;
}

void TestRayMeshCollisions(inout CollisionInfo ClosestHit, in Ray ray)
{
	// For all the mesh objects...

	for (uint i = 0; i < uSceneInfo.MeshCount; i++)
	{
		bool FoundCloser = FindCollisionNode(ClosestHit, ray, sMeshInfos.Infos[i].BeginIndex);
		ClosestHit.IsLightSrc = ClosestHit.IsLightSrc && (!FoundCloser);
		ClosestHit.ObjectMaterial = FoundCloser ? sMeshInfos.Infos[i].SurfaceMaterial :
			ClosestHit.ObjectMaterial;
	}
}

void TestRayLightCollisions(inout CollisionInfo ClosestHit, in Ray ray)
{
	// For all the light sources...

	for (uint i = 0; i < uSceneInfo.LightCount; i++)
	{
		bool FoundCloser = FindCollisionNode(ClosestHit, ray, sLightInfos.Infos[i].BeginIndex);
		ClosestHit.IsLightSrc = ClosestHit.IsLightSrc || FoundCloser;
		ClosestHit.LightProps = FoundCloser ? sLightInfos.Infos[i].Props : ClosestHit.LightProps;
	}
}

void CheckForRayCollisions(inout CollisionInfo ClosestHit, in Ray ray)
{
	ClosestHit.HitOccured = false;
	ClosestHit.RayDis = FLT_MAX;

	TestRayLightCollisions(ClosestHit, ray);
	TestRayMeshCollisions(ClosestHit, ray);
}


#endif
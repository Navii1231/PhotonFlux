#version 440

#ifndef PBR_RENDER
#define PBR_RENDER
#endif

#ifndef RAY_TRACER_GLSL
#define RAY_TRACER_GLSL

#ifndef RAY_TRACER_CONFIG_GLSL
#define RAY_TRACER_CONFIG_GLSL

#define _DEBUG  0

// Note: Set 1 is reserved for the estimator!

struct Ray
{
	vec3 Origin;
	vec3 Direction;
};

struct Material
{
	vec3 Albedo;
	float Metallic;
	float Roughness;
	float Transmission;
	float Padding1;
	float RefractiveIndex;
};

struct LightProperties
{
	vec3 Color;
};

struct MeshInfo
{
	uint BeginIndex;
	uint Padding;
	uint EndIndex;
	Material SurfaceMaterial;
};

struct LightInfo
{
	uint BeginIndex;
	uint Padding;
	uint EndIndex;
	LightProperties Props;
};

struct Node
{
	vec3 MinBound;
	uint BeginIndex;

	vec3 MaxBound;
	uint EndIndex;

	uint FirstChildIndex;
	uint Padding;
	uint SecondChildIndex;
};

vec3 GetPoint(Ray ray, float Par)
{
	return ray.Origin + ray.Direction * Par;
}

layout (std140, set = 1, binding = 0) uniform CameraUniform
{
	vec3 Position;
	vec3 Direction;
	vec3 Tangent;
	vec3 Bitangent;
} uCamera;

layout (std430, set = 1, binding = 1) readonly buffer VertexBuffer
{
	vec3 Positions[];
} sVertexBuffer;

layout (std430, set = 1, binding = 2) readonly buffer IndexBuffer
{
	uvec4 Indices[];
} sIndexBuffer;

layout (std430, set = 1, binding = 3) readonly buffer NodeBuffer
{
	Node Nodes[];
} sNodeBuffer;

layout (std430, set = 1, binding = 4) readonly buffer MeshInfoBuffer
{
	MeshInfo Infos[];
} sMeshInfos;

layout (std430, set = 1, binding = 5) readonly buffer LightInfoBuffer
{
	LightInfo Infos[];
} sLightInfos;

layout (std140, set = 1, binding = 6) uniform SceneInfo
{
	ivec2 MinBound;
	ivec2 MaxBound;
	ivec2 ImageResolution;

	uint MeshCount;
	uint LightCount;
	uint MaxBounceLimit;
	uint PixelSamples;

	uint RandomSeed;
	uint ResetImage;
	uint FrameCount;
} uSceneInfo;

layout (set = 1, binding = 7) uniform sampler2D uCubeMap;

#endif

#ifndef COLLISIONS_GLSL
#define COLLISIONS_GLSL

#ifndef RAY_TRACER_CONFIG_GLSL
#define RAY_TRACER_CONFIG_GLSL

#define _DEBUG  0

// Note: Set 1 is reserved for the estimator!

struct Ray
{
	vec3 Origin;
	vec3 Direction;
};

struct Material
{
	vec3 Albedo;
	float Metallic;
	float Roughness;
	float Transmission;
	float Padding1;
	float RefractiveIndex;
};

struct LightProperties
{
	vec3 Color;
};

struct MeshInfo
{
	uint BeginIndex;
	uint Padding;
	uint EndIndex;
	Material SurfaceMaterial;
};

struct LightInfo
{
	uint BeginIndex;
	uint Padding;
	uint EndIndex;
	LightProperties Props;
};

struct Node
{
	vec3 MinBound;
	uint BeginIndex;

	vec3 MaxBound;
	uint EndIndex;

	uint FirstChildIndex;
	uint Padding;
	uint SecondChildIndex;
};

vec3 GetPoint(Ray ray, float Par)
{
	return ray.Origin + ray.Direction * Par;
}

layout (std140, set = 1, binding = 0) uniform CameraUniform
{
	vec3 Position;
	vec3 Direction;
	vec3 Tangent;
	vec3 Bitangent;
} uCamera;

layout (std430, set = 1, binding = 1) readonly buffer VertexBuffer
{
	vec3 Positions[];
} sVertexBuffer;

layout (std430, set = 1, binding = 2) readonly buffer IndexBuffer
{
	uvec4 Indices[];
} sIndexBuffer;

layout (std430, set = 1, binding = 3) readonly buffer NodeBuffer
{
	Node Nodes[];
} sNodeBuffer;

layout (std430, set = 1, binding = 4) readonly buffer MeshInfoBuffer
{
	MeshInfo Infos[];
} sMeshInfos;

layout (std430, set = 1, binding = 5) readonly buffer LightInfoBuffer
{
	LightInfo Infos[];
} sLightInfos;

layout (std140, set = 1, binding = 6) uniform SceneInfo
{
	ivec2 MinBound;
	ivec2 MaxBound;
	ivec2 ImageResolution;

	uint MeshCount;
	uint LightCount;
	uint MaxBounceLimit;
	uint PixelSamples;

	uint RandomSeed;
	uint ResetImage;
	uint FrameCount;
} uSceneInfo;

layout (set = 1, binding = 7) uniform sampler2D uCubeMap;

#endif


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
#ifndef SHADING_GLSL
#define SHADING_GLSL

#define MATH_PI 3.14159265358979323846
#define SHADING_TOLERENCE 0.0001

// Physically based BSDF structures...

struct BSDF_Input
{
    vec3 LightDir;
    float InterfaceRoughness;

    vec3 Normal;
    float InterfaceMetallic;

    vec3 HalfVec;
    float TransmissionWeight;

    vec3 BaseColor;
    float RefractiveIndexRatio;
};

// Post-processing...

vec3 ToneMap(in vec3 color, float exposure)
{
    return vec3(1.0) - exp(-color * exposure);
}

vec3 GammaCorrection(in vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

vec3 GammaCorrectionInv(in vec3 color)
{
    return pow(color, vec3(2.2));
}

float RefractionJacobian(float IdotH, float VdotH, float RefractiveIndex)
{
    float Num = RefractiveIndex * RefractiveIndex * VdotH;
    float Denom = max(IdotH + RefractiveIndex * VdotH, SHADING_TOLERENCE);

    float Value = Num / (Denom * Denom);

    return max(Value, SHADING_TOLERENCE);
}

// Sampling the environment map...
vec3 SampleCubeMap(in vec3 Direction, float Rotation)
{
	// Find the spherical angles of the direction
	float ArcRadius = sqrt(1.0 - Direction.y * Direction.y);
	float Theta = acos(Direction.y);
	float Phi = sign(Direction.z) * acos(Direction.x / ArcRadius);

	// Find the UV coordinates
	float u = (Phi + Rotation + MATH_PI) / (2.0 * MATH_PI);
	float v = Theta / MATH_PI;

    u -= float(int(u));

	vec3 ImageColor = texture(uCubeMap, vec2(u, v)).rgb;

	return GammaCorrectionInv(ImageColor);
}

// Blinn Phong model...

vec3 BlinnPhongBRDF(in vec3 L, in vec3 V, in vec3 N, in Material SurfaceProps)
{
    vec3 H = normalize(L + V);

    float NdotH = dot(N, H);
    float NdotL = dot(N, L);

    float Shininess = 512.0 * (1.0 - SurfaceProps.Roughness);

    float RefractiveIndex = SurfaceProps.RefractiveIndex;

    vec3 Reflectivity = vec3((RefractiveIndex - 1.0) / (RefractiveIndex - 1.0));
    Reflectivity *= Reflectivity;

    vec3 DiffuseAmplitude = vec3(1.0) - Reflectivity;
    DiffuseAmplitude = max(vec3(0.0), DiffuseAmplitude);

    float Diffuse = max(NdotL, 0.0);
    float Specular = pow(max(NdotH, 0.0), Shininess);

    return Diffuse * SurfaceProps.Albedo * DiffuseAmplitude + Specular * Reflectivity;
}

// PBR model...

// Fresnel equations approximation
vec3 FresnelSchlick(float HdotV, in vec3 Reflectivity)
{
	return Reflectivity + (vec3(1.0) - Reflectivity) * pow(1.0 - HdotV, 5.0);
}

float GeometrySchlickGGX(float DotProduct, float Roughness)
{
    float RoughnessSq = Roughness * Roughness;
    float DotProdClamped = max(DotProduct, SHADING_TOLERENCE);

    return (2.0 * DotProdClamped) / (DotProdClamped + sqrt(RoughnessSq +
        (1 - RoughnessSq) * DotProdClamped * DotProdClamped));
}

// Compute geometry term using Smith's method
float GeometrySmith(float NdotV, float NdotL, float Roughness)
{
    float ggx2 = GeometrySchlickGGX(NdotV, Roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, Roughness);
    return ggx1 * ggx2;
}

// Compute normal distribution function using GGX/Trowbridge-Reitz
float DistributionGGX(float NdotH, float Roughness) 
{
    NdotH = max(NdotH, SHADING_TOLERENCE);
    
    float a2 = Roughness * Roughness;
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = MATH_PI * denom * denom;

    float Value = num / denom;

    const float MaxGGX_Value = 1.0 / (2.0 * TOLERENCE);

    return clamp(Value, SHADING_TOLERENCE, MaxGGX_Value);
}

float PDF_GGXVNDF_Reflection(in vec3 Normal, in vec3 ViewDir, in vec3 HalfVec, float Roughness)
{
    float NdotH = max(dot(Normal, HalfVec), SHADING_TOLERENCE);
    float VdotH = max(dot(ViewDir, HalfVec), SHADING_TOLERENCE);
    float NdotV = max(dot(Normal, ViewDir), SHADING_TOLERENCE);

    float G1 = GeometrySchlickGGX(NdotV, Roughness);
    float D_w = DistributionGGX(NdotH, Roughness);
    
    return (G1 * D_w) / (4.0 * NdotV);
}

float PDF_GGXVNDF_Refraction(in vec3 Normal, in vec3 ViewDir,
    in vec3 HalfVec, in vec3 IncidentDir, float Roughness, float RefractiveIndex)
{
    float NdotH = dot(Normal, HalfVec);
    float VdotH = dot(ViewDir, HalfVec);
    float NdotV = dot(ViewDir, Normal);
    float IdotH = dot(IncidentDir, HalfVec);

    float G1 = GeometrySchlickGGX(abs(NdotV), Roughness);
    float D_w = DistributionGGX(NdotH, Roughness);

    return (G1 * D_w * max(abs(VdotH), SHADING_TOLERENCE)) /
        max(abs(NdotV), SHADING_TOLERENCE);// * RefractionJacobian(IdotH, VdotH, RefractiveIndex);
}

vec3 CookTorranceBRDF(in BSDF_Input bsdfInput)
{
#if _DEBUG
    if(dot(V, N) < 0.0)
        return vec3(1.0, 0.0, 0.0);

    if(dot(L, N) < 0.0)
        return vec3(1.0, 0.0, 0.0);
#endif

    vec3 ViewDir = reflect(-bsdfInput.LightDir, bsdfInput.HalfVec);

    float NdotH = max(dot(bsdfInput.Normal, bsdfInput.HalfVec), SHADING_TOLERENCE);
    float NdotV = max(dot(bsdfInput.Normal, ViewDir), SHADING_TOLERENCE);
    float NdotL = max(dot(bsdfInput.Normal, bsdfInput.LightDir), SHADING_TOLERENCE);

    vec3 Reflectivity = vec3((bsdfInput.RefractiveIndexRatio - 1.0) /
        (bsdfInput.RefractiveIndexRatio + 1.0));

    Reflectivity *= Reflectivity;

    Reflectivity = mix(Reflectivity, bsdfInput.BaseColor, bsdfInput.InterfaceMetallic);

    float NDF = DistributionGGX(NdotH, bsdfInput.InterfaceRoughness);
    float G = GeometrySmith(NdotL, NdotV, bsdfInput.InterfaceRoughness);
    vec3 F = FresnelSchlick(NdotH, Reflectivity);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL;

    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0 - bsdfInput.TransmissionWeight);
    kD *= kD * (vec3(1.0) - kS);
    kD *= 1.0 - bsdfInput.InterfaceMetallic;

    vec3 Lo = (kD * bsdfInput.BaseColor / MATH_PI + specular) * NdotL;

    return Lo;
}

vec3 CookTorranceBTDF(in BSDF_Input bsdfInput)
{
#if _DEBUG
    if(dot(L, N) < 0.0)
        return vec3(1.0, 0.0, 0.0);
#endif

    float RefractiveIndex = bsdfInput.RefractiveIndexRatio;

    vec3 T = refract(-bsdfInput.LightDir, -bsdfInput.HalfVec, RefractiveIndex);

    if(length(T) == 0.0)
        return vec3(0);

    T = normalize(T);

    float NdotH = max(dot(bsdfInput.Normal, bsdfInput.HalfVec), SHADING_TOLERENCE);
    float NdotL = max(dot(-bsdfInput.Normal, bsdfInput.LightDir), SHADING_TOLERENCE);
    float NdotT = dot(-bsdfInput.Normal, T);

    float LdotH = max(dot(bsdfInput.LightDir, -bsdfInput.HalfVec), SHADING_TOLERENCE);
    float TdotH = dot(T, -bsdfInput.HalfVec);

    vec3 Reflectivity = vec3((RefractiveIndex - 1.0) / (RefractiveIndex + 1.0));
    Reflectivity *= Reflectivity;

    Reflectivity = mix(Reflectivity, bsdfInput.BaseColor, bsdfInput.InterfaceMetallic);

    float NDF = DistributionGGX(NdotH, bsdfInput.InterfaceRoughness);
    float G2 = GeometrySmith(abs(NdotT), NdotL, bsdfInput.InterfaceRoughness);

    vec3 F = FresnelSchlick(NdotH, Reflectivity);

    vec3 R = (vec3(1.0) - F) * bsdfInput.TransmissionWeight;

    R *= 1.0 - bsdfInput.InterfaceMetallic;

    vec3 kD = R;
    kD *= 1.0 - bsdfInput.TransmissionWeight;

    vec3 numerator = LdotH * NDF * G2 * R;// * RefractionJacobian(LdotH, TdotH, RefractiveIndex);
    float denominator = max(abs(NdotT), SHADING_TOLERENCE) * NdotL;
    denominator = NdotL;

    vec3 radiance = numerator / denominator;

    vec3 Lo = (kD * bsdfInput.BaseColor / MATH_PI + radiance) * NdotL;

    if(RefractiveIndex == 1.0)
        return vec3(100, 0, 0);

    return Lo;
}

vec3 CookTorranceBSDF(in BSDF_Input bsdfInput, float alpha)
{
    return mix(CookTorranceBRDF(bsdfInput), CookTorranceBTDF(bsdfInput), alpha);
}

#endif
#ifndef SAMPLER_GLSL
#define SAMPLER_GLSL

#ifndef COLLISIONS_GLSL
#define COLLISIONS_GLSL

#ifndef RAY_TRACER_CONFIG_GLSL
#define RAY_TRACER_CONFIG_GLSL

#define _DEBUG  0

// Note: Set 1 is reserved for the estimator!

struct Ray
{
	vec3 Origin;
	vec3 Direction;
};

struct Material
{
	vec3 Albedo;
	float Metallic;
	float Roughness;
	float Transmission;
	float Padding1;
	float RefractiveIndex;
};

struct LightProperties
{
	vec3 Color;
};

struct MeshInfo
{
	uint BeginIndex;
	uint Padding;
	uint EndIndex;
	Material SurfaceMaterial;
};

struct LightInfo
{
	uint BeginIndex;
	uint Padding;
	uint EndIndex;
	LightProperties Props;
};

struct Node
{
	vec3 MinBound;
	uint BeginIndex;

	vec3 MaxBound;
	uint EndIndex;

	uint FirstChildIndex;
	uint Padding;
	uint SecondChildIndex;
};

vec3 GetPoint(Ray ray, float Par)
{
	return ray.Origin + ray.Direction * Par;
}

layout (std140, set = 1, binding = 0) uniform CameraUniform
{
	vec3 Position;
	vec3 Direction;
	vec3 Tangent;
	vec3 Bitangent;
} uCamera;

layout (std430, set = 1, binding = 1) readonly buffer VertexBuffer
{
	vec3 Positions[];
} sVertexBuffer;

layout (std430, set = 1, binding = 2) readonly buffer IndexBuffer
{
	uvec4 Indices[];
} sIndexBuffer;

layout (std430, set = 1, binding = 3) readonly buffer NodeBuffer
{
	Node Nodes[];
} sNodeBuffer;

layout (std430, set = 1, binding = 4) readonly buffer MeshInfoBuffer
{
	MeshInfo Infos[];
} sMeshInfos;

layout (std430, set = 1, binding = 5) readonly buffer LightInfoBuffer
{
	LightInfo Infos[];
} sLightInfos;

layout (std140, set = 1, binding = 6) uniform SceneInfo
{
	ivec2 MinBound;
	ivec2 MaxBound;
	ivec2 ImageResolution;

	uint MeshCount;
	uint LightCount;
	uint MaxBounceLimit;
	uint PixelSamples;

	uint RandomSeed;
	uint ResetImage;
	uint FrameCount;
} uSceneInfo;

layout (set = 1, binding = 7) uniform sampler2D uCubeMap;

#endif


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
#ifndef SHADING_GLSL
#define SHADING_GLSL

#define MATH_PI 3.14159265358979323846
#define SHADING_TOLERENCE 0.0001

// Physically based BSDF structures...

struct BSDF_Input
{
    vec3 LightDir;
    float InterfaceRoughness;

    vec3 Normal;
    float InterfaceMetallic;

    vec3 HalfVec;
    float TransmissionWeight;

    vec3 BaseColor;
    float RefractiveIndexRatio;
};

// Post-processing...

vec3 ToneMap(in vec3 color, float exposure)
{
    return vec3(1.0) - exp(-color * exposure);
}

vec3 GammaCorrection(in vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

vec3 GammaCorrectionInv(in vec3 color)
{
    return pow(color, vec3(2.2));
}

float RefractionJacobian(float IdotH, float VdotH, float RefractiveIndex)
{
    float Num = RefractiveIndex * RefractiveIndex * VdotH;
    float Denom = max(IdotH + RefractiveIndex * VdotH, SHADING_TOLERENCE);

    float Value = Num / (Denom * Denom);

    return max(Value, SHADING_TOLERENCE);
}

// Sampling the environment map...
vec3 SampleCubeMap(in vec3 Direction, float Rotation)
{
	// Find the spherical angles of the direction
	float ArcRadius = sqrt(1.0 - Direction.y * Direction.y);
	float Theta = acos(Direction.y);
	float Phi = sign(Direction.z) * acos(Direction.x / ArcRadius);

	// Find the UV coordinates
	float u = (Phi + Rotation + MATH_PI) / (2.0 * MATH_PI);
	float v = Theta / MATH_PI;

    u -= float(int(u));

	vec3 ImageColor = texture(uCubeMap, vec2(u, v)).rgb;

	return GammaCorrectionInv(ImageColor);
}

// Blinn Phong model...

vec3 BlinnPhongBRDF(in vec3 L, in vec3 V, in vec3 N, in Material SurfaceProps)
{
    vec3 H = normalize(L + V);

    float NdotH = dot(N, H);
    float NdotL = dot(N, L);

    float Shininess = 512.0 * (1.0 - SurfaceProps.Roughness);

    float RefractiveIndex = SurfaceProps.RefractiveIndex;

    vec3 Reflectivity = vec3((RefractiveIndex - 1.0) / (RefractiveIndex - 1.0));
    Reflectivity *= Reflectivity;

    vec3 DiffuseAmplitude = vec3(1.0) - Reflectivity;
    DiffuseAmplitude = max(vec3(0.0), DiffuseAmplitude);

    float Diffuse = max(NdotL, 0.0);
    float Specular = pow(max(NdotH, 0.0), Shininess);

    return Diffuse * SurfaceProps.Albedo * DiffuseAmplitude + Specular * Reflectivity;
}

// PBR model...

// Fresnel equations approximation
vec3 FresnelSchlick(float HdotV, in vec3 Reflectivity)
{
	return Reflectivity + (vec3(1.0) - Reflectivity) * pow(1.0 - HdotV, 5.0);
}

float GeometrySchlickGGX(float DotProduct, float Roughness)
{
    float RoughnessSq = Roughness * Roughness;
    float DotProdClamped = max(DotProduct, SHADING_TOLERENCE);

    return (2.0 * DotProdClamped) / (DotProdClamped + sqrt(RoughnessSq +
        (1 - RoughnessSq) * DotProdClamped * DotProdClamped));
}

// Compute geometry term using Smith's method
float GeometrySmith(float NdotV, float NdotL, float Roughness)
{
    float ggx2 = GeometrySchlickGGX(NdotV, Roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, Roughness);
    return ggx1 * ggx2;
}

// Compute normal distribution function using GGX/Trowbridge-Reitz
float DistributionGGX(float NdotH, float Roughness) 
{
    NdotH = max(NdotH, SHADING_TOLERENCE);
    
    float a2 = Roughness * Roughness;
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = MATH_PI * denom * denom;

    float Value = num / denom;

    const float MaxGGX_Value = 1.0 / (2.0 * TOLERENCE);

    return clamp(Value, SHADING_TOLERENCE, MaxGGX_Value);
}

float PDF_GGXVNDF_Reflection(in vec3 Normal, in vec3 ViewDir, in vec3 HalfVec, float Roughness)
{
    float NdotH = max(dot(Normal, HalfVec), SHADING_TOLERENCE);
    float VdotH = max(dot(ViewDir, HalfVec), SHADING_TOLERENCE);
    float NdotV = max(dot(Normal, ViewDir), SHADING_TOLERENCE);

    float G1 = GeometrySchlickGGX(NdotV, Roughness);
    float D_w = DistributionGGX(NdotH, Roughness);
    
    return (G1 * D_w) / (4.0 * NdotV);
}

float PDF_GGXVNDF_Refraction(in vec3 Normal, in vec3 ViewDir,
    in vec3 HalfVec, in vec3 IncidentDir, float Roughness, float RefractiveIndex)
{
    float NdotH = dot(Normal, HalfVec);
    float VdotH = dot(ViewDir, HalfVec);
    float NdotV = dot(ViewDir, Normal);
    float IdotH = dot(IncidentDir, HalfVec);

    float G1 = GeometrySchlickGGX(abs(NdotV), Roughness);
    float D_w = DistributionGGX(NdotH, Roughness);

    return (G1 * D_w * max(abs(VdotH), SHADING_TOLERENCE)) /
        max(abs(NdotV), SHADING_TOLERENCE);// * RefractionJacobian(IdotH, VdotH, RefractiveIndex);
}

vec3 CookTorranceBRDF(in BSDF_Input bsdfInput)
{
#if _DEBUG
    if(dot(V, N) < 0.0)
        return vec3(1.0, 0.0, 0.0);

    if(dot(L, N) < 0.0)
        return vec3(1.0, 0.0, 0.0);
#endif

    vec3 ViewDir = reflect(-bsdfInput.LightDir, bsdfInput.HalfVec);

    float NdotH = max(dot(bsdfInput.Normal, bsdfInput.HalfVec), SHADING_TOLERENCE);
    float NdotV = max(dot(bsdfInput.Normal, ViewDir), SHADING_TOLERENCE);
    float NdotL = max(dot(bsdfInput.Normal, bsdfInput.LightDir), SHADING_TOLERENCE);

    vec3 Reflectivity = vec3((bsdfInput.RefractiveIndexRatio - 1.0) /
        (bsdfInput.RefractiveIndexRatio + 1.0));

    Reflectivity *= Reflectivity;

    Reflectivity = mix(Reflectivity, bsdfInput.BaseColor, bsdfInput.InterfaceMetallic);

    float NDF = DistributionGGX(NdotH, bsdfInput.InterfaceRoughness);
    float G = GeometrySmith(NdotL, NdotV, bsdfInput.InterfaceRoughness);
    vec3 F = FresnelSchlick(NdotH, Reflectivity);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL;

    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0 - bsdfInput.TransmissionWeight);
    kD *= kD * (vec3(1.0) - kS);
    kD *= 1.0 - bsdfInput.InterfaceMetallic;

    vec3 Lo = (kD * bsdfInput.BaseColor / MATH_PI + specular) * NdotL;

    return Lo;
}

vec3 CookTorranceBTDF(in BSDF_Input bsdfInput)
{
#if _DEBUG
    if(dot(L, N) < 0.0)
        return vec3(1.0, 0.0, 0.0);
#endif

    float RefractiveIndex = bsdfInput.RefractiveIndexRatio;

    vec3 T = refract(-bsdfInput.LightDir, -bsdfInput.HalfVec, RefractiveIndex);

    if(length(T) == 0.0)
        return vec3(0);

    T = normalize(T);

    float NdotH = max(dot(bsdfInput.Normal, bsdfInput.HalfVec), SHADING_TOLERENCE);
    float NdotL = max(dot(-bsdfInput.Normal, bsdfInput.LightDir), SHADING_TOLERENCE);
    float NdotT = dot(-bsdfInput.Normal, T);

    float LdotH = max(dot(bsdfInput.LightDir, -bsdfInput.HalfVec), SHADING_TOLERENCE);
    float TdotH = dot(T, -bsdfInput.HalfVec);

    vec3 Reflectivity = vec3((RefractiveIndex - 1.0) / (RefractiveIndex + 1.0));
    Reflectivity *= Reflectivity;

    Reflectivity = mix(Reflectivity, bsdfInput.BaseColor, bsdfInput.InterfaceMetallic);

    float NDF = DistributionGGX(NdotH, bsdfInput.InterfaceRoughness);
    float G2 = GeometrySmith(abs(NdotT), NdotL, bsdfInput.InterfaceRoughness);

    vec3 F = FresnelSchlick(NdotH, Reflectivity);

    vec3 R = (vec3(1.0) - F) * bsdfInput.TransmissionWeight;

    R *= 1.0 - bsdfInput.InterfaceMetallic;

    vec3 kD = R;
    kD *= 1.0 - bsdfInput.TransmissionWeight;

    vec3 numerator = LdotH * NDF * G2 * R;// * RefractionJacobian(LdotH, TdotH, RefractiveIndex);
    float denominator = max(abs(NdotT), SHADING_TOLERENCE) * NdotL;
    denominator = NdotL;

    vec3 radiance = numerator / denominator;

    vec3 Lo = (kD * bsdfInput.BaseColor / MATH_PI + radiance) * NdotL;

    if(RefractiveIndex == 1.0)
        return vec3(100, 0, 0);

    return Lo;
}

vec3 CookTorranceBSDF(in BSDF_Input bsdfInput, float alpha)
{
    return mix(CookTorranceBRDF(bsdfInput), CookTorranceBTDF(bsdfInput), alpha);
}

#endif
#ifndef RANDOM_GLSL
#define RANDOM_GLSL

#define MATH_PI 3.14159265358979323846

float GetRandom(inout uint state)
{
	state *= state * 747796405 + 2891336453;
	uint result = ((state >> (state >> 28) + 4) ^ state) * 277803737;
	result = (result >> 22) ^ result;
	return result / 4294967295.0;
}

// Function to generate a spherically uniform distribution
vec3 SampleUnitVecUniform(in vec3 Normal, inout uint state) 
{
    float u = GetRandom(state);
    float v = GetRandom(state); // Offset to get a different random number
    float phi = u * 2.0 * MATH_PI; // Random azimuthal angle in [0, 2*pi]
    float cosTheta = 2.0 * (v - 0.5); // Random polar angle, acos maps [0,1] to [0,pi]
	float sinTheta = sqrt(1 - cosTheta * cosTheta);

    // Convert spherical coordinates to Cartesian coordinates
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float z = cosTheta;

    vec3 RandomUnit = vec3(x, y, z);

	// Flip the vector if it lies outside of the hemisphere
	return dot(RandomUnit, Normal) < 0.0 ? -RandomUnit : RandomUnit;
}

// Function to generate a cosine weighted distribution
vec3 SampleUnitVecCosineWeighted(in vec3 Normal, inout uint state) 
{
    float u = GetRandom(state);
    float v = GetRandom(state); // Offset to get a different random number
    float phi = u * 2.0 * MATH_PI; // Random azimuthal angle in [0, 2*pi]
    float sqrtV = sqrt(v);

    // Convert spherical coordinates to Cartesian coordinates
    float x = sqrt(1.0 - v) * cos(phi);
    float y = sqrt(1.0 - v) * sin(phi);
    float z = sqrtV;

    vec3 Tangent = abs(Normal.x) > abs(Normal.z) ? 
        normalize(vec3(Normal.z, 0.0, -Normal.x)) : normalize(vec3(0.0, -Normal.z, Normal.y));

    vec3 Bitangent = cross(Normal, Tangent);

    mat3 TBN = mat3(Tangent, Bitangent, Normal);

    return normalize(TBN * vec3(x, y, z));
}

// TODO: This routine needs to be optimised
vec3 SampleHalfVecGGXVNDF_Distribution(in vec3 View, in vec3 Normal, float Roughness, inout uint state)
{
	float u1 = GetRandom(state);
	float u2 = GetRandom(state);

    vec3 Tangent = abs(Normal.x) > abs(Normal.z) ? 
        normalize(vec3(Normal.z, 0.0, -Normal.x)) : normalize(vec3(0.0, -Normal.z, Normal.y));

	vec3 Bitangent = cross(Normal, Tangent);

	mat3 TBN = mat3(Tangent, Bitangent, Normal);
	mat3 TBNInv = transpose(TBN);

	vec3 ViewLocal = TBNInv * View;

	// Stretch view...
	vec3 StretchedView = normalize(vec3(Roughness * ViewLocal.x,
		Roughness * ViewLocal.y, ViewLocal.z));

	// Orthonormal basis...

	vec3 T1 = StretchedView.z < 0.999 ? normalize(cross(StretchedView, vec3(0.0, 0.0, 1.0))) :
		vec3(1.0, 0.0, 0.0);

	vec3 T2 = cross(T1, StretchedView);

	// Sample point with polar coordinates (r, phi)

	float a = 1.0 / (1.0 + StretchedView.z);
	float r = sqrt(u1);
	float phi = (u2 < a) ? u2 / a * MATH_PI : MATH_PI * (1.0 + (u2 - a) / (1.0 - a));
	float P1 = r * cos(phi);
	float P2 = r * sin(phi) * ((u2 < a) ? 1.0 : StretchedView.z);

	// Form a normal

	vec3 HalfVec = P1 * T1 + P2 * T2 + sqrt(max(0.0, 1.0 - P1 * P1 - P2 * P2)) * StretchedView;

	// Unstretch normal

	HalfVec = normalize(vec3(Roughness * HalfVec.x, Roughness * HalfVec.y, max(0.0, HalfVec.z)));
	HalfVec = TBN * HalfVec;

	return normalize(HalfVec);
}

#endif

uint sRandomGeneratorSeed;
#define POWER_HEURISTICS_EXP      2.0

float GetDiffuseSpecularSamplingSeparation(float Roughness, float Metallic)
{
	return clamp((Roughness * Roughness + 1.0 - Metallic) / 2.0, 0.0, 1.0);
}

vec2 GetTransmissionProbability(float TransmissionWeight, float Metallic, 
	float RefractiveIndex, float NdotV, in vec3 RefractionDir)
{
	float Reflectivity = (RefractiveIndex - 1.0) / (RefractiveIndex + 1.0);
	Reflectivity *= Reflectivity;
	
	vec3 ReflectionFresnel = FresnelSchlick(NdotV, vec3(Reflectivity));
	float ReflectionProb = length(ReflectionFresnel);
	float TransmissionProb = clamp(1.0 - ReflectionProb, 0.0, 1.0) * TransmissionWeight * (1.0 - Metallic);

	TransmissionProb = length(RefractionDir) == 0.0 ? 0.0 : TransmissionProb;

	//TransmissionProb = TransmissionWeight;

	return vec2(1.0 - TransmissionProb, TransmissionProb);
}

vec3 GetSampleProbablities(float Roughness, float Metallic, float NdotV,
	float TransmissionWeight, float RefractiveIndex, in vec3 RefractionDir)
{
	// [ReflectionDiffuse, ReflectionSpecular, TransmissionDiffuse, TransmissionSpecular]

	float ScatteringSplit = GetDiffuseSpecularSamplingSeparation(Roughness, Metallic);
	vec2 TransmissionSplit = GetTransmissionProbability(TransmissionWeight, Metallic, 
		RefractiveIndex, NdotV, RefractionDir);

	vec3 Probs = vec3(0.0, 0.0, 0.0);

	Probs[0] = (1.0 - ScatteringSplit) * TransmissionSplit[0];
	Probs[1] = ScatteringSplit * TransmissionSplit[0];
	Probs[2] = TransmissionSplit[1];

	return Probs;
}

ivec2 GetSampleIndex(in vec3 SampleProbablities)
{
	float SampleCoin = GetRandom(sRandomGeneratorSeed);
	
	// Accumulate probabilities
    float p0 = SampleProbablities.x;
    float p1 = p0 + SampleProbablities.y;
    float p2 = p1 + SampleProbablities.z;

    // Use step to determine in which region the randomValue falls and cast to int
    int s1 = int(step(p0, SampleCoin)); // 1 if randomValue >= p0, else 0
    int s2 = int(step(p1, SampleCoin)); // 1 if randomValue >= p1, else 0
    int s3 = int(step(p2, SampleCoin)); // 1 if randomValue >= p2, else 0

    // Calculate the index by summing the step results and adding 0.5 offsets
    int SampleIndex = s1 + s2 + s3;
	int Hemisphere = SampleIndex == 2 ? -1 : 1;

	return ivec2(SampleIndex, Hemisphere);
}

bool SampleUnitVec(inout StackEntry Entry, float RefractiveIndex)
{	
	vec3 Normal = Entry.HitInfo.Normal;
	vec3 ViewDir = -Entry.OutgoingRay.Direction;
	float Roughness = Entry.HitInfo.ObjectMaterial.Roughness;
	float Metallic = Entry.HitInfo.ObjectMaterial.Metallic;
	float TransmissionWeight = Entry.HitInfo.ObjectMaterial.Transmission;

	float NdotV = dot(Normal, ViewDir);

	vec3 Directions[SAMPLE_SIZE];
	vec3 HalfVecs[SAMPLE_SIZE];
	float SampleWeights[SAMPLE_SIZE];

	HalfVecs[0] = SampleHalfVecGGXVNDF_Distribution(ViewDir, Normal, Roughness, sRandomGeneratorSeed);
	Directions[1] = SampleUnitVecCosineWeighted(Normal, sRandomGeneratorSeed);

	HalfVecs[2] = HalfVecs[0];

	HalfVecs[1] = normalize(ViewDir + Directions[1]);

	Directions[0] = reflect(-ViewDir, HalfVecs[0]);
	Directions[2] = refract(-ViewDir, HalfVecs[0], RefractiveIndex);

	vec3 SampleProbabilities = GetSampleProbablities(Roughness, Metallic, NdotV,
		TransmissionWeight, RefractiveIndex, Directions[0]);

	ivec2 SampleIndexInfo = GetSampleIndex(SampleProbabilities);
	int SampleIndex = SampleIndexInfo[0];
	float HemisphereSide = float(SampleIndexInfo[1]);

	vec3 LightDir = normalize(Directions[SampleIndex]);
	vec3 H = HalfVecs[SampleIndex];

	float VNDF_SampleWeightReflection = PDF_GGXVNDF_Reflection(Normal, ViewDir, H, Roughness);
	float VNDF_SampleWeightRefraction = PDF_GGXVNDF_Refraction(-Normal, ViewDir, -H,
		LightDir, Roughness, RefractiveIndex);

	float CosineSampleWeight = max(abs(dot(Normal, LightDir)), SHADING_TOLERENCE) / MATH_PI;

	SampleWeights[0] = VNDF_SampleWeightReflection;
	SampleWeights[1] = CosineSampleWeight;
	SampleWeights[2] = VNDF_SampleWeightRefraction;

	float SampleWeightInvReflection = (pow(SampleProbabilities[0] * SampleWeights[0], POWER_HEURISTICS_EXP) +
		pow(SampleProbabilities[1] * SampleWeights[1], POWER_HEURISTICS_EXP)) / 
		pow(SampleProbabilities[SampleIndex] * SampleWeights[SampleIndex], POWER_HEURISTICS_EXP - 1.0);

	//float SampleWeightInvReflection = (SampleProbabilities[0] * SampleWeights[0] +
	//	SampleProbabilities[1] * SampleWeights[1]);

	float SampleWeightInvRefraction = (SampleProbabilities[2] * SampleWeights[2]);

	float SampleWeightInv = HemisphereSide > 0.0 ? SampleWeightInvReflection : SampleWeightInvRefraction;

	Entry.HalfVec = H;
	Entry.IncomingRay.Origin = Entry.HitInfo.IntersectionPoint +
		HemisphereSide * TOLERENCE * Entry.HitInfo.Normal;
	Entry.IncomingRay.Direction = LightDir;
	Entry.IncomingSampleWeight = SampleWeightInv;
	Entry.InterfaceRefractiveIndex = 1.0 / RefractiveIndex;

	return dot(LightDir, Normal) * HemisphereSide > 0.0;
}


#endif

const vec3 MaxRadiance = vec3(100.0);

#define STACK_SIZE                20
#define SAMPLE_SIZE               3

vec3 GlobalIllumination(in StackEntry[STACK_SIZE] stack, int depth, bool foundLightSrc)
{
	// The last entry is always assumed to be a light source or a fragment on a skybox
	if (depth < 0)
		return vec3(0.0);

	uint TransmissionCount = 0;

	StackEntry LastEntry = stack[depth--];
	StackEntry SecondLastEntry = stack[depth--];

	BSDF_Input bsdfInput;
	
	bsdfInput.Normal = SecondLastEntry.HitInfo.Normal;
	bsdfInput.LightDir = SecondLastEntry.IncomingRay.Direction;
	bsdfInput.HalfVec = SecondLastEntry.HalfVec;
	
	bsdfInput.RefractiveIndexRatio = SecondLastEntry.InterfaceRefractiveIndex;
	
	bsdfInput.InterfaceRoughness = SecondLastEntry.HitInfo.ObjectMaterial.Roughness;
	bsdfInput.InterfaceMetallic = SecondLastEntry.HitInfo.ObjectMaterial.Metallic;
	bsdfInput.TransmissionWeight = SecondLastEntry.HitInfo.ObjectMaterial.Transmission;
	bsdfInput.BaseColor = SecondLastEntry.HitInfo.ObjectMaterial.Albedo;

	float Hemisphere = dot(bsdfInput.LightDir, bsdfInput.Normal) > 0.0 ? 0.0 : 1.0;
	
	// Debug variable...
	TransmissionCount += Hemisphere > 0.5 ? 1 : 0;

	vec3 ViewDir = -SecondLastEntry.OutgoingRay.Direction;

	vec3 LightSrcColor = foundLightSrc ? LastEntry.HitInfo.LightProps.Color :
		SampleCubeMap(bsdfInput.LightDir, 0.0);

	// Direct light
	vec3 Radiance = CookTorranceBSDF(bsdfInput, Hemisphere);

	Radiance *= LightSrcColor;
	Radiance /= SecondLastEntry.IncomingSampleWeight;

	Radiance = min(Radiance, MaxRadiance);

	// Indirect light
	while (depth >= 0)
	{
		StackEntry Entry = stack[depth--];

		bsdfInput.Normal = Entry.HitInfo.Normal;
		bsdfInput.LightDir = Entry.IncomingRay.Direction;
		bsdfInput.HalfVec = Entry.HalfVec;
		
		bsdfInput.RefractiveIndexRatio = Entry.InterfaceRefractiveIndex;
		
		bsdfInput.InterfaceRoughness = Entry.HitInfo.ObjectMaterial.Roughness;
		bsdfInput.InterfaceMetallic = Entry.HitInfo.ObjectMaterial.Metallic;
		bsdfInput.TransmissionWeight = Entry.HitInfo.ObjectMaterial.Transmission;
		bsdfInput.BaseColor = Entry.HitInfo.ObjectMaterial.Albedo;

		Hemisphere = dot(bsdfInput.LightDir, bsdfInput.Normal) > 0.0 ? 0.0 : 1.0;
		
		// Debug variable...
		TransmissionCount += Hemisphere > 0.5 ? 1 : 0;

		ViewDir = -Entry.OutgoingRay.Direction;

		// Calculate the outgoing Radiance from the radiance carried by the previous ray
		vec3 NewRadiance = CookTorranceBSDF(bsdfInput, Hemisphere);

		NewRadiance *= Radiance;
		// Weight the results according to the sample distributions...
		NewRadiance /= Entry.IncomingSampleWeight;
		Radiance = NewRadiance;

		Radiance = min(Radiance, MaxRadiance);
	}

	Radiance = min(Radiance, MaxRadiance);

	//if(TransmissionCount % 2 != 0)
	//	return vec3(100, 100, 0);

	return Radiance;
}

vec3 TraceRayMonteCarlo(in Ray ray)
{
	StackEntry stack[STACK_SIZE];
	int stackPtr = 0;

	float RayRefractiveIndex[STACK_SIZE];
	int layerPtr = 0;

	CollisionInfo hitInfo;
	CheckForRayCollisions(hitInfo, ray);

	if(!hitInfo.HitOccured)
		return SampleCubeMap(ray.Direction, 0.0);

	if(hitInfo.IsLightSrc)
		return hitInfo.LightProps.Color * hitInfo.NormalInverted;

	stack[stackPtr].HitInfo = hitInfo;
	stack[stackPtr].OutgoingRay = ray;

	RayRefractiveIndex[layerPtr] = 1.0;

	bool FoundLightSrc = false;
	uint i = 0;

	// Include the light src if we find one and then terminate
	for(; i < uSceneInfo.MaxBounceLimit; i++)
	{
		layerPtr -= int((-hitInfo.NormalInverted + 1.0) / 2.0 + 0.5);// < 0.0 ? 1 : 0;
		float RefractiveIndex = RayRefractiveIndex[layerPtr] / hitInfo.ObjectMaterial.RefractiveIndex;

		RefractiveIndex = pow(RefractiveIndex, hitInfo.NormalInverted);

		if(!SampleUnitVec(stack[stackPtr], RefractiveIndex))
			return vec3(0.0, 0.0, 0.0);

		RayRefractiveIndex[++layerPtr] = hitInfo.ObjectMaterial.RefractiveIndex;

		layerPtr -= dot(stack[stackPtr].IncomingRay.Direction, hitInfo.Normal) *
			hitInfo.NormalInverted > 0.0 ? 1 : 0;

		CheckForRayCollisions(hitInfo, stack[stackPtr].IncomingRay);
		stackPtr++;

		stack[stackPtr].HitInfo = hitInfo;
		stack[stackPtr].OutgoingRay = stack[stackPtr - 1].IncomingRay;

		if(!hitInfo.HitOccured || hitInfo.IsLightSrc)
		{
			FoundLightSrc = hitInfo.IsLightSrc;
			break;
		}
	}

	vec3 IncomingLight = (i != uSceneInfo.MaxBounceLimit || FoundLightSrc) ?
		GlobalIllumination(stack, stackPtr, FoundLightSrc) : vec3(0.0);

	return IncomingLight;
}

#endif

layout (local_size_x = 8, local_size_y = 4) in;

layout (set = 0, binding = 0, rgba8) uniform image2D ImageOutput;
layout (set = 0, binding = 1, rgba32f) uniform image2D ColorSum;

layout (std430, set = 2, binding = 0) writeonly buffer CPU_Feedback
{
	uint Finished;
} uCPU_Feedback;

void main()
{
	uvec2 Position = gl_GlobalInvocationID.xy;
	//Position.x = 0;

	sRandomGeneratorSeed = Position.x * uSceneInfo.RandomSeed + Position.y * 
		(Position.x + uSceneInfo.RandomSeed) + uSceneInfo.RandomSeed;

	ivec2 ImageResolution = uSceneInfo.MaxBound - uSceneInfo.MinBound;
	
	if(Position.x >= (uSceneInfo.MaxBound - uSceneInfo.MinBound).x ||
		Position.y >= (uSceneInfo.MaxBound - uSceneInfo.MinBound).y)
		return;

	ivec2 PositionOnImage = ivec2(Position) + uSceneInfo.MinBound;
	
	vec2 RawOffset = (vec2(PositionOnImage) - vec2(uSceneInfo.ImageResolution) / 2.0) /
		vec2(uSceneInfo.ImageResolution);
	
	RawOffset.y *= 900.0 / 1600.0;
	
	RawOffset *= 2.0;
	//RawOffset /= 8.0;
	
	Ray ray;
	ray.Origin = uCamera.Position;
	ray.Direction = normalize(uCamera.Direction + 
		(uCamera.Tangent * RawOffset.x - uCamera.Bitangent * RawOffset.y));
	
	vec3 IncomingLight = vec3(0.0);
	
	for(int i = 0; i < uSceneInfo.PixelSamples; i++)
		IncomingLight += TraceRayMonteCarlo(ray);
	
	IncomingLight /= uSceneInfo.PixelSamples;

	vec3 ExistingColor = imageLoad(ColorSum, ivec2(Position)).rgb;

	vec3 Delta = IncomingLight - ExistingColor;
	vec3 Color = ExistingColor + Delta / uSceneInfo.FrameCount;
	
	if(bool(uSceneInfo.ResetImage))
		Color = IncomingLight;

	vec3 FinalColor = ToneMap(Color, 0.5);
	FinalColor = GammaCorrection(FinalColor);

	if(uSceneInfo.FrameCount > 100)
	{
		// TODO: If something like 10 consecutive frames have the mean difference
		// values less than some threshold value, then we terminate...

		vec3 PercentageChange = (Color - ExistingColor) / ExistingColor;

		atomicAnd(uCPU_Feedback.Finished, uint(length(PercentageChange) < 0.01));

		if(length(Color - ExistingColor) < 0.0001)
			atomicAnd(uCPU_Feedback.Finished, 1);
	}
	else
	{
		uCPU_Feedback.Finished = 0;
	}

	imageStore(ColorSum, ivec2(Position), vec4(Color, 1.0));
	imageStore(ImageOutput, ivec2(Position + uSceneInfo.MinBound), vec4(FinalColor, 1.0));
}

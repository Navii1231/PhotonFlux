#ifndef RAY_TRACER_CONFIG_GLSL
#define RAY_TRACER_CONFIG_GLSL

// Note: Set 1 is reserved for the estimator!

struct Ray
{
	vec3 Origin;
	vec3 Direction;
};

struct Wavefront
{
	Ray Wave[WAVEFRONT_RAY_COUNT];
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
	uint MinBounceLimit;
	uint MaxBounceLimit;
	uint PixelSamples;

	uint RandomSeed;
	uint ResetImage;
	uint FrameCount;
	uint MaxSamples;
} uSceneInfo;

layout (set = 1, binding = 7) uniform sampler2D uCubeMap;

#endif

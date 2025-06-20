#pragma once
#include "RayTracingStructures.h"

#include "SortRecorder.h"
#include "WavefrontWorkflow.h"
#include "RayGenerationPipeline.h"

#include <memory>
#include <unordered_map>

AQUA_BEGIN
PH_BEGIN

#define OBJECT_FACE_ID    0
#define LIGHT_FACE_ID     1

#define OPTIMIZE_MATERIAL       1
#define OPTIMIZE_INTERSECTION   1

using RaySortRecorder = SortRecorder<uint32_t>;

enum class MaterialPreprocessState
{
	eSuccess               = 1,
	eInvalidSyntax         = 2,
	eShaderNotFound        = 3,
};

enum class TraceSessionState
{
	eReset             = 1,
	eOpenScope         = 2,
	eReady             = 3,
	eTracing           = 4,
};

struct MaterialShaderError
{
	std::string Info;
	MaterialPreprocessState State;
};

struct SessionInfo
{
	MeshInfoBuffer MeshInfos;
	LightInfoBuffer LightInfos;

	LightPropsBuffer LightPropsInfos;

	GeometryBuffers SharedBuffers;
	GeometryBuffers LocalBuffers;

	vkEngine::Buffer<PhysicalCamera> CameraSpecsBuffer;
	vkEngine::Buffer<ShaderData> ShaderConstData;

	WavefrontSceneInfo SceneData{};

	uint32_t ActiveBuffer = 0;

	glm::mat4 CameraView = glm::mat4(1.0f);
	PhysicalCamera CameraSpecs{};

	// For scoped operations...
	WavefrontTraceInfo TraceInfo{};

	TraceSessionState State = TraceSessionState::eReset;
};

struct WavefrontEstimatorCreateInfo
{
	vkEngine::Device Context;
	std::string ShaderDirectory;

	// Work group sizes for pipelines
	glm::ivec2 RayGenWorkgroupSize = { 16, 16 };
	uint32_t IntersectionWorkgroupSize = 256;
	uint32_t MaterialEvalWorkgroupSize = 256;

	float Tolerence = 0.001f;
};

template <typename T>
T HashCombine(T& seed, T value)
{
	std::hash<T> hasher;
	seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

	return seed;
}

PH_END
AQUA_END

namespace std
{
	template<>
	struct hash<vkEngine::DescriptorLocation>
	{
		size_t operator()(const vkEngine::DescriptorLocation& location) const
		{
			uint32_t seed = 0;
			seed = ::AQUA_NAMESPACE::PH_FLUX_NAMESPACE::HashCombine(seed, location.SetIndex);
			seed = ::AQUA_NAMESPACE::PH_FLUX_NAMESPACE::HashCombine(seed, location.Binding);
			seed = ::AQUA_NAMESPACE::PH_FLUX_NAMESPACE::HashCombine(seed, location.ArrayIndex);

			return seed;
		}
	};
}

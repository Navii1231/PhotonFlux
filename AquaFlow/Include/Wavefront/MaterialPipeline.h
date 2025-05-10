#pragma once
#include "WavefrontConfig.h"

#include <string>

AQUA_BEGIN
PH_BEGIN

/*
* user defined macro definitions...
* SHADER_TOLERENCE = 0.001, POWER_HEURISTIC_EXP = 2.0,
* EMPTY_MATERIAL_ID = -1, SKYBOX_MATERIAL_ID = -2, LIGHT_MATERIAL_ID = -3,
*/
struct MaterialCreateInfo
{
	std::string ShaderCode;
	uint32_t WorkGroupSize = 256;
	float ShadingTolerence = 0.001f;
	float PowerHeuristics = 2.0f;
};

class MaterialPipelineContext : public vkEngine::ComputePipelineContext
{
public:
	MaterialPipelineContext() = default;

	template <typename T>
	void InsertBufferResource(const vkEngine::DescriptorLocation& location,
		const vkEngine::Buffer<T>& buffer);

	void InsertImageResource(const vkEngine::DescriptorLocation& location,
		const vkEngine::Image& image);

	void RemoveResource(const vkEngine::DescriptorLocation& location);

	void Prepare(const MaterialCreateInfo& createInfo, float intersectionTolerence);

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override;

	void ClearImageResources() { mImages.clear(); }
	void ClearBufferResources() { mBuffers.clear(); }

private:
	MaterialCreateInfo mCreateInfo;

	// Set 0 stuff...
	RayBuffer mRays;
	RayInfoBuffer mRayInfos;
	CollisionInfoBuffer mCollisionInfos;
	GeometryBuffers mGeometry;
	LightInfoBuffer mLightInfos;
	LightPropsBuffer mLightProps;

private:
	// Other sets...
	std::unordered_map<vkEngine::DescriptorLocation, vkEngine::Core::BufferChunk> mBuffers;
	std::unordered_map<vkEngine::DescriptorLocation, vkEngine::Image> mImages;

private:
	friend class WavefrontEstimator;
	friend class Executor;
};

class MaterialPipeline
{
public:
	using PipelineHandle = vkEngine::ComputePipeline<MaterialPipelineContext>;

public:
	MaterialPipeline() = default;

	template <typename T>
	void InsertBufferResource(const vkEngine::DescriptorLocation& location,
		const vkEngine::Buffer<T>& buffer)
	{ mHandle.GetPipelineContext().InsertBufferResource(location, buffer); }

	inline void InsertImageResource(const vkEngine::DescriptorLocation& location,
		const vkEngine::Image& image)
	{ mHandle.GetPipelineContext().InsertImageResource(location, image); }

	inline void RemoveResource(const vkEngine::DescriptorLocation& location)
	{ mHandle.GetPipelineContext().RemoveResource(location); }

	inline void ClearBufferResources() { mHandle.GetPipelineContext().ClearBufferResources(); }
	inline void ClearImageResources() { mHandle.GetPipelineContext().ClearImageResources(); }

private:
	PipelineHandle mHandle;

	friend class WavefrontEstimator;
	friend class Executor;
};

template <typename T>
void PH_FLUX_NAMESPACE::MaterialPipelineContext::InsertBufferResource(
	const vkEngine::DescriptorLocation& location, const vkEngine::Buffer<T>& buffer)
{
	RemoveResource(location);

	mBuffers[location] = buffer.GetBufferChunk();
}

PH_END
AQUA_END


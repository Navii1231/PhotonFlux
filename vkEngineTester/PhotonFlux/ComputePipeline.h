#pragma once
#include "Core.h"
#include "Wavefront/BVHFactory.h"
#include "RayTracingStructures.h"

#include "Geometry3D/Geometry.h"
#include "Geometry3D/MeshLoader.h"

PH_BEGIN

struct EstimatorComputePipeline : public vkEngine::ComputePipeline
{
public:
	EstimatorComputePipeline() = default;
	EstimatorComputePipeline(const vkEngine::PShader& shader, vkEngine::ResourcePool manager);

	virtual void UpdateDescriptors() override;
	void UpdateCameraPosition(float deltaTime, float speed, CameraMovementFlags movement);
	void UpdateCameraOrientation(float deltaTime, float speed, float xpos, float ypos, bool Reset);

	void SubmitRenderable(const AquaFlow::MeshData& meshData, const Material& color,
		RenderableType type, uint32_t bvhDepth);

public:
	// Set 0...
	EstimatorTarget mTarget;
	vk::ImageLayout mBufferLayout = vk::ImageLayout::eGeneral;

	uint32_t mResetBounceLimit = 2;
	uint32_t mResetSampleCount = 1;

	CameraData mCameraData;
	SceneInfo mSceneData;

	// Set 1...
	vkEngine::Buffer<Camera> mCamera;

	GeometryBuffers mGeomBuffersShared;
	GeometryBuffers mGeomBuffersLocal;

	MaterialBuffer mMaterials; // Temporary... will be removed once a runtime material evaluation is implemented!
	LightPropsBuffer mLightProps;

	vkEngine::Buffer<MeshInfo> mMeshInfos;
	vkEngine::Buffer<LightInfo> mLightInfos;

	vkEngine::Buffer<SceneInfo> mSceneInfo;

	// Set 2...
	vkEngine::Buffer<uint32_t> mCPU_Feedback;

	vkEngine::Image mSkybox;
	vkEngine::Core::Ref<vk::Sampler> mSkyboxSampler;

	GLFWwindow* mWindow = nullptr;

private:
	vkEngine::Image LoadImage(const std::string& filepath, vkEngine::ResourcePool manager) const;

	GeometryBuffers CreateGeometryBuffers(vk::MemoryPropertyFlags memProps, vkEngine::ResourcePool manager);

	template <typename T, typename Iter, typename Fn>
	void CopyVertexAttrib(vkEngine::Buffer<T>& SharedBuffer, vkEngine::Buffer<T>& LocalBuffer, 
		Iter Begin, Iter End, Fn CopyRoutine);
};

template<typename T, typename Iter, typename Fn>
void EstimatorComputePipeline::CopyVertexAttrib(vkEngine::Buffer<T>& SharedBuffer, 
	vkEngine::Buffer<T>& LocalBuffer, Iter Begin, Iter End, Fn CopyRoutine)
{
	size_t LocalBufferSize = LocalBuffer.GetSize();
	size_t HostCount = End - Begin;

	if (HostCount == 0)
		return;

	SharedBuffer.Clear();
	SharedBuffer.Resize(HostCount);

	T* BeginDevice = SharedBuffer.MapMemory(HostCount);
	T* EndDevice = BeginDevice + HostCount;

	CopyRoutine(BeginDevice, EndDevice, &(*Begin), &(*(End - 1)) + 1);

	SharedBuffer.UnmapMemory();

	LocalBuffer.Resize(LocalBufferSize + HostCount);

	vk::BufferCopy CopyInfo{};
	CopyInfo.setSrcOffset(0);
	CopyInfo.setDstOffset(LocalBufferSize);
	CopyInfo.setSize(HostCount);

	vkEngine::CopyBufferRegions(LocalBuffer, SharedBuffer, std::vector<vk::BufferCopy>({ CopyInfo }));
}

PH_END

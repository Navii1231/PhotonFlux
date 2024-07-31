#pragma once
#include "Core.h"
#include "BVH_Factory.h"
#include "RayTracingStructures.h"

#include "../Geometry3D/Geometry.h"
#include "../Geometry3D/MeshLoader.h"

PH_BEGIN

struct ComputePipelineSettings : public vkEngine::ComputePipelineSettingsBase
{
public:
	ComputePipelineSettings(vkEngine::MemoryResourceManager manager);

	virtual void UpdateDescriptors(vkEngine::DescriptorWriter& writer) override;
	void UpdateCameraPosition(float deltaTime, float speed, CameraMovementFlags movement);
	void UpdateCameraOrientation(float deltaTime, float speed, float xpos, float ypos, bool Reset);

	void SubmitRenderable(const MeshData& meshData, const Material& color,
		RenderableType type, uint32_t bvhDepth);

public:
	// Set 0...
	EstimatorTarget mTarget;
	vk::ImageLayout mBufferLayout = vk::ImageLayout::eGeneral;

	uint32_t mResetBounceLimit = 1;
	uint32_t mResetSampleCount = 1;

	CameraData mCameraData;
	SceneInfo mSceneData;

	// Set 1...
	vkEngine::Buffer<Camera> mCamera;

	vkEngine::Buffer<glm::vec4> mVertices;
	vkEngine::Buffer<glm::vec4> mVerticesLocal;

	vkEngine::Buffer<glm::uvec4> mFaces;
	vkEngine::Buffer<glm::uvec4> mFacesLocal;

	vkEngine::Buffer<Node> mNodes;
	vkEngine::Buffer<Node> mNodesLocal;

	vkEngine::Buffer<MeshInfo> mMeshInfos;
	vkEngine::Buffer<LightInfo> mLightInfos;

	vkEngine::Buffer<SceneInfo> mSceneInfo;

	// Set 2...
	vkEngine::Buffer<uint32_t> mCPU_Feedback;

	vkEngine::Image mSkybox;
	vkEngine::Core::Ref<vk::Sampler> mSkyboxSampler;

	GLFWwindow* mWindow = nullptr;

private:
	vkEngine::Image LoadImage(const std::string& filepath, vkEngine::MemoryResourceManager manager) const;
};

PH_END

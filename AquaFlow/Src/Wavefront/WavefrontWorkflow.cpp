#include "Core/Aqpch.h"
#include "Wavefront/WavefrontWorkflow.h"
#include "Wavefront/WavefrontConfig.h"

AQUA_BEGIN

std::string GetShaderDirectory();

AQUA_END

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::IntersectionPipeline::UpdateDescriptors()
{
	vkEngine::DescriptorWriter& writer = this->GetDescriptorWriter();

	vkEngine::StorageBufferWriteInfo rayBufferWrite{};
	rayBufferWrite.Buffer = mRays.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, rayBufferWrite);

	vkEngine::StorageBufferWriteInfo collisionInfo{};
	collisionInfo.Buffer = mCollisionInfos.GetNativeHandles().Handle;

	writer.Update({ 0, 2, 0 }, collisionInfo);

	vkEngine::UniformBufferWriteInfo sceneInfo{};
	sceneInfo.Buffer = mSceneInfo.GetNativeHandles().Handle;

	writer.Update({ 1, 9, 0 }, sceneInfo);

	UpdateGeometryBuffers(writer);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::IntersectionPipeline::UpdateGeometryBuffers(
	vkEngine::DescriptorWriter& writer)
{
/*
* Descriptor layout in shaders pipelines for GeometryBuffers and Geometry references
* 
	layout(std430, set = 1, binding = 0) readonly buffer VertexBuffer
	{
		vec3 sPositions[];
	};

	layout(std430, set = 1, binding = 1) readonly buffer NormalBuffer
	{
		vec3 sNormals[];
	};

	layout(std430, set = 1, binding = 2) readonly buffer TexCoordBuffer
	{
		vec2 sTexCoords[];
	};

	layout(std430, set = 1, binding = 3) readonly buffer IndexBuffer
	{
		Face sIndices[];
	};

	layout(std430, set = 1, binding = 4) readonly buffer NodeBuffer
	{
		Node sNodes[];
	};

	layout(std430, set = 1, binding = 6) readonly buffer LightPropsBuffer
	{
		LightProperties sLightPropsInfos[];
	};
	
	layout(std430, set = 1, binding = 7) readonly buffer MeshInfoBuffer
	{
		MeshInfo sMeshInfos[];
	};
	
	layout(std430, set = 1, binding = 8) readonly buffer LightInfoBuffer
	{
		LightInfo sLightInfos[];
	};

*/

	vkEngine::StorageBufferWriteInfo storageInfo{};
	storageInfo.Buffer = mGeometryBuffers.Vertices.GetNativeHandles().Handle;
	writer.Update({ 1, 0, 0 }, storageInfo);

#if 0
	storageInfo.Buffer = mGeometryBuffers.Normals.GetNativeHandles().Handle;
	writer.Update({ 1, 1, 0 }, storageInfo);

	storageInfo.Buffer = mGeometryBuffers.TexCoords.GetNativeHandles().Handle;
	writer.Update({ 1, 2, 0 }, storageInfo);

#endif

	storageInfo.Buffer = mGeometryBuffers.Faces.GetNativeHandles().Handle;
	writer.Update({ 1, 3, 0 }, storageInfo);

	storageInfo.Buffer = mGeometryBuffers.Nodes.GetNativeHandles().Handle;
	writer.Update({ 1, 4, 0 }, storageInfo);

	// Meta data about vertices and light sources
	storageInfo.Buffer = mLightProps.GetNativeHandles().Handle;
	//writer.Update({ 1, 6, 0 }, storageInfo);

	storageInfo.Buffer = mMeshInfos.GetNativeHandles().Handle;
	writer.Update({ 1, 7, 0 }, storageInfo);

	storageInfo.Buffer = mLightInfos.GetNativeHandles().Handle;
	writer.Update({ 1, 8, 0 }, storageInfo);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::RaySortEpiloguePipeline::UpdateDescriptors()
{
	vkEngine::DescriptorWriter& writer = this->GetDescriptorWriter();

	if (mSortingEvent == RaySortEvent::eFinish)
	{
		vkEngine::StorageBufferWriteInfo collisionInfo{};
		collisionInfo.Buffer = mCollisionInfos.GetNativeHandles().Handle;

		writer.Update({ 0, 2, 0 }, collisionInfo);

		collisionInfo.Buffer = mRaysInfos.GetNativeHandles().Handle;
		writer.Update({ 0, 4, 0 }, collisionInfo);
	}

	vkEngine::StorageBufferWriteInfo rayBufferWrite{};
	rayBufferWrite.Buffer = mRays.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, rayBufferWrite);

	vkEngine::StorageBufferWriteInfo rayRefs{};
	rayRefs.Buffer = mRayRefs.GetNativeHandles().Handle;

	writer.Update({ 0, 3, 0 }, rayRefs);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::RayRefCounterPipeline::UpdateDescriptors()
{
	vkEngine::DescriptorWriter& writer = this->GetDescriptorWriter();

	vkEngine::StorageBufferWriteInfo rayRefs{};
	rayRefs.Buffer = mRayRefs.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, rayRefs);

	vkEngine::StorageBufferWriteInfo counts{};
	counts.Buffer = mRefCounts.GetNativeHandles().Handle;

	writer.Update({ 0, 1, 0 }, counts);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::PrefixSumPipeline::UpdateDescriptors()
{
	vkEngine::DescriptorWriter& writer = this->GetDescriptorWriter();

	vkEngine::StorageBufferWriteInfo counts{};
	counts.Buffer = mRefCounts.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, counts);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::LuminanceMeanPipeline::UpdateDescriptors()
{
	vkEngine::DescriptorWriter& writer = this->GetDescriptorWriter();

	vkEngine::StorageImageWriteInfo mean{};
	mean.ImageLayout = vk::ImageLayout::eGeneral;

	mean.ImageView = mPresentable .GetIdentityImageView();
	writer.Update({ 2, 0, 0 }, mean);

	mean.ImageView = mPixelVariance.GetIdentityImageView();
	//writer.Update({ 2, 2, 0 }, mean);

	mean.ImageView = mPixelMean.GetIdentityImageView();
	writer.Update({ 2, 1, 0 }, mean);

	vkEngine::StorageBufferWriteInfo rayInfos{};
	rayInfos.Buffer = mRays.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, rayInfos);

	rayInfos.Buffer = mRayInfos.GetNativeHandles().Handle;

	writer.Update({ 0, 4, 0 }, rayInfos);

	vkEngine::UniformBufferWriteInfo sceneInfo{};
	sceneInfo.Buffer = mSceneInfo.GetNativeHandles().Handle;

	writer.Update({ 1, 9, 0 }, sceneInfo);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::PostProcessImagePipeline::UpdateDescriptors()
{
	vkEngine::DescriptorWriter& writer = this->GetDescriptorWriter();

	vkEngine::StorageImageWriteInfo mean{};
	mean.ImageLayout = vk::ImageLayout::eGeneral;

	mean.ImageView = mPresentable.GetIdentityImageView();
	writer.Update({ 0, 0, 0 }, mean);
}

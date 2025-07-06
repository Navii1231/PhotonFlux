#include "Core/Aqpch.h"
#include "Wavefront/Executor.h"

#define MAX_UINT32 (static_cast<uint32_t>(~0))

AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::Executor() {}

uint32_t AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::GetRandomNumber()
{
	uint32_t Random = mExecutorInfo->UniformDistribution(mExecutorInfo->RandomEngine);
	_STL_ASSERT(Random != 0, "Random number can't be zero!");

	return Random;
}


// TODO: Freezes when complex geometry is introduced
AQUA_NAMESPACE::PH_FLUX_NAMESPACE::TraceResult AQUA_NAMESPACE::PH_FLUX_NAMESPACE::
	Executor::Trace(vk::CommandBuffer commandBuffer)
{
	// Assuming descriptors have been updated in the PH_FLUX_NAMESPACE::WavefrontEstimator::End() function...

	UpdateSceneInfo();

	uint32_t pRayCount = static_cast<uint32_t>(mExecutorInfo->Rays.GetSize()) / 2;
	uint32_t pActiveBuffer = 0;
	uint32_t pMaterialCount = static_cast<uint32_t>(mExecutorInfo->MaterialResources.size() + 2);
	glm::uvec3 rayGroupSize = mExecutorInfo->PipelineResources.RayGenerator.GetWorkGroupSize();

	PostProcessFlags postProcess = PostProcessFlagBits::eToneMap;
	postProcess |= PostProcessFlagBits::eGammaCorrection;

	glm::uvec3 workGroups = { mExecutorInfo->CreateInfo.TileSize.x / rayGroupSize.x,
		mExecutorInfo->CreateInfo.TileSize.y / rayGroupSize.y, 1 };

	uint32_t intersectionWorkgroups = pRayCount / 256;
	uint32_t pBounceIdx = 0;

	// Can be launched separately...
	ExecuteRayGenerator(commandBuffer, pActiveBuffer, { intersectionWorkgroups, 1, 1 });

	// Loop begins here...
	// TODO: implement Russain Roulette...
	for (uint32_t i = 0; i < mExecutorInfo->TracingInfo.MaxBounceLimit; i++)
	{
		// Intersection stage...
		// Must be launched separately...
		ExecuteIntersectionTester(commandBuffer, pRayCount, pActiveBuffer, { intersectionWorkgroups , 1, 1});

		if (mExecutorInfo->CreateInfo.AllowSorting)
		{
			// This stuff is optional...
			ExecuteRaySortPreparer(commandBuffer, pRayCount, pActiveBuffer, { intersectionWorkgroups , 1, 1 });
			ExecuteRayCounter(commandBuffer, pRayCount, pMaterialCount, { intersectionWorkgroups , 1, 1 });

			uint32_t pRayRefBuffer = mExecutorInfo->PipelineResources.SortRecorder->Run(commandBuffer);

			ExecuteRaySortFinisher(commandBuffer, pRayCount,
				pActiveBuffer, 0, { intersectionWorkgroups , 1, 1 });

			ExecutePrefixSummer(commandBuffer, pMaterialCount);

			pActiveBuffer = 1 - pActiveBuffer;
		}

		// All material pipelines can be launched together...
		RecordMaterialPipelines(commandBuffer, pRayCount, pBounceIdx, pActiveBuffer, { intersectionWorkgroups , 1, 1 });

		pBounceIdx++;
	}

	// Luminance mean calculations and post processing can be done together...
	RecordLuminanceMean(commandBuffer, pRayCount, pActiveBuffer, intersectionWorkgroups);
	//RecordPostProcess(commandBuffer, postProcess, workGroups);

	return TraceResult::eUnknownError;
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::SetTraceSession(const TraceSession& traceSession)
{
	_STL_ASSERT(traceSession.GetState() != TraceSessionState::eOpenScope,
		"Can't execute the tace session in the eOpenScope state!");

	mExecutorInfo->TracingSession = traceSession;
	mExecutorInfo->TracingInfo = traceSession.mSessionInfo->TraceInfo;

	auto& pipelines = mExecutorInfo->PipelineResources;

	pipelines.RayGenerator.mCamera = traceSession.mSessionInfo->CameraSpecsBuffer;
	pipelines.RayGenerator.mRays = mExecutorInfo->Rays;
	pipelines.RayGenerator.mSceneInfo = mExecutorInfo->Scene;
	pipelines.RayGenerator.mRayInfos = mExecutorInfo->RayInfos;

	pipelines.IntersectionPipeline.mCollisionInfos = mExecutorInfo->CollisionInfos;
	pipelines.IntersectionPipeline.mRays = mExecutorInfo->Rays;
	pipelines.IntersectionPipeline.mSceneInfo = mExecutorInfo->Scene;
	pipelines.IntersectionPipeline.mGeometryBuffers = traceSession.mSessionInfo->LocalBuffers;
	pipelines.IntersectionPipeline.mLightInfos = traceSession.mSessionInfo->LightInfos;
	pipelines.IntersectionPipeline.mLightProps = traceSession.mSessionInfo->LightPropsInfos;
	pipelines.IntersectionPipeline.mMeshInfos = traceSession.mSessionInfo->MeshInfos;

	pipelines.PrefixSummer.mRefCounts = mExecutorInfo->RefCounts;

	pipelines.RayRefCounter.mRayRefs = mExecutorInfo->RayRefs;
	pipelines.RayRefCounter.mRefCounts = mExecutorInfo->RefCounts;

	pipelines.RaySortPreparer.mCollisionInfos = mExecutorInfo->CollisionInfos;
	pipelines.RaySortPreparer.mRayRefs = mExecutorInfo->RayRefs;
	pipelines.RaySortPreparer.mRays = mExecutorInfo->Rays;
	pipelines.RaySortPreparer.mRaysInfos = mExecutorInfo->RayInfos;

	pipelines.RaySortFinisher.mRays = mExecutorInfo->Rays;
	pipelines.RaySortFinisher.mRayRefs = mExecutorInfo->RayRefs;
	pipelines.RaySortFinisher.mCollisionInfos = mExecutorInfo->CollisionInfos;
	pipelines.RaySortFinisher.mRaysInfos = mExecutorInfo->RayInfos;

	pipelines.InactiveRayShader.mHandle.mRays = mExecutorInfo->Rays;
	pipelines.InactiveRayShader.mHandle.mRayInfos = mExecutorInfo->RayInfos;
	pipelines.InactiveRayShader.mHandle.mCollisionInfos = mExecutorInfo->CollisionInfos;
	pipelines.InactiveRayShader.mHandle.mGeometry = traceSession.mSessionInfo->LocalBuffers;
	pipelines.InactiveRayShader.mHandle.mLightInfos = traceSession.mSessionInfo->LightInfos;
	pipelines.InactiveRayShader.mHandle.mLightProps = traceSession.mSessionInfo->LightPropsInfos;

	pipelines.LuminanceMean.mPixelMean = mExecutorInfo->Target.PixelMean;
	pipelines.LuminanceMean.mPixelVariance = mExecutorInfo->Target.PixelVariance;
	pipelines.LuminanceMean.mPresentable = mExecutorInfo->Target.Presentable;
	pipelines.LuminanceMean.mRays = mExecutorInfo->Rays;
	pipelines.LuminanceMean.mRayInfos = mExecutorInfo->RayInfos;
	pipelines.LuminanceMean.mSceneInfo = mExecutorInfo->Scene;

	pipelines.PostProcessor.mPresentable = mExecutorInfo->Target.Presentable;

	InvalidateMaterialData();

	pipelines.RayGenerator.UpdateDescriptors();
	pipelines.IntersectionPipeline.UpdateDescriptors();
	pipelines.PrefixSummer.UpdateDescriptors();
	pipelines.RayRefCounter.UpdateDescriptors();
	pipelines.RaySortPreparer.UpdateDescriptors();
	pipelines.RaySortFinisher.UpdateDescriptors();
	pipelines.LuminanceMean.UpdateDescriptors();
	pipelines.PostProcessor.UpdateDescriptors();
	pipelines.InactiveRayShader.UpdateDescriptors();

	UpdateMaterialDescriptors();
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::SetCameraView(const glm::mat4& cameraView)
{
	if (!mExecutorInfo->TracingSession.mSessionInfo)
		return;

	mExecutorInfo->TracingInfo.CameraView = cameraView;
	mExecutorInfo->TracingSession.SetCameraView(cameraView);
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::ExecuteRayGenerator(vk::CommandBuffer commandBuffer,
	uint32_t pActiveBuffer, glm::uvec3 workGroups)
{
	mExecutorInfo->PipelineResources.RayGenerator.Begin(commandBuffer);

	mExecutorInfo->PipelineResources.RayGenerator.BindPipeline();

	mExecutorInfo->PipelineResources.RayGenerator.SetShaderConstant(
		"eCompute.Camera.Index_0", mExecutorInfo->TracingInfo.CameraView);
	
	mExecutorInfo->PipelineResources.RayGenerator.SetShaderConstant("eCompute.Camera.Index_1", GetRandomNumber());
	mExecutorInfo->PipelineResources.RayGenerator.SetShaderConstant("eCompute.Camera.Index_2", pActiveBuffer);

	mExecutorInfo->PipelineResources.RayGenerator.Dispatch(workGroups);

	mExecutorInfo->PipelineResources.RayGenerator.InsertMemoryBarrier(
		vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);
	
	mExecutorInfo->PipelineResources.RayGenerator.End();
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::ExecuteRaySortFinisher(vk::CommandBuffer commandBuffer,
	uint32_t pRayCount, uint32_t pActiveBuffer, uint32_t pRayRefBuffer, glm::uvec3 workGroups)
{
	mExecutorInfo->PipelineResources.RaySortFinisher.Begin(commandBuffer);

	mExecutorInfo->PipelineResources.RaySortFinisher.InsertMemoryBarrier(
		vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);

	mExecutorInfo->PipelineResources.RaySortFinisher.BindPipeline();
	mExecutorInfo->PipelineResources.RaySortFinisher.SetShaderConstant("eCompute.RayData.Index_0", pRayCount);
	mExecutorInfo->PipelineResources.RaySortFinisher.SetShaderConstant("eCompute.RayData.Index_1", pActiveBuffer);
	mExecutorInfo->PipelineResources.RaySortFinisher.SetShaderConstant("eCompute.RayData.Index_2", pRayRefBuffer);

	mExecutorInfo->PipelineResources.RaySortFinisher.Dispatch(workGroups);

	mExecutorInfo->PipelineResources.RaySortFinisher.InsertMemoryBarrier(
		vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);

	mExecutorInfo->PipelineResources.RaySortFinisher.End();
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::ExecutePrefixSummer(
	vk::CommandBuffer commandBuffer, uint32_t pMaterialCount)
{
	mExecutorInfo->PipelineResources.PrefixSummer.Begin(commandBuffer);

	mExecutorInfo->PipelineResources.PrefixSummer.BindPipeline();
	mExecutorInfo->PipelineResources.PrefixSummer.SetShaderConstant("eCompute.MetaData.Index_0", pMaterialCount);

	mExecutorInfo->PipelineResources.PrefixSummer.Dispatch({ 1, 1, 1 });

	mExecutorInfo->PipelineResources.PrefixSummer.InsertMemoryBarrier(
		vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);

	mExecutorInfo->PipelineResources.PrefixSummer.End();
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::ExecuteRayCounter(vk::CommandBuffer commandBuffer,
	uint32_t pRayCount, uint32_t pMaterialCount, glm::uvec3 workGroups)
{
	mExecutorInfo->PipelineResources.RayRefCounter.Begin(commandBuffer);

	mExecutorInfo->PipelineResources.RayRefCounter.BindPipeline();
	mExecutorInfo->PipelineResources.RayRefCounter.SetShaderConstant("eCompute.MetaData.Index_0", pRayCount);
	mExecutorInfo->PipelineResources.RayRefCounter.SetShaderConstant("eCompute.MetaData.Index_1", pMaterialCount);

	mExecutorInfo->PipelineResources.RayRefCounter.Dispatch(workGroups);

	mExecutorInfo->PipelineResources.RayRefCounter.InsertMemoryBarrier(
		vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);

	mExecutorInfo->PipelineResources.RayRefCounter.End();
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::ExecuteRaySortPreparer(vk::CommandBuffer commandBuffer,
	uint32_t pRayCount, uint32_t pActiveBuffer, glm::uvec3 workGroups)
{
	mExecutorInfo->PipelineResources.RaySortPreparer.Begin(commandBuffer);

	mExecutorInfo->PipelineResources.RaySortPreparer.BindPipeline();
	mExecutorInfo->PipelineResources.RaySortPreparer.SetShaderConstant("eCompute.RayData.Index_0", pRayCount);
	mExecutorInfo->PipelineResources.RaySortPreparer.SetShaderConstant("eCompute.RayData.Index_1", pActiveBuffer);

	mExecutorInfo->PipelineResources.RaySortPreparer.Dispatch(workGroups);

	mExecutorInfo->PipelineResources.RaySortPreparer.InsertMemoryBarrier(
		vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);

	mExecutorInfo->PipelineResources.RaySortPreparer.End();
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::ExecuteIntersectionTester(vk::CommandBuffer commandBuffer,
	uint32_t pRayCount, uint32_t pActiveBuffer, glm::uvec3 workGroups)
{
	mExecutorInfo->PipelineResources.IntersectionPipeline.Begin(commandBuffer);

	mExecutorInfo->PipelineResources.IntersectionPipeline.BindPipeline();
	mExecutorInfo->PipelineResources.IntersectionPipeline.SetShaderConstant(
		"eCompute.RayData.Index_0", pRayCount);

	mExecutorInfo->PipelineResources.IntersectionPipeline.SetShaderConstant(
		"eCompute.RayData.Index_1", pActiveBuffer);

	mExecutorInfo->PipelineResources.IntersectionPipeline.Dispatch(workGroups);

	mExecutorInfo->PipelineResources.IntersectionPipeline.InsertMemoryBarrier(
		vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);

	mExecutorInfo->PipelineResources.IntersectionPipeline.End();
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::RecordLuminanceMean(vk::CommandBuffer commandBuffer,
	uint32_t pRayCount, uint32_t pActiveBuffer, uint32_t intersectionWorkgroups)
{
	mExecutorInfo->PipelineResources.LuminanceMean.Begin(commandBuffer);

	mExecutorInfo->PipelineResources.LuminanceMean.BindPipeline();

	mExecutorInfo->PipelineResources.LuminanceMean.SetShaderConstant("eCompute.ShaderData.Index_0", pRayCount);
	mExecutorInfo->PipelineResources.LuminanceMean.SetShaderConstant("eCompute.ShaderData.Index_1", pActiveBuffer);

	mExecutorInfo->PipelineResources.LuminanceMean.Dispatch({ intersectionWorkgroups , 1, 1 });

	mExecutorInfo->PipelineResources.LuminanceMean.InsertMemoryBarrier(
		vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);

	mExecutorInfo->PipelineResources.LuminanceMean.End();
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::RecordPostProcess(vk::CommandBuffer commandBuffer,
	PostProcessFlags postProcess, glm::uvec3 workGroups)
{
	mExecutorInfo->PipelineResources.PostProcessor.Begin(commandBuffer);

	mExecutorInfo->PipelineResources.PostProcessor.BindPipeline();

	mExecutorInfo->PipelineResources.PostProcessor.SetShaderConstant("eCompute.ShaderData.Index_0",
		mExecutorInfo->CreateInfo.TileSize.x);

	mExecutorInfo->PipelineResources.PostProcessor.SetShaderConstant("eCompute.ShaderData.Index_1",
		mExecutorInfo->CreateInfo.TileSize.y);

	mExecutorInfo->PipelineResources.PostProcessor.SetShaderConstant("eCompute.ShaderData.Index_2",
		(uint32_t) (int) postProcess);

	mExecutorInfo->PipelineResources.PostProcessor.Dispatch(workGroups);

	mExecutorInfo->PipelineResources.PostProcessor.InsertMemoryBarrier(
		vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);

	mExecutorInfo->PipelineResources.PostProcessor.End();
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::UpdateSceneInfo()
{
	WavefrontSceneInfo& sceneInfo = mExecutorInfo->TracingSession.mSessionInfo->SceneData;
	sceneInfo.ImageResolution = mExecutorInfo->CreateInfo.TargetResolution;
	sceneInfo.MinBound = { 0, 0 };
	sceneInfo.MaxBound = mExecutorInfo->CreateInfo.TargetResolution;
	sceneInfo.FrameCount = mExecutorInfo->TracingSession.mSessionInfo->State == TraceSessionState::eReady ?
		1 : sceneInfo.FrameCount + 1;

	mExecutorInfo->Scene.Clear();
	mExecutorInfo->Scene << sceneInfo;

	mExecutorInfo->TracingSession.mSessionInfo->State = TraceSessionState::eTracing;

	ShaderData shaderData{};
	shaderData.uRayCount = (uint32_t) mExecutorInfo->Rays.GetSize();
	shaderData.uSkyboxColor = glm::vec4(0.0f, 1.0f, 1.0f, 0.0f);
	shaderData.uSkyboxExists = false;

	mExecutorInfo->TracingSession.mSessionInfo->ShaderConstData.Clear();
	mExecutorInfo->TracingSession.mSessionInfo->ShaderConstData << shaderData;
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::InvalidateMaterialData()
{
	if (!mExecutorInfo->TracingSession)
		return;

	auto& TracingSession = *mExecutorInfo->TracingSession.mSessionInfo;

	for (auto& curr : mExecutorInfo->MaterialResources)
	{
		curr.mHandle.mRays = mExecutorInfo->Rays;
		curr.mHandle.mRayInfos = mExecutorInfo->RayInfos;
		curr.mHandle.mCollisionInfos = mExecutorInfo->CollisionInfos;
		curr.mHandle.mGeometry = TracingSession.LocalBuffers;
		curr.mHandle.mLightInfos = TracingSession.LightInfos;
		curr.mHandle.mLightProps = TracingSession.LightPropsInfos;
		curr.mHandle.mShaderData = TracingSession.ShaderConstData;
	}

	auto& inactivePipeline = mExecutorInfo->PipelineResources.InactiveRayShader.mHandle;

	inactivePipeline.mRays = mExecutorInfo->Rays;
	inactivePipeline.mRayInfos = mExecutorInfo->RayInfos;
	inactivePipeline.mCollisionInfos = mExecutorInfo->CollisionInfos;
	inactivePipeline.mGeometry = TracingSession.LocalBuffers;
	inactivePipeline.mLightInfos = TracingSession.LightInfos;
	inactivePipeline.mLightProps = TracingSession.LightPropsInfos;
	inactivePipeline.mShaderData = TracingSession.ShaderConstData;
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::RecordMaterialPipelines(
	vk::CommandBuffer commandBuffer, uint32_t pRayCount, uint32_t pBounceIdx, uint32_t pActiveBuffer, glm::uvec3 workGroups)
{
#define INACTIVE_MATERIAL 1

	uint32_t MaterialCount = static_cast<uint32_t>(mExecutorInfo->MaterialResources.size());

	for (uint32_t i = 0; i < MaterialCount; i++)
	{
		auto pipeline = mExecutorInfo->MaterialResources[i];
		uint32_t pMaterialRef = i;

		pipeline.Begin(commandBuffer);

		pipeline.BindPipeline();

		pipeline.SetShaderConstant("eCompute.ShaderConstants.Index_0", pMaterialRef);
		pipeline.SetShaderConstant("eCompute.ShaderConstants.Index_1", pActiveBuffer);
		pipeline.SetShaderConstant("eCompute.ShaderConstants.Index_2", GetRandomNumber());
		//pipeline.SetShaderConstant("eCompute.ShaderConstants.Index_3", pBounceIdx);

		pipeline.Dispatch(workGroups);

#if !INACTIVE_MATERIAL

#endif

		pipeline.InsertMemoryBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eComputeShader,
			vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
			vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);

		pipeline.End();

		std::vector<ShaderData> shaderData;

		pipeline.mHandle.mShaderData >> shaderData;
	}

#if INACTIVE_MATERIAL
	auto inactivePipeline = mExecutorInfo->PipelineResources.InactiveRayShader;

	inactivePipeline.Begin(commandBuffer);

	inactivePipeline.BindPipeline();

	inactivePipeline.SetShaderConstant("eCompute.ShaderConstants.Index_0", static_cast<uint32_t>(-1));
	inactivePipeline.SetShaderConstant("eCompute.ShaderConstants.Index_1", pActiveBuffer);

	inactivePipeline.Dispatch(workGroups);

	inactivePipeline.InsertMemoryBarrier(
		vk::PipelineStageFlagBits::eComputeShader,
		vk::PipelineStageFlagBits::eComputeShader,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
		vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);

	inactivePipeline.End();

#endif
}

void AQUA_NAMESPACE::PH_FLUX_NAMESPACE::Executor::UpdateMaterialDescriptors()
{
	for (auto& pipeline : mExecutorInfo->MaterialResources)
	{
		pipeline.UpdateDescriptors();
	}
}


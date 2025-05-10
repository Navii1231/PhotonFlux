#include "ComputePipelineTester.h"
#include "Utils/Utils.h"
#include "Geometry3D/MeshLoader.h"

#include <chrono>

#define USE_WAVEFRONT_PATHTRACER   1

bool ComputePipelineTester::OnStart()
{
	PhFlux::EstimatorCreateInfo createInfo{ *mDevice };
	createInfo.ShaderDirectory = "D:/Dev/VulkanEngine/vkEngineTester/Shaders/RayTracer";
	createInfo.TargetResolution = { 1920, 1080 };
	createInfo.TileSize = { 240, 135 };
	createInfo.TileSize = { 1920, 1080 };

	mEstimator = std::make_shared<PhFlux::ComputeEstimator>(createInfo);

	AquaFlow::PhFlux::WavefrontEstimatorCreateInfo wavefrontCreateInfo{ *mDevice };
	wavefrontCreateInfo.ShaderDirectory = "D:/Dev/VulkanEngine/vkEngineTester/Shaders/Wavefront";

	mWavefrontEstimator = std::make_shared<AquaFlow::PhFlux::WavefrontEstimator>(wavefrontCreateInfo);

	// Builders
	mPipelineBuilder = mDevice->MakePipelineBuilder();
	mMemoryResourceManager = mDevice->MakeMemoryResourceManager();

	mRenderContextBuilder = mDevice->FetchRenderContextBuilder(vk::PipelineBindPoint::eGraphics);

	vkEngine::ImageAttachmentInfo colorInfo{};

	colorInfo.Format = vk::Format::eR8G8B8A8Unorm;
	colorInfo.Layout = vk::ImageLayout::eColorAttachmentOptimal;
	colorInfo.Usage = vk::ImageUsageFlagBits::eColorAttachment;

	vkEngine::ImageAttachmentInfo depthInfo{};

	depthInfo.Format = vk::Format::eD24UnormS8Uint;
	depthInfo.Layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	depthInfo.Usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;

	vkEngine::RenderContextCreateInfo contextInfo{};
	contextInfo.ColorAttachments = { colorInfo };
	contextInfo.DepthStencilAttachment = depthInfo;
	contextInfo.UsingDepthAttachment = true;
	contextInfo.UsingStencilAttachment = true;

	mRenderContext = mRenderContextBuilder.MakeContext(contextInfo);
	mRenderTarget = mRenderContext.CreateFramebuffer(1600, 900);

	mQueueManager = mDevice->GetQueueManager();
	mComputeWorker = mQueueManager->FetchExecutor(0, vkEngine::QueueAccessType::eGeneric);
	mGraphicsWorker = mComputeWorker;

	mSwapchain = mDevice->GetSwapchain();

	SetupCameras();

#if USE_WAVEFRONT_PATHTRACER
	//TestNodeSystem();
	SetSceneWavefront();
#else
	SetScene();
#endif

	return true;
}

bool ComputePipelineTester::OnUpdate(float ElaspedTime)
{
	mCurrTimeStep = ElaspedTime;

	auto Data = mSwapchain->GetSwapchainData();

	mComputeWorker[0]->WaitIdle();

	auto FrameIndex = mSwapchain->AcquireNextFrame(Data.ImageAcquired);

	_STL_ASSERT(FrameIndex.result == vk::Result::eSuccess, "Could not acquire the image from swapchain");

	auto& ActiveFrame = *Data.Frames[FrameIndex.value];

	static size_t i = 0;

	i++;

	vk::SubmitInfo submitInfo{};
	vk::PipelineStageFlags waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	uint32_t index = 0;

	ActiveFrame.CmdBuffer.reset();

	ActiveFrame.CmdBuffer.begin(vk::CommandBufferBeginInfo());

	// Ray tracing...

	//FillWavefrontHostBuffers();
	//PrintWavefrontRayHostBuffer();

#if USE_WAVEFRONT_PATHTRACER
	RayTracerWithWavefront(ActiveFrame.CmdBuffer, ActiveFrame.RenderTarget);

#else
	RayTrace(ActiveFrame.CmdBuffer, ActiveFrame.RenderTarget);

#endif

	ActiveFrame.CmdBuffer.end();

	waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

	submitInfo = vk::SubmitInfo();

	submitInfo.setCommandBuffers(ActiveFrame.CmdBuffer);
	submitInfo.setWaitDstStageMask(waitStages);
	submitInfo.setWaitSemaphores(*Data.ImageAcquired);
	submitInfo.setSignalSemaphores(ActiveFrame.ImageRendered);

	index = mGraphicsWorker.SubmitWork(submitInfo);

	ActiveFrame.RenderTarget.TransitionColorAttachmentLayouts(vk::ImageLayout::ePresentSrcKHR,
		vk::PipelineStageFlagBits::eTopOfPipe);

	mGraphicsWorker[index]->WaitIdle();

#if 0
	FillWavefrontHostBuffers();

	//PrintWavefrontRayHostBuffer();
	PrintWavefrontCollisionInfoHostBuffer();
	//PrintWavefrontRayRefHostBuffer();
	//PrintWavefrontMaterialRefCountHostBuffer();
	//PrintWavefrontRayInfoHostBuffer();

	std::cin.get();
#elif 0
	std::vector<PhFlux::WavefrontSceneInfo> sceneInfo;
	auto sceneInfoBuffer = mExecutor.GetSceneInfo();

	sceneInfoBuffer >> sceneInfo;

	PhFlux::WavefrontSceneInfo info = sceneInfo[0];

	std::cout << info.MinBound << ", " << info.MaxBound << ", " <<
		info.ImageResolution << std::endl;

	std::cin.get();

#endif

	PresentScreen(FrameIndex.value, ActiveFrame);

	return true;
}

bool ComputePipelineTester::UpdateCamera(AquaFlow::EditorCamera& camera)
{
	AquaFlow::CameraMovementFlags movement = GetMovementFlags();
	glm::vec2 MousePos = mWindow->GetCursorPosition();

	bool HasMoved = camera.OnUpdate(std::chrono::nanoseconds((size_t) (mCurrTimeStep * 1e9f)),
		movement, MousePos, glfwGetMouseButton(mWindow->GetNativeHandle(), GLFW_MOUSE_BUTTON_1));

	return HasMoved;
}

void ComputePipelineTester::RayTrace(vk::CommandBuffer commandBuffer, vkEngine::Framebuffer renderTarget)
{
	// Update Camera...
	bool CameraMoved = UpdateCamera(mEditorCamera);

	glm::mat4 ViewMatrix = mEditorCamera.GetViewMatrix();

	mCameraData.mCamera.Position = ViewMatrix[3];

	mCameraData.mCamera.Front = ViewMatrix[2];
	mCameraData.mCamera.Tangent = ViewMatrix[0];
	mCameraData.mCamera.Bitangent = ViewMatrix[1];

	if(CameraMoved)
		mEstimator->SetCamera(mCameraData.mCamera);

	mComputeTraceResult = mEstimator->Trace(commandBuffer);

	vkEngine::Image Target = mEstimator->GetTargetImage();

	vkEngine::Image ColorAttach = renderTarget.GetColorAttachments()[0];

	vkEngine::ImageBlitInfo blitInfo{};
	blitInfo.DstEndRegion = { 1600, 900 };
	blitInfo.SrcEndRegion = mEstimator->GetImageResolution();

	ColorAttach.BeginCommands(commandBuffer);
	ColorAttach.RecordBlit(Target, blitInfo);
	ColorAttach.EndCommands();
}

void ComputePipelineTester::RayTracerWithWavefront(vk::CommandBuffer commandBuffer,
	vkEngine::Framebuffer renderTarget)
{
	// Update Camera...
	bool CameraMoved = UpdateCamera(mEditorCamera);

	glm::mat4 ViewMatrix = mEditorCamera.GetViewMatrix();

	if (CameraMoved)
		mExecutor.SetCameraView(mEditorCamera.GetViewMatrix());

	mTraceResult = mExecutor.Trace(commandBuffer);

	vkEngine::Image Target = mExecutor.GetPresentable();
	
	vkEngine::Image ColorAttach = renderTarget.GetColorAttachments()[0];
	
	vkEngine::ImageBlitInfo blitInfo{};
	blitInfo.DstEndRegion = { 1600, 900 };
	blitInfo.SrcEndRegion = mExecutor.GetTargetResolution();
	
	ColorAttach.BeginCommands(commandBuffer);
	ColorAttach.RecordBlit(Target, blitInfo);
	ColorAttach.EndCommands();
}

void ComputePipelineTester::PresentScreen(uint32_t FrameIndex, vkEngine::SwapchainFrame& ActiveFrame)
{
	vk::PresentInfoKHR presentInfo{};

	uint32_t ImageIndices = FrameIndex;
	vk::Result result = vk::Result::eSuccess;
	presentInfo.setImageIndices(ImageIndices);
	presentInfo.setSwapchains(*mSwapchain->GetHandle());
	presentInfo.setResults(result);
	presentInfo.setWaitSemaphores(ActiveFrame.ImageRendered);

	auto Result = mComputeWorker[0]->PresentKHR(presentInfo);

	_STL_ASSERT(Result == vk::Result::eSuccess, "Could not present image!");
}

AquaFlow::CameraMovementFlags ComputePipelineTester::GetMovementFlags() const
{
	AquaFlow::CameraMovementFlags movementFlags;

	auto ChainMovementFlags = [&movementFlags, this](uint32_t key, AquaFlow::CameraMovement flag)
	{
		if (glfwGetKey(mWindow->GetNativeHandle(), key))
			movementFlags.SetFlag(flag);
	};

	ChainMovementFlags(GLFW_KEY_W, AquaFlow::CameraMovement::eForward);
	ChainMovementFlags(GLFW_KEY_S, AquaFlow::CameraMovement::eBackward);
	ChainMovementFlags(GLFW_KEY_D, AquaFlow::CameraMovement::eRight);
	ChainMovementFlags(GLFW_KEY_A, AquaFlow::CameraMovement::eLeft);
	ChainMovementFlags(GLFW_KEY_E, AquaFlow::CameraMovement::eUp);
	ChainMovementFlags(GLFW_KEY_Q, AquaFlow::CameraMovement::eDown);

	return movementFlags;
}

void ComputePipelineTester::SetupCameras()
{
	mVisualizerCamera.SetLinearSpeed(0.5f);
	mVisualizerCamera.SetRotationSpeed(0.05f);

	mVisualizerCamera.SetHorizontalScale(1.0f);
	mVisualizerCamera.SetVerticalScale(1.0f);

	mEditorCamera.SetLinearSpeed(2.0f);
	mEditorCamera.SetRotationSpeed(0.05f);

	mEditorCamera.SetHorizontalScale(1.0f);
	mEditorCamera.SetVerticalScale(1.0f);
}

void ComputePipelineTester::SetScene()
{
	uint32_t PostProcessing = aiProcess_GenSmoothNormals
		| aiProcess_GenUVCoords | aiProcess_Triangulate;

	//PostProcessing = aiProcess_GenUVCoords | aiProcess_Triangulate;

	AquaFlow::MeshLoader loader(PostProcessing);

	auto WineGlass = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Blender Models\\WineGlass.obj");
	auto Table = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Blender Models\\Table.obj");

	auto Cube = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Blender Models\\Cube.obj");
	auto Cup = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Blender Models\\Cup.obj");
	auto Sphere = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Blender Models\\Sphere.obj");

	// Setting the Path tracer...

	PhFlux::TraceInfo traceInfo;
	traceInfo.BVH_Depth = 24;
	traceInfo.CameraInfo = mCameraData.mCamera;
	traceInfo.MinBounceLimit = 12;
	traceInfo.MaxBounceLimit = 12;
	traceInfo.MaxSamples = 4096;
	traceInfo.PixelSamplesPerBatch = 2;

	PhFlux::Material material;
	material.Albedo = { 0.6f, 0.0f, 0.6f };
	//material.Albedo = { 1.0f, 1.0f, 1.0f };
	material.Metallic = 0.2f;
	//material.Metallic = 1.0f;
	material.Roughness = 0.6f;
	//material.Roughness = 0.000001f;
	//material.Reflectivity = { 0.04f, 0.04f, 0.04f };
	material.RefractiveIndex = 4.5f;
	material.Transmission = 0.0f;

	mEstimator->Begin(traceInfo);
	//mEstimator->SubmitRenderable(Cube[0], material, PhFlux::RenderableType::eObject);

	material.Albedo = { 229.0f / 256.0f, 184.0f / 256.0f, 11.0f / 256.0f };
	material.Albedo = { 1.0f, 1.0f, 1.0f };
	material.Metallic = 0.0f;
	material.Roughness = 0.0001f;
	//material.Reflectivity = material.Albedo;
	material.RefractiveIndex = 1.5f;
	material.Transmission = 1.0f;

	//mEstimator->SubmitRenderable(Cup[0], material, PhFlux::RenderableType::eObject);

	material.Albedo = { 1.0f, 1.0f, 1.0f };
	material.Metallic = 0.9f;
	material.Roughness = 0.001f;
	material.RefractiveIndex = 4.5f;
	material.Transmission = 0.0f;

	//mEstimator->SubmitRenderable(Cube[4], material, PhFlux::RenderableType::eObject);

	material.Albedo = { 0.6f, 0.6f, 0.6f };
	material.Metallic = 0.0f;
	material.Roughness = 0.1f;
	material.RefractiveIndex = 4.5f;
	material.Transmission = 0.0f;

	//for(auto& elem : Table)
	//	mEstimator->SubmitRenderable(elem, material, PhFlux::RenderableType::eObject);

	material.Albedo = { 1.0f, 1.0f, 1.0f };
	material.Metallic = 0.0f;
	material.Roughness = 0.001f;
	material.RefractiveIndex = 1.5f;
	material.Transmission = 1.0f;

	mEstimator->SubmitRenderable(WineGlass[0], material, PhFlux::RenderableType::eObject);

	material.Albedo = { 229.0f / 256.0f, 184.0f / 256.0f, 11.0f / 256.0f };
	material.Albedo = { 1.0f, 0.0f, 1.0f };
	material.Metallic = 0.0f;
	material.Roughness = 0.000001f;
	material.RefractiveIndex = 1.5f;
	material.Transmission = 1.0f;

	mEstimator->SubmitRenderable(Cube[3], material, PhFlux::RenderableType::eObject);

	material.Albedo = { 229.0f / 256.0f, 184.0f / 256.0f, 11.0f / 256.0f };
	material.Albedo = { 192.0f / 256.0f, 192.0f / 256.0f, 192.0f / 256.0f };
	//material.Albedo = { 1.0f, 0.0f, 1.0f };
	material.Metallic = 0.9f;
	material.Roughness = 0.2f;
	material.RefractiveIndex = 4.5f;
	material.Transmission = 0.0f;

	mEstimator->SubmitRenderable(Cube[2], material, PhFlux::RenderableType::eObject);

	material.Albedo = { 10.0f, 10.0f, 10.0f };
	//mEstimator->SubmitRenderable(Cube[1], material, PhFlux::RenderableType::eLightSrc);

	mEstimator->End();
}

void ComputePipelineTester::SetSceneWavefront()
{
	CreateMaterialPipelines();

	uint32_t PostProcessing = aiProcess_GenSmoothNormals
		| aiProcess_GenUVCoords | aiProcess_Triangulate;

	//PostProcessing = aiProcess_GenUVCoords | aiProcess_Triangulate;

	AquaFlow::MeshLoader loader(PostProcessing);

	auto WineGlass = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Blender Models\\WineGlass.obj");
	auto Table = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Blender Models\\Table.obj");

	auto Cube = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Blender Models\\Cube.obj");
	auto Cup = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Blender Models\\Cup.obj");
	auto Sphere = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Blender Models\\Sphere.obj");
	auto Plane = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Blender Models\\Plane.obj");

	AquaFlow::PhFlux::ExecutorCreateInfo executorInfo{};
	executorInfo.TargetResolution = { 1920, 1080 };
	//executorInfo.TargetResolution = { 1024, 1024 };
	executorInfo.TileSize = executorInfo.TargetResolution;
	executorInfo.AllowSorting = false;

	mExecutor = mWavefrontEstimator->CreateExecutor(executorInfo);

	std::vector<AquaFlow::PhFlux::MaterialPipeline> materialPipelines;

	// TODO: Loading a texture from the disk...

	// Set the texture to diffuse material at set = 1, and binding = 0 point...

	// Continue...

	materialPipelines.push_back(mDiffuseMaterial);
	materialPipelines.push_back(mGlossyMaterial);
	materialPipelines.push_back(mRefractionMaterial);
	materialPipelines.push_back(mCookTorranceMaterial);
	materialPipelines.push_back(mGlassMaterial);

	mExecutor.SetMaterialPipelines(materialPipelines.begin(), materialPipelines.end());

	mTraceSession = mWavefrontEstimator->CreateTraceSession();

	// Setting the path tracer...

	//mEditorCamera.SetPosition({ 0.0f, -1.0f, 0.0f });
	//mEditorCamera.SetOrientation({
	//	glm::vec3(1.0f, 0.0f, 0.0f),
	//	glm::vec3(0.0f, 0.0f, 1.0f),
	//	glm::vec3(0.0f, 1.0f, 0.0f)
	//	});
	
	AquaFlow::PhFlux::PhysicalCamera cameraSpecs{};
	cameraSpecs.ApertureSize = 2.8f;
	cameraSpecs.SensorSize = glm::vec2(36.0f, 9.0f / 16.0f * 36.0f);

	cameraSpecs.FocalLength = 26.0f;

	AquaFlow::PhFlux::WavefrontTraceInfo traceInfo{};
	traceInfo.CameraView = mEditorCamera.GetViewMatrix();
	traceInfo.MaxSamples = 4096;
	traceInfo.MinBounceLimit = 1;
	traceInfo.MaxBounceLimit = 8;

	traceInfo.CameraSpecs = cameraSpecs;

	mTraceSession.Begin(traceInfo);

	int nodeTreeDepth = 12;

	auto SubmitRenderable = [this, nodeTreeDepth](AquaFlow::MeshData& mesh, uint32_t matIdx)
	{
		mesh.SetMaterialRef(matIdx);
		mTraceSession.SubmitRenderable(mesh, nodeTreeDepth);
	};

	auto SubmitLightSrc = [this, nodeTreeDepth](AquaFlow::MeshData& mesh, const glm::vec3& color)
	{
		mesh.SetMaterialRef(0);
		mTraceSession.SubmitLightSrc(mesh, color, nodeTreeDepth);
	};

	SubmitRenderable(Cube[5], 0);
	SubmitRenderable(Cube[1], 3);
	//SubmitRenderable(Cube[4], 1);

	SubmitLightSrc(Cube[0], { 10.0f, 10.0f, 10.0f });

	mTraceSession.End();

	mExecutor.SetTraceSession(mTraceSession);
}

void ComputePipelineTester::CreateMaterialPipelines()
{
	AquaFlow::PhFlux::MaterialCreateInfo createInfo;

	std::string directory = "../AquaFlow/include/Shaders/MaterialTests/";
	std::string imageDirectory = "C:/Users/Navjot Singh/Desktop/Sprites/";

	bool FileFound = vkEngine::ReadFile(directory + "Diffuse.glsl",
		createInfo.ShaderCode);

	_VK_ASSERT(FileFound, "Could not read file at: " << directory << "Diffuse.glsl");

	AquaFlow::StbImage imageLoader;
	imageLoader.SetMemoryManager(mMemoryResourceManager);

	auto image = imageLoader.CreateImageBuffer(imageDirectory + "Chess background.jpg");

	vkEngine::SamplerInfo samplerInfo{};

	auto sampler = mMemoryResourceManager.CreateSampler(samplerInfo);

	image.SetSampler(sampler);

#if 0

	createInfo.ShaderCode = R"(
	
	import DiffuseBSDF
	
	SampleInfo Evaluate(in Ray ray, in CollisionInfo collisionInfo)
	{
		DiffuseBSDF_Input diffuseInput;
		diffuseInput.ViewDir = -ray.Direction;
		diffuseInput.Normal = collisionInfo.Normal;
		diffuseInput.BaseColor = vec3(0.6, 0.0, 0.6);
	
		SampleInfo sampleInfo = SampleDiffuseBSDF(diffuseInput);
	
		sampleInfo.Luminance = DiffuseBSDF(diffuseInput, sampleInfo);
	
		return sampleInfo;
	}
		)";
#endif

	mDiffuseMaterial = mWavefrontEstimator->CreateMaterialPipeline(createInfo);
	mDiffuseMaterial.InsertImageResource({ 1, 0, 0 }, image);

	createInfo.ShaderCode = R"(
	import GlossyBSDF

	SampleInfo Evaluate(in Ray ray, in CollisionInfo collisionInfo)
	{
		GlossyBSDF_Input glossyInput;
		glossyInput.ViewDir = -ray.Direction;
		glossyInput.Normal = collisionInfo.Normal;
		glossyInput.BaseColor = vec3(1.0, 1.0, 1.0);
		glossyInput.BaseColor = vec3(192.0 / 256.0);
		glossyInput.Roughness = 0.0001;

		SampleInfo sampleInfo = SampleGlossyBSDF(glossyInput);

		sampleInfo.Luminance = GlossyBSDF(glossyInput, sampleInfo);

		return sampleInfo;
	}
		)";

	mGlossyMaterial = mWavefrontEstimator->CreateMaterialPipeline(createInfo);

	createInfo.ShaderCode = R"(
	import RefractionBSDF

	SampleInfo Evaluate(in Ray ray, in CollisionInfo collisionInfo)
	{
		RefractionBSDF_Input refractionInput;
		refractionInput.ViewDir = -ray.Direction;
		refractionInput.Normal = collisionInfo.Normal;
		refractionInput.BaseColor = vec3(0.0, 1.0, 1.0);
		refractionInput.Roughness = 0.1;
		refractionInput.RefractiveIndex = 1.33;
		refractionInput.NormalInverted = collisionInfo.NormalInverted;

		SampleInfo sampleInfo = SampleRefractionBSDF(refractionInput);

		sampleInfo.Luminance = RefractionBSDF(refractionInput, sampleInfo);

		return sampleInfo;
	}
		)";

	mRefractionMaterial = mWavefrontEstimator->CreateMaterialPipeline(createInfo);

	createInfo.ShaderCode = R"(
	import CookTorranceBSDF

	SampleInfo Evaluate(in Ray ray, in CollisionInfo collisionInfo)
	{
		CookTorranceBSDF_Input cookTorranceInput;
		cookTorranceInput.ViewDir = -ray.Direction;
		cookTorranceInput.Normal = collisionInfo.Normal;
		cookTorranceInput.BaseColor = vec3(1.0, 1.0, 1.0);
		cookTorranceInput.Roughness = 0.7;
		cookTorranceInput.Roughness = 0.00001;
		cookTorranceInput.Metallic = 0.0;
		cookTorranceInput.RefractiveIndex = 1.5;
		cookTorranceInput.TransmissionWeight = 1.0;
		cookTorranceInput.NormalInverted = collisionInfo.NormalInverted;

		SampleInfo sampleInfo = SampleCookTorranceBSDF(cookTorranceInput);

		sampleInfo.Luminance = CookTorranceBSDF(cookTorranceInput, sampleInfo);

		return sampleInfo;
	}
		)";

	mCookTorranceMaterial = mWavefrontEstimator->CreateMaterialPipeline(createInfo);

	createInfo.ShaderCode = R"(
	import GlassBSDF

	SampleInfo Evaluate(in Ray ray, in CollisionInfo collisionInfo)
	{
		GlassBSDF_Input glassInput;
		glassInput.ViewDir = -ray.Direction;
		glassInput.Normal = collisionInfo.Normal;
		glassInput.BaseColor = vec3(1.0, 1.0, 1.0);
		glassInput.Roughness = 0.00001;
		glassInput.RefractiveIndex = 1.5;
		glassInput.NormalInverted = collisionInfo.NormalInverted;

		SampleInfo sampleInfo = SampleGlassBSDF(glassInput);

		sampleInfo.Luminance = GlassBSDF(glassInput, sampleInfo);

		return sampleInfo;
	}
		)";

	mGlassMaterial = mWavefrontEstimator->CreateMaterialPipeline(createInfo);
}

void ComputePipelineTester::FillWavefrontHostBuffers()
{
	auto RayBuffer = mExecutor.GetRayBuffer();
	auto CollisionInfoBuffer = mExecutor.GetCollisionBuffer();
	auto RayRefBuffer = mExecutor.GetRayRefBuffer();
	auto RayInfoBuffer = mExecutor.GetRayInfoBuffer();
	auto HostRefCounts = mExecutor.GetMaterialRefCounts();

	mHostRayBuffer.clear();
	mHostCollisionInfoBuffer.clear();
	mHostRayRefBuffer.clear();
	mHostRayInfoBuffer.clear();
	mHostRefCounts.clear();

	RayBuffer >> mHostRayBuffer;
	CollisionInfoBuffer >> mHostCollisionInfoBuffer;
	RayRefBuffer >> mHostRayRefBuffer;
	RayInfoBuffer >> mHostRayInfoBuffer;
	HostRefCounts >> mHostRefCounts;
}

void ComputePipelineTester::PrintWavefrontRayHostBuffer()
{
	uint32_t index = 0;

	std::for_each(mHostRayBuffer.begin(), mHostRayBuffer.end(),
		[&index](const AquaFlow::PhFlux::Ray& ray)
	{
		std::cout << index++ << ": " << ray.Origin << ", " << 
			ray.Direction << ", " << ray.MaterialIndex << std::endl;
	});
}

void ComputePipelineTester::PrintWavefrontCollisionInfoHostBuffer()
{
	uint32_t index = 0;

	std::for_each(mHostCollisionInfoBuffer.begin(), mHostCollisionInfoBuffer.end(),
		[&index](const AquaFlow::PhFlux::CollisionInfo& info)
	{
		std::cout << index++ << ": " << info.Normal << ", " <<
			info.IntersectionPoint << ", " << info.HitOccured << ", " << info.IsLightSrc <<
			", " << info.MaterialIndex << std::endl;
	});
}

void ComputePipelineTester::PrintWavefrontRayRefHostBuffer()
{
	uint32_t index = 0;

	std::for_each(mHostRayRefBuffer.begin(), mHostRayRefBuffer.end(),
		[&index](const AquaFlow::PhFlux::RayRef& info)
	{
		std::cout << index++ << ": " << info.ElemIdx << ", " << info.CompareElem << std::endl;
	});
}

void ComputePipelineTester::PrintWavefrontMaterialRefCountHostBuffer()
{
	uint32_t index = 0;

	std::for_each(mHostRefCounts.begin(), mHostRefCounts.end(),
		[&index](uint32_t info)
	{
		std::cout << index++ << ": " << info << std::endl;
	});
}

void ComputePipelineTester::PrintWavefrontRayInfoHostBuffer()
{
	uint32_t index = 0;

	std::for_each(mHostRayInfoBuffer.begin(), mHostRayInfoBuffer.end(),
		[&index](const AquaFlow::PhFlux::RayInfo& info)
	{
		std::cout << index++ << 
			": Coordinates: " << info.ImageCoordinate <<
			", Luminance: " << info.Luminance << std::endl;
	});
}

#include "MaterialParser/ExprParser.h"

void ComputePipelineTester::TestNodeSystem()
{
	// Testing expr parser first...

	using namespace AQUA_NAMESPACE;

	std::string expr = "sin(x) + cos(x)";

	Lexer lexer;
	lexer.SetString(expr);
	lexer.SetWhiteSpacesAndDelimiters(" \t\r", "()+-*/");

	auto tokens = lexer.Lex();

	std::vector<ExpressionOperation> ops;
	ops.emplace_back("+", 1, 2, NodeType::eMathsExpr);
	ops.emplace_back("-", 1, 2, NodeType::eMathsExpr);
	ops.emplace_back("*", 2, 2, NodeType::eMathsExpr);
	ops.emplace_back("/", 2, 2, NodeType::eMathsExpr);

	ExprParser parser;
	parser.SetTokenStream(tokens.begin(), tokens.end());
	parser.SetOps(ops.begin(), ops.end());

	NodeAST* node = parser.ParseExpr();

	delete node;
}

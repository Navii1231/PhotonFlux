#include "ComputePipelineTester.h"
#include "../Utils/Utils.h"

bool ComputePipelineTester::OnStart()
{
	PhFlux::EstimatorCreateInfo createInfo{ *mDevice };
	createInfo.ShaderDirectory = "D:/Dev/VulkanEngine/vkEngineTester/Shaders/RayTracer";
	createInfo.TargetResolution = { 1920, 1080 };
	createInfo.TileSize = { 240, 135 };
	//createInfo.TileSize = { 1920, 1080 };

	mEstimator = std::make_shared<PhFlux::ComputeEstimator>(createInfo);

	MeshLoader loader(0);

	mCube = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Cube.obj");
	mCup = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Cup.obj");

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

	SetScene();

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

	ActiveFrame.CmdBuffer.begin(vk::CommandBufferBeginInfo());

	// Ray tracing...

#if 1
	RayTrace(ActiveFrame, Data);
#elif 1
	//if (i % 2 == 0)
		//VisualizeBVH(ActiveFrame, Data);
	//else
		VisualizeMesh(ActiveFrame, Data);
#elif 1
	VisualizeSamples(ActiveFrame, Data);
#else
	VisualizeMesh(ActiveFrame, Data);
#endif

	ActiveFrame.CmdBuffer.end();

	vk::SubmitInfo submitInfo{};

	vk::PipelineStageFlags waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

	submitInfo.setCommandBuffers(ActiveFrame.CmdBuffer);
	submitInfo.setWaitDstStageMask(waitStages);
	submitInfo.setWaitSemaphores(*Data.ImageAcquired);
	submitInfo.setSignalSemaphores(ActiveFrame.ImageRendered);

	uint32_t index = mGraphicsWorker.SubmitWork(submitInfo);

	ActiveFrame.RenderTarget.TransitionColorAttachmentLayouts(vk::ImageLayout::ePresentSrcKHR,
		vk::PipelineStageFlagBits::eTopOfPipe);

	mGraphicsWorker[index]->WaitIdle();

	PresentScreen(FrameIndex.value, ActiveFrame);

	return true;
}

void ComputePipelineTester::MoveCamera()
{
	auto MoveCamera = [this](uint32_t key, const glm::vec3& movement)
	{
		if (glfwGetKey(mWindow->GetNativeHandle(), key))
			mCameraData.mCamera.Position += movement;
	};

	float speed = 1.5f;

	glm::vec3 movement = { 0.0f, 0.0f, 1.0f };
	MoveCamera(GLFW_KEY_W, movement * speed);

	movement = { 0.0f, 0.0f, -1.0f };
	MoveCamera(GLFW_KEY_S, movement * speed);

	movement = { -1.0f, 0.0f, 0.0f };
	MoveCamera(GLFW_KEY_A, movement * speed);

	movement = { 1.0f, 0.0f, 1.0f };
	MoveCamera(GLFW_KEY_D, movement * speed);

 	movement = { 0.0f, 1.0f, 0.0f };
	MoveCamera(GLFW_KEY_E, movement * speed);

 	movement = { 0.0f, -1.0f, 0.0f };
	MoveCamera(GLFW_KEY_Q, movement * speed);
}

bool ComputePipelineTester::UpdateCamera(float elaspedTime)
{
	bool Moved = false;
		
	double cursorPos[2];

	glfwGetCursorPos(mWindow->GetNativeHandle(), &cursorPos[0], &cursorPos[1]);
	bool MouseButtonPressed = glfwGetMouseButton(mWindow->GetNativeHandle(), GLFW_MOUSE_BUTTON_1);

	float xpos = static_cast<float>(cursorPos[0]);
	float ypos = static_cast<float>(cursorPos[1]);

	float speed = 1.5f * elaspedTime;
	//float orientSpeed = 30.0f * elaspedTime;
	float orientSpeed = 0.3f;

	auto MoveCamera = [this, speed, &Moved](
		const glm::vec3& direction, uint32_t key)
	{
		if (glfwGetKey(mWindow->GetNativeHandle(), key))
		{
			Moved |= true;
			mCameraData.mCamera.Position += direction * speed;
		}
	};

	MoveCamera(mCameraData.mCamera.Front, GLFW_KEY_W);
	MoveCamera(-mCameraData.mCamera.Front, GLFW_KEY_S);
	MoveCamera(mCameraData.mCamera.Tangent, GLFW_KEY_D);
	MoveCamera(-mCameraData.mCamera.Tangent, GLFW_KEY_A);
	MoveCamera(mCameraData.mCamera.Bitangent, GLFW_KEY_E);
	MoveCamera(-mCameraData.mCamera.Bitangent, GLFW_KEY_Q);

	glm::mat4 invertY = glm::mat4(1.0f);
	invertY[1][1] = -1.0f;

	bool Rotated = true;

	// Function to update camera direction based on mouse movement
	if (mCameraData.mFirstMouse || !MouseButtonPressed) {
		mCameraData.mLastX = xpos;
		mCameraData.mLastY = ypos;
		mCameraData.mFirstMouse = false;

		Rotated = false;
		return Moved;
	}

	Moved |= Rotated;

	float xoffset = (xpos - mCameraData.mLastX) * orientSpeed;
	float yoffset = (ypos - mCameraData.mLastY) * orientSpeed;  // Reversed since y-coordinates go from bottom to top
	mCameraData.mLastX = xpos;
	mCameraData.mLastY = ypos;

	float sensitivity = 0.1f;  // Change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	mCameraData.mYaw += xoffset;
	mCameraData.mPitch += yoffset;

	// Constrain the pitch to avoid flipping
	if (mCameraData.mPitch > 89.0f)
		mCameraData.mPitch = 89.0f;
	if (mCameraData.mPitch < -89.0f)
		mCameraData.mPitch = -89.0f;

	// Calculate the new Front vector
	glm::mat4 rotationMatrix(1.0f);
	rotationMatrix = glm::rotate(rotationMatrix,
		glm::radians(mCameraData.mYaw), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw rotation

	rotationMatrix = glm::rotate(rotationMatrix,
		glm::radians(mCameraData.mPitch), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch rotation

	glm::vec4 front = rotationMatrix * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
	glm::vec4 tangent = rotationMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

	mCameraData.mCamera.Front = glm::normalize(glm::vec3(front));
	mCameraData.mCamera.Tangent = glm::normalize(glm::vec3(tangent));
	mCameraData.mCamera.Bitangent = glm::cross(mCameraData.mCamera.Front, mCameraData.mCamera.Tangent);

	return Moved;
}

void ComputePipelineTester::RayTrace(vkEngine::SwapchainFrame& ActiveFrame, vkEngine::SwapchainData& Data)
{
	// Update Camera...
	double cursorPos[2];

	glfwGetCursorPos(mWindow->GetNativeHandle(), &cursorPos[0], &cursorPos[1]);
	bool MouseButtonPressed = glfwGetMouseButton(mWindow->GetNativeHandle(), GLFW_MOUSE_BUTTON_1);

	bool CameraMoved = UpdateCamera(mCurrTimeStep);

	if(CameraMoved)
		mEstimator->SetCamera(mCameraData.mCamera);

	mTraceResult = mEstimator->Trace(ActiveFrame.CmdBuffer);

	vkEngine::Image Target = mEstimator->GetTargetImage();

	vkEngine::Image ColorAttach = ActiveFrame.RenderTarget.GetColorAttachments()[0];

	vkEngine::ImageBlitInfo blitInfo{};
	blitInfo.DstEndRegion = { 1600, 900 };
	blitInfo.SrcEndRegion = mEstimator->GetImageResolution();

	ColorAttach.BeginCommands(ActiveFrame.CmdBuffer);
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

void ComputePipelineTester::SetScene()
{
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
	mEstimator->SubmitRenderable(mCube[0], material, PhFlux::RenderableType::eObject);

	material.Albedo = { 1.0f, 1.0f, 1.0f };
	material.Albedo = { 229.0f / 256.0f, 184.0f / 256.0f, 11.0f / 256.0f };
	material.Metallic = 0.8f;
	material.Roughness = 0.0001f;
	//material.Reflectivity = material.Albedo;
	material.RefractiveIndex = 1.5f;
	material.Transmission = 0.0f;

	//mEstimator->SubmitRenderable(mCup[0], material, PhFlux::RenderableType::eObject);

	material.Albedo = { 1.0f, 1.0f, 1.0f };
	material.Metallic = 0.9f;
	material.Roughness = 0.001f;
	material.RefractiveIndex = 4.5f;
	material.Transmission = 0.0f;

	mEstimator->SubmitRenderable(mCube[4], material, PhFlux::RenderableType::eObject);

#if 0
	material.Albedo = { 1.0f, 1.0f, 1.0f };
	material.Albedo = { 0.0f, 10.0f, 10.0f };
	material.Metallic = 0.9f;
	material.Roughness = 0.001f;
	material.RefractiveIndex = 4.5f;
	material.Transmission = 0.0f;

	//mEstimator->SubmitRenderable(mCube[4], material, PhFlux::RenderableType::eObject);
	mEstimator->SubmitRenderable(mCube[5], material, PhFlux::RenderableType::eLightSrc);

	material.Albedo = { 0.0f, 0.0f, 10.0f };

	mEstimator->SubmitRenderable(mCube[6], material, PhFlux::RenderableType::eLightSrc);

#endif

	material.Albedo = { 229.0f / 256.0f, 184.0f / 256.0f, 11.0f / 256.0f };
	material.Albedo = { 1.0f, 1.0f, 1.0f };
	material.Metallic = 0.0f;
	material.Roughness = 0.0001f;
	material.RefractiveIndex = 1.5f;
	material.Transmission = 1.0f;

	//mEstimator->SubmitRenderable(mCube[3], material, PhFlux::RenderableType::eObject);

	material.Albedo = { 0.0f, 0.0f, 1.0f };
	material.Albedo = { 229.0f / 256.0f, 184.0f / 256.0f, 11.0f / 256.0f };
	material.Albedo = { 192.0f / 256.0f, 192.0f / 256.0f, 192.0f / 256.0f };
	material.Metallic = 0.95f;
	material.Roughness = 0.001f;
	material.RefractiveIndex = 4.5f;
	material.Transmission = 0.0f;

	mEstimator->SubmitRenderable(mCube[2], material, PhFlux::RenderableType::eObject);

	material.Albedo = { 10.0f, 10.0f, 10.0f };
	mEstimator->SubmitRenderable(mCube[1], material, PhFlux::RenderableType::eLightSrc);

	mEstimator->End();
}

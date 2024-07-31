#include "GraphicsPipelineTester.h"



bool GraphicsPipelineTester::OnStart()
{
	mQuad.first.push_back({ glm::vec3(-0.5f,  0.5f, 0.2f), glm::vec3(0.0f, 1.0f, 0.0f) });
	mQuad.first.push_back({ glm::vec3(0.5f,  0.5f, 0.2f), glm::vec3(0.0f, 1.0f, 0.0f) });
	mQuad.first.push_back({ glm::vec3(0.5f, -0.5f, 0.2f), glm::vec3(0.0f, 1.0f, 0.0f) });
	mQuad.first.push_back({ glm::vec3(-0.5f, -0.5f, 0.2f), glm::vec3(0.0f, 1.0f, 0.0f) });

	mQuad1.first.push_back({ glm::vec3(-1.0f,  0.5f, 0.3f), glm::vec3(1.0f, 0.0f, 0.5f) });
	mQuad1.first.push_back({ glm::vec3(0.0f,  0.5f, 0.3f), glm::vec3(1.0f, 0.0f, 0.5f) });
	mQuad1.first.push_back({ glm::vec3(0.0f, -0.5f, 0.3f), glm::vec3(1.0f, 0.0f, 0.5f) });
	mQuad1.first.push_back({ glm::vec3(-1.0f, -0.5f, 0.3f), glm::vec3(1.0f, 0.0f, 0.5f) });

	mQuad.second = { 0, 1, 2, 2, 3, 0 };
	mQuad1.second = { 0, 1, 2, 2, 3, 0 };

	mPipelineBuilder = mDevice->MakePipelineBuilder();
	mMemoryResourceManager = mDevice->MakeMemoryResourceManager();

	mContextBuilder = mDevice->FetchRenderContextBuilder(vk::PipelineBindPoint::eGraphics);

	vkEngine::ImageAttachmentInfo attachmentConfig{};
	attachmentConfig.Layout = vk::ImageLayout::eColorAttachmentOptimal;
	attachmentConfig.Usage = vk::ImageUsageFlagBits::eColorAttachment;

	vkEngine::ImageAttachmentInfo depthAttachment{};
	depthAttachment.Format = vk::Format::eD24UnormS8Uint;
	depthAttachment.Layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	depthAttachment.Usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;

	vkEngine::RenderContextCreateInfo createInfo{};
	createInfo.ColorAttachments.push_back(attachmentConfig);
	createInfo.DepthStencilAttachment = depthAttachment;
	createInfo.UsingDepthAttachment = true;
	createInfo.UsingStencilAttachment = true;

	mContext = mContextBuilder.MakeContext(createInfo);

	MyPipelineConfig config(mContext, 1600.0f, 900.0f);

	config.mDevice = mDevice->GetHandle();

	CameraUniform cameraInfo{};
	cameraInfo.Projection = glm::mat4(1.0f);
	// glm::perspective(glm::radians(90.0f), 1600.0f / 900.0f, 0.01f, 100.0f);
	cameraInfo.View = glm::mat4(1.0f);
	//glm::lookAt(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	std::vector<CameraUniform> uniformCamera = { cameraInfo };

	vkEngine::BufferCreateInfo bufferInfo{};
	bufferInfo.Size = 0;
	bufferInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;
	bufferInfo.Usage = vk::BufferUsageFlagBits::eUniformBuffer;

	config.mCameraUniform = mMemoryResourceManager.CreateBuffer<CameraUniform>(bufferInfo);

	config.mCameraUniform << uniformCamera;

	std::vector<vkEngine::ShaderInput> shaders;
	shaders.push_back({ {}, vk::ShaderStageFlagBits::eVertex, "Shaders/BasicVertex.vert" });
	shaders.push_back({ {}, vk::ShaderStageFlagBits::eFragment, "Shaders/BasicFragment.frag" });

	auto Errors = config.CompileShaders(shaders);

	for (const auto& Error : Errors)
	{
		std::cout << Error.Info << std::endl;
		_STL_ASSERT(Error.Type == vkEngine::ErrorType::eNone, "Failed compile shaders!");
	}

	mPipeline = mPipelineBuilder.BuildGraphicsPipeline(config);

	mQueueManager = mDevice->GetQueueManager();
	mGraphicsWorker = mQueueManager->FetchExecutor(0, vkEngine::QueueAccessType::eGeneric);

	return true;
}

bool GraphicsPipelineTester::OnUpdate(float ElaspedTime)
{
	auto Data = mSwapchain->GetSwapchainData();

	mGraphicsWorker[0]->WaitIdle();

	auto FrameIndex = mSwapchain->AcquireNextFrame(Data.ImageAcquired);

	_STL_ASSERT(FrameIndex.result == vk::Result::eSuccess, "Could not acquire the image from swapchain");

	auto& ActiveFrame = Data.Frames[FrameIndex.value];

	uint32_t index = Render(*ActiveFrame, Data);

	ActiveFrame->RenderTarget.TransitionColorAttachmentLayouts(vk::ImageLayout::ePresentSrcKHR,
		vk::PipelineStageFlagBits::eTopOfPipe);

	mGraphicsWorker[index]->WaitIdle();

	PresentScreen(FrameIndex.value, *ActiveFrame);

	return true;
}

void GraphicsPipelineTester::PresentScreen(uint32_t FrameIndex, vkEngine::SwapchainFrame& ActiveFrame)
{
	vk::PresentInfoKHR presentInfo{};

	uint32_t ImageIndices = FrameIndex;
	vk::Result result = vk::Result::eSuccess;
	presentInfo.setImageIndices(ImageIndices);
	presentInfo.setSwapchains(*mSwapchain->GetHandle());
	presentInfo.setResults(result);
	presentInfo.setWaitSemaphores(ActiveFrame.ImageRendered);

	auto Result = mGraphicsWorker[0]->PresentKHR(presentInfo);

	_STL_ASSERT(Result == vk::Result::eSuccess, "Could not present image!");
}

uint32_t GraphicsPipelineTester::Render(vkEngine::SwapchainFrame& ActiveFrame, vkEngine::SwapchainData& Data)
{
	vkEngine::Framebuffer renderTarget = ActiveFrame.RenderTarget;
	vkEngine::Framebuffer pipelineTarget = mPipeline.GetRenderTarget();

	pipelineTarget.TransitionColorAttachmentLayouts(vk::ImageLayout::eTransferSrcOptimal,
		vk::PipelineStageFlagBits::eTransfer);

	ActiveFrame.CmdBuffer.reset();
	ActiveFrame.CmdBuffer.begin(vk::CommandBufferBeginInfo());

	std::vector<vk::ClearValue> clearValue(2);
	clearValue[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValue[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

	mPipeline.Begin(ActiveFrame.CmdBuffer, clearValue);
	mPipeline.SubmitRenderable(mQuad);
	mPipeline.SubmitRenderable(mQuad1);
	mPipeline.End();

	vkEngine::BlitInfo blitInfo{};
	blitInfo.Flags = vkEngine::AttachmentTypeFlagBits::eColor;

	blitInfo.BlitInfo.SrcBeginRegion = { 0, 0 };
	blitInfo.BlitInfo.SrcEndRegion = { 1600, 900 };

	blitInfo.BlitInfo.DstBeginRegion = { 0, 0 };
	blitInfo.BlitInfo.DstEndRegion = { 1600, 900 };

	renderTarget.BeginCommands(ActiveFrame.CmdBuffer);
	renderTarget.RecordBlit(pipelineTarget, blitInfo);
	renderTarget.EndCommands();

	ActiveFrame.CmdBuffer.end();

	vk::SubmitInfo submitInfo{};

	vk::PipelineStageFlags WaitFlags[1] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

	submitInfo.setCommandBuffers(ActiveFrame.CmdBuffer);
	submitInfo.setSignalSemaphores(ActiveFrame.ImageRendered);
	submitInfo.setWaitSemaphores(*Data.ImageAcquired);
	submitInfo.setWaitDstStageMask(WaitFlags);

	return mGraphicsWorker.SubmitWork(submitInfo);
}

#include "ComputeEstimator.h"

PH_FLUX_NAMESPACE::ComputeEstimator::ComputeEstimator(const EstimatorCreateInfo& createInfo)
	: mDevice(createInfo.Context)
{
	mPresentableSize = createInfo.TargetResolution;
	mTileSize = createInfo.TileSize;

	mPipelineBuilder = mDevice.MakePipelineBuilder();
	mMemoryManager = mDevice.MakeMemoryResourceManager();

	GenerateTiles(createInfo.TileSize);

	ComputePipelineSettings settings(mMemoryManager);

	vkEngine::ShaderInput input;
	input.FilePath = createInfo.ShaderDirectory + "/main.comp";
	input.Stage = vk::ShaderStageFlagBits::eCompute;

	auto erros = settings.CompileShaders({ input });

	CheckErrors(erros);

	vkEngine::ImageCreateInfo imageInfo{};
	imageInfo.Extent = vk::Extent3D(mPresentableSize.x, mPresentableSize.y, 1);
	imageInfo.MemProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
	imageInfo.Type = vk::ImageType::e2D;
	imageInfo.Usage = vk::ImageUsageFlagBits::eStorage;
	imageInfo.Format = vk::Format::eR8G8B8A8Unorm;

	mPresentable = mMemoryManager.CreateImage(imageInfo);

	imageInfo.Format = vk::Format::eR8G8B8A8Unorm;
	mCameraView = mMemoryManager.CreateImage(imageInfo);

	settings.mTarget.Presentable = mPresentable;
	settings.mTarget.ImageResolution = mPresentableSize;

	mPresentable.TransitionLayout(vk::ImageLayout::eGeneral, 
		vk::PipelineStageFlagBits::eComputeShader);

	mCameraView.TransitionLayout(vk::ImageLayout::eGeneral,
		vk::PipelineStageFlagBits::eComputeShader);

	mEstimator = mPipelineBuilder.BuildComputePipeline(settings);
}

void PH_FLUX_NAMESPACE::ComputeEstimator::Begin(const TraceInfo& beginInfo)
{
	mTraceInfo = beginInfo;

	auto& settings = mEstimator.GetConfigSettings();

	settings.mCameraData.mCamera = beginInfo.CameraInfo;

	settings.mSceneData.FrameCount = 1;
	settings.mSceneData.MaxBounceLimit = beginInfo.MaxBounceLimit;
	settings.mSceneData.PixelSamples = beginInfo.PixelSamplesPerBatch;
	settings.mSceneData.ResetImage = 1;
	settings.mSceneData.RandomSeed = rand();
}

void PH_FLUX_NAMESPACE::ComputeEstimator::SubmitRenderable(
	const MeshData& data, const Material& material, RenderableType type)
{
	auto& settings = mEstimator.GetConfigSettings();
	settings.SubmitRenderable(data, material, type, mTraceInfo.BVH_Depth);
}

void PH_FLUX_NAMESPACE::ComputeEstimator::End()
{
	// TODO: Validate the path tracer configuration and the scene
}

PH_FLUX_NAMESPACE::TraceResult PH_FLUX_NAMESPACE::ComputeEstimator::Trace(vk::CommandBuffer buffer)
{
	auto& settings = mEstimator.GetConfigSettings();

	glm::uvec2 ImageResolution;

	if (!mCameraSet)
	{
		settings.mTarget.PixelMean = mTileImages[mTileIndex];
		ImageResolution = mTiles[mTileIndex].Max - mTiles[mTileIndex].Min;

		settings.mSceneData.MinBound = mTiles[mTileIndex].Min;
		settings.mSceneData.MaxBound = mTiles[mTileIndex].Max;
		settings.mSceneData.ImageResolution = mPresentableSize;
	}
	else
	{
		settings.mTarget.PixelMean = mCameraView;
		ImageResolution = mPresentableSize;

		settings.mSceneData.MinBound = { 0, 0 };
		settings.mSceneData.MaxBound = mPresentableSize;
		settings.mSceneData.ImageResolution = mPresentableSize;
	}

	// Dispatch compute shader...
	settings.mTarget.Presentable.TransitionLayout(
		settings.mBufferLayout, vk::PipelineStageFlagBits::eTopOfPipe);

	settings.mTarget.PixelMean.TransitionLayout(
		settings.mBufferLayout, vk::PipelineStageFlagBits::eTopOfPipe);

	bool IsCameraMoving = mCameraSet;
	mCameraSet = false;

	glm::uvec3 WorkGroups = glm::uvec3(ImageResolution.x, ImageResolution.y, 1) / 
		settings.GetWorkGroupSize();

	WorkGroups += glm::uvec3(1, 1, 0);

	vk::BufferMemoryBarrier barrier;
	barrier.setBuffer(settings.mVerticesLocal.GetNativeHandles().Handle);
	barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
	barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
	barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
	barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
	barrier.setSize(VK_WHOLE_SIZE);

	buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eComputeShader,
		vk::DependencyFlags(), nullptr, barrier, nullptr);

	barrier.setBuffer(settings.mFacesLocal.GetNativeHandles().Handle);

	buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eComputeShader,
		vk::DependencyFlags(), nullptr, barrier, nullptr);

	uint32_t Finished = false;
	settings.mCPU_Feedback.FetchMemory(&Finished, &Finished + 1, 0);

	if (Finished == 0)
	{
		Finished = 1;
		settings.mCPU_Feedback.SetBuf(&Finished, &Finished + 1, 0);
		mEstimator.Dispatch(buffer, WorkGroups);

		settings.mSceneData.ResetImage = IsCameraMoving;

		if(!IsCameraMoving)
			settings.mSceneData.FrameCount++;
		else
			settings.mSceneData.FrameCount = 1;

		settings.mSceneData.MaxBounceLimit = mTraceInfo.MaxBounceLimit;
		settings.mSceneData.PixelSamples = mTraceInfo.PixelSamplesPerBatch;

		return TraceResult::ePending;
	}

	if (mTileIndex == 0)
		return TraceResult::eComplete;

	mTileIndex--;

	settings.mSceneData.ResetImage = 1;
	settings.mSceneData.FrameCount = 1;
	settings.mSceneData.MaxBounceLimit = mTraceInfo.MaxBounceLimit;
	settings.mSceneData.PixelSamples = mTraceInfo.PixelSamplesPerBatch;

	Finished = 0;
	settings.mCPU_Feedback.SetBuf(&Finished, &Finished + 1, 0);

	return TraceResult::ePending;
}

void PH_FLUX_NAMESPACE::ComputeEstimator::SetCamera(const Camera& camera)
{
	auto& settings = mEstimator.GetConfigSettings();
	settings.mCameraData.mCamera = camera;

	settings.mSceneData.FrameCount = 1;
	settings.mSceneData.ResetImage = 1;
	settings.mSceneData.MaxBounceLimit = settings.mResetBounceLimit;
	settings.mSceneData.PixelSamples = settings.mResetSampleCount;

	mCameraSet = true;
	mTileIndex = mTiles.size() - 1;
}

void PH_FLUX_NAMESPACE::ComputeEstimator::CheckErrors(const std::vector<vkEngine::CompileError>& Errors)
{
	for (const auto& error : Errors)
	{
		WriteFile("Logging/ShaderFails/CompilationAST.txt", error.DebugInfo);

		if (error.Type != vkEngine::ErrorType::eNone)
		{
			char* buffer = new char[4096];

			std::string filepath = "Logging/ShaderFails/CompilationFail.glsl";

			bool FileWritten = WriteFile(filepath, error.SrcCode);

			sprintf_s(buffer, 4096, "Failed to compile the shader!\n%s", error.Info.c_str());
			size_t bufSize = strlen(buffer);

			if (!FileWritten)
				sprintf_s(buffer + bufSize, 1024 - bufSize,
					"\nCouldn't dump the source code into file: %s\n", filepath.c_str());

			_STL_ASSERT(false, buffer);

			delete[] buffer;
		}
	}
}

void PH_FLUX_NAMESPACE::ComputeEstimator::GenerateTiles(const glm::ivec2& tileSize)
{
	glm::ivec2 ImageSize = mPresentableSize;
	glm::ivec2 TileCount = ImageSize / tileSize;
	glm::ivec2 Bleed = ImageSize % tileSize;

	glm::ivec2 BleedUpper = Bleed / 2 + Bleed % 2;
	glm::ivec2 BleedLower = Bleed / 2;

	auto Scan = [this, &tileSize](glm::ivec2 Begin, 
		glm::ivec2 End, const glm::ivec2& Dir, int DirIndex)->int
	{
		int Count = glm::max(glm::abs((End - Begin).x * Dir.x) / tileSize.x,
			glm::abs((End - Begin).y * Dir.y) / tileSize.y);

		switch (DirIndex)
		{
			case 0:
				break;
			case 1:
			{
				Begin.x = End.x - tileSize.x;
				break;
			}
			case 2:
			{
				std::swap(Begin, End);
				Begin -= tileSize;
				break;
			}
			case 3:
			{
				End.x = Begin.x + tileSize.x;
				std::swap(Begin, End);
				Begin -= tileSize;
				break;
			}
		}

		for (int i = 0; i < Count; i++)
		{
			Tile tile;
			tile.Min = Begin + i * Dir * tileSize;
			tile.Max = tile.Min + tileSize;

			mTiles.emplace_back(tile);
		}

		return Count;
	};

	glm::ivec2 Directions[4] =
	{
		glm::ivec2(1.0f, 0.0f),
		glm::ivec2(0.0f, 1.0f),
		glm::ivec2(-1.0f, 0.0f),
		glm::ivec2(0.0f, -1.0f),
	};

	int DirIndex = 0;

	glm::ivec2 MinBound = { 0, 0 };
	glm::ivec2 MaxBound = ImageSize;

	int TileNum = -1;

	while (TileNum != 0)
	{
		bool Sign = DirIndex == 0 || DirIndex == 1;

		TileNum = Scan(MinBound, MaxBound, Directions[DirIndex], DirIndex);
		DirIndex = (DirIndex + 1) % 4;

		if(DirIndex == 0 || DirIndex == 1)
			MinBound += Directions[DirIndex] * tileSize;
		else
			MaxBound += Directions[DirIndex] * tileSize;
	}

	mTileIndex = mTiles.size() - 1;
	CreateImageViews(mTiles);
}

void PH_FLUX_NAMESPACE::ComputeEstimator::CreateImageViews(const std::vector<Tile>& tiles)
{
	mTileImages.reserve(tiles.size());

	vkEngine::ImageCreateInfo imageInfo{};

	imageInfo.MemProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
	imageInfo.Type = vk::ImageType::e2D;
	imageInfo.Usage = vk::ImageUsageFlagBits::eStorage;

	imageInfo.Format = vk::Format::eR32G32B32A32Sfloat;

	for (const auto& tile : tiles)
	{
		glm::ivec2 Size = tile.Max - tile.Min;
		imageInfo.Extent = vk::Extent3D(Size.x, Size.y, 1);

		mTileImages.emplace_back(mMemoryManager.CreateImage(imageInfo));
	}
}

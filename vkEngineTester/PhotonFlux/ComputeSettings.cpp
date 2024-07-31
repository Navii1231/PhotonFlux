#include "ComputeSettings.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

PH_BEGIN

ComputePipelineSettings::ComputePipelineSettings(vkEngine::MemoryResourceManager manager)
{
	vkEngine::BufferCreateInfo bufferInfo{};
	bufferInfo.Size = 1;
	bufferInfo.Usage = vk::BufferUsageFlagBits::eUniformBuffer;

	mCamera = manager.CreateBuffer<Camera>(bufferInfo);

	bufferInfo.Size = 0;
	bufferInfo.Usage = vk::BufferUsageFlagBits::eStorageBuffer;

	bufferInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;
	mVertices = manager.CreateBuffer<glm::vec4>(bufferInfo);
	bufferInfo.MemProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
	mVerticesLocal = manager.CreateBuffer<glm::vec4>(bufferInfo);

	bufferInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;
	mFaces = manager.CreateBuffer<glm::uvec4>(bufferInfo);
	bufferInfo.MemProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
	mFacesLocal = manager.CreateBuffer<glm::uvec4>(bufferInfo);

	bufferInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;
	mNodes = manager.CreateBuffer<Node>(bufferInfo);
	bufferInfo.MemProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
	mNodesLocal = manager.CreateBuffer<Node>(bufferInfo);

	bufferInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;
	bufferInfo.Size = 0;
	bufferInfo.Usage = vk::BufferUsageFlagBits::eStorageBuffer;

	mMeshInfos = manager.CreateBuffer<MeshInfo>(bufferInfo);
	mLightInfos = manager.CreateBuffer<LightInfo>(bufferInfo);

	bufferInfo.Size = 1;
	bufferInfo.Usage = vk::BufferUsageFlagBits::eUniformBuffer;

	mSceneInfo = manager.CreateBuffer<SceneInfo>(bufferInfo);

	mSkybox = LoadImage("C:\\Users\\Navjot Singh\\Desktop\\CubeMaps\\Eq7.png", manager);

	// Set 2...

	bufferInfo.Size = 1;
	bufferInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;
	bufferInfo.Usage = vk::BufferUsageFlagBits::eStorageBuffer;

	mCPU_Feedback = manager.CreateBuffer<uint32_t>(bufferInfo);

	uint32_t Finished = false;
	mCPU_Feedback.SetBuf(&Finished, &Finished + 1, 0);

	vkEngine::SamplerInfo samplerInfo{};
	//samplerInfo.AddressModeU = vk::SamplerAddressMode::eRepeat;
	//samplerInfo.AddressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.AnisotropyEnable = true;
	samplerInfo.MaxAnisotropy = 16.0f;
	samplerInfo.MinLod = 0.0f;
	samplerInfo.MaxLod = 1.0f;
	samplerInfo.BorderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.MinFilter = vk::Filter::eLinear;
	samplerInfo.MagFilter = vk::Filter::eLinear;
	samplerInfo.NormalisedCoordinates = true;

	mSkyboxSampler = manager.CreateSampler(samplerInfo);
}

void ComputePipelineSettings::UpdateDescriptors(vkEngine::DescriptorWriter& writer)
{
	mSceneData.RandomSeed = rand();
	mSceneData.LightCount = mLightInfos.GetSize();
	mSceneData.MeshCount = mMeshInfos.GetSize();

	mCamera.SetBuf(&mCameraData.mCamera, &mCameraData.mCamera + 1, 0);
	mSceneInfo.SetBuf(&mSceneData, &mSceneData + 1, 0);

	uint32_t Finished = false;

	mCPU_Feedback.FetchMemory(&Finished, &Finished + 1, 0);
	mCPU_Feedback.SetBuf(&Finished, &Finished + 1, 0);

	vkEngine::StorageBufferWriteInfo CPU_FeedbackInfo{};
	CPU_FeedbackInfo.Buffer = mCPU_Feedback.GetNativeHandles().Handle;

	writer.Update({ 2, 0, 0 }, CPU_FeedbackInfo);

	// Color Buffer...
	vkEngine::StorageImageWriteInfo colorInfo{};
	colorInfo.ImageView = mTarget.PixelMean.GetIdentityImageView();
	colorInfo.ImageLayout = mBufferLayout;

	writer.Update({ 0, 1, 0 }, colorInfo);

	// Image Output...
	colorInfo.ImageView = mTarget.Presentable.GetIdentityImageView();
	colorInfo.ImageLayout = mBufferLayout;

	writer.Update({ 0, 0, 0 }, colorInfo);

	// Camera uniform...
	vkEngine::UniformBufferWriteInfo cameraInfo{};
	cameraInfo.Buffer = mCamera.GetNativeHandles().Handle;

	writer.Update({ 1, 0, 0 }, cameraInfo);

	// Vertices...
	vkEngine::StorageBufferWriteInfo verticesInfo{};
	verticesInfo.Buffer = mVerticesLocal.GetNativeHandles().Handle;

	writer.Update({ 1, 1, 0 }, verticesInfo);

	// Indices...
	vkEngine::StorageBufferWriteInfo indicesInfo{};
	indicesInfo.Buffer = mFacesLocal.GetNativeHandles().Handle;

	writer.Update({ 1, 2, 0 }, indicesInfo);

	vkEngine::StorageBufferWriteInfo nodesInfo{};
	nodesInfo.Buffer = mNodesLocal.GetNativeHandles().Handle;

	writer.Update({ 1, 3, 0 }, nodesInfo);

	// MeshInfos...
	vkEngine::StorageBufferWriteInfo meshInfos{};
	meshInfos.Buffer = mMeshInfos.GetNativeHandles().Handle;

	writer.Update({ 1, 4, 0 }, meshInfos);

	// LightInfos...
	vkEngine::StorageBufferWriteInfo lightInfos{};
	lightInfos.Buffer = mLightInfos.GetNativeHandles().Handle;

	writer.Update({ 1, 5, 0 }, lightInfos);

	// Scene Info...
	vkEngine::UniformBufferWriteInfo sceneInfo{};
	sceneInfo.Buffer = mSceneInfo.GetNativeHandles().Handle;

	writer.Update({ 1, 6, 0 }, sceneInfo);

	// Skybox...
	vkEngine::CombinedImageSamplerWriteInfo samplerInfo{};
	samplerInfo.ImageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	samplerInfo.ImageView = mSkybox.GetIdentityImageView();
	samplerInfo.Sampler = *mSkyboxSampler;

	writer.Update({ 1, 7, 0 }, samplerInfo);
}

void ComputePipelineSettings::UpdateCameraPosition(float deltaTime, float speed, CameraMovementFlags movement)
{
	auto MoveCamera = [this, movement, speed, deltaTime](
		const glm::vec3& direction, CameraMovementFlagBits compare)
	{
		if (movement & compare)
		{
			mCameraData.mCamera.Position += direction * speed * deltaTime;
			mSceneData.ResetImage = 1;
			mSceneData.FrameCount = 1;
			mSceneData.MaxBounceLimit = mResetBounceLimit;
			mSceneData.PixelSamples = mResetSampleCount;
		}
	};

	MoveCamera(mCameraData.mCamera.Front, CameraMovementFlagBits::eForward);
	MoveCamera(-mCameraData.mCamera.Front, CameraMovementFlagBits::eBackward);
	MoveCamera(-mCameraData.mCamera.Tangent, CameraMovementFlagBits::eRight);
	MoveCamera(mCameraData.mCamera.Tangent, CameraMovementFlagBits::eLeft);
	MoveCamera(mCameraData.mCamera.Bitangent, CameraMovementFlagBits::eUp);
	MoveCamera(-mCameraData.mCamera.Bitangent, CameraMovementFlagBits::eDown);
}

void ComputePipelineSettings::UpdateCameraOrientation(float deltaTime, float speed,
	float xpos, float ypos, bool Reset)
{
	// Function to update camera direction based on mouse movement
	if (mCameraData.mFirstMouse || Reset) {
		mCameraData.mLastX = xpos;
		mCameraData.mLastY = ypos;
		mCameraData.mFirstMouse = false;
		return;
	}

	mSceneData.ResetImage = 1;
	mSceneData.FrameCount = 1;
	mSceneData.MaxBounceLimit = mResetBounceLimit;
	mSceneData.PixelSamples = mResetSampleCount;

	float xoffset = (xpos - mCameraData.mLastX) * speed;
	float yoffset = (ypos - mCameraData.mLastY) * speed;  // Reversed since y-coordinates go from bottom to top
	mCameraData.mLastX = xpos;
	mCameraData.mLastY = ypos;

	float sensitivity = 0.1f;  // Change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	mCameraData.mYaw -= xoffset;
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
}

void ComputePipelineSettings::SubmitRenderable(const MeshData& meshData,
	const Material& color, RenderableType type, uint32_t bvhDepth)
{
	BVH_Factory bvhFactory(meshData.aPositions, meshData.aFaces);
	BVH bvhStruct = bvhFactory.MakeBVH(bvhDepth);

	size_t VertexCount = mVertices.GetSize();
	size_t FaceCount = mFaces.GetSize();
	size_t NodeCount = mNodes.GetSize();

	mVertices.Resize(VertexCount + bvhStruct.Vertices.size());
	mFaces.Resize(FaceCount + bvhStruct.Faces.size());
	mNodes.Resize(NodeCount + bvhStruct.Nodes.size());

	glm::vec4* VertexBegin = mVertices.MapMemory(bvhStruct.Vertices.size(), VertexCount);
	glm::uvec4* FaceBegin = mFaces.MapMemory(bvhStruct.Faces.size(), FaceCount);
	Node* NodeBegin = mNodes.MapMemory(bvhStruct.Nodes.size(), NodeCount);

	for (const auto& Pos : bvhStruct.Vertices)
	{
		*VertexBegin = glm::vec4(Pos, 1.0f);
		VertexBegin++;
	}

	for (auto& Face : bvhStruct.Faces)
	{
		FaceBegin->x = Face.x + static_cast<uint32_t>(VertexCount);
		FaceBegin->y = Face.y + static_cast<uint32_t>(VertexCount);
		FaceBegin->z = Face.z + static_cast<uint32_t>(VertexCount);
		FaceBegin++;
	}

	for (auto& node : bvhStruct.Nodes)
	{
		node.BeginIndex += static_cast<uint32_t>(FaceCount);
		node.EndIndex += static_cast<uint32_t>(FaceCount);

		node.FirstChildIndex += NodeCount;
		node.SecondChildIndex += NodeCount;

		*NodeBegin = node;
		NodeBegin++;
	}

	mVertices.UnmapMemory();
	mFaces.UnmapMemory();
	mNodes.UnmapMemory();

	mVerticesLocal.Resize(mVertices.GetSize());
	mFacesLocal.Resize(mFaces.GetSize());
	mNodesLocal.Resize(mNodes.GetSize());

	vkEngine::CopyBuffer(mVerticesLocal, mVertices);
	vkEngine::CopyBuffer(mFacesLocal, mFaces);
	vkEngine::CopyBuffer(mNodesLocal, mNodes);

	mSceneData.MeshCount++;

	switch (type)
	{
		case RenderableType::eObject:
		{
			MeshInfo meshInfo{};
			meshInfo.BeginIndex = static_cast<uint32_t>(NodeCount);
			meshInfo.EndIndex = static_cast<uint32_t>(mNodes.GetSize());
			meshInfo.SurfaceMaterial = color;

			mMeshInfos << std::vector<MeshInfo>({ meshInfo });
			break;
		}
		case RenderableType::eLightSrc:
		{
			LightInfo lightInfo{};
			lightInfo.BeginIndex = static_cast<uint32_t>(NodeCount);
			lightInfo.EndIndex = static_cast<uint32_t>(mNodes.GetSize());
			lightInfo.Props.Color = color.Albedo;

			mLightInfos << std::vector<LightInfo>({ lightInfo });
			break;
		}
		default:
			break;
	}
}

vkEngine::Image ComputePipelineSettings::LoadImage(const std::string& filepath,
	vkEngine::MemoryResourceManager manager) const
{
	int width, height, bpp;
	uint8_t* buffer = stbi_load(filepath.c_str(), &width, &height, &bpp, 4);

	if (!buffer)
		return {};

	vkEngine::BufferCreateInfo createInfo{};
	createInfo.Size = width * height * 4;
	createInfo.Usage = vk::BufferUsageFlagBits::eTransferSrc;

	vkEngine::Buffer<uint8_t> imageBuffer;
	imageBuffer = manager.CreateBuffer<uint8_t>(createInfo);

	imageBuffer.SetBuf(buffer, buffer + createInfo.Size, 0);

	vkEngine::ImageCreateInfo imageInfo{};
	imageInfo.Type = vk::ImageType::e2D;
	imageInfo.Extent = vk::Extent3D(width, height, 1);
	imageInfo.Format = vk::Format::eR8G8B8A8Unorm;
	imageInfo.Usage = vk::ImageUsageFlagBits::eSampled;

	vkEngine::Image image = manager.CreateImage(imageInfo);

	image.TransitionLayout(vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTopOfPipe);
	image.CopyBufferData(imageBuffer);
	image.TransitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eComputeShader);

	stbi_image_free(buffer);

	return image;
}

PH_END

#include "ComputePipeline.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

PH_BEGIN

EstimatorComputePipeline::EstimatorComputePipeline(
	const vkEngine::PShader& shader, vkEngine::ResourcePool manager)
{
	this->SetShader(shader);

	vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eUniformBuffer;
	vk::MemoryPropertyFlags memProps = vk::MemoryPropertyFlagBits::eHostCoherent;

	mCamera = manager.CreateBuffer<Camera>(usage, memProps);

	usage = vk::BufferUsageFlagBits::eStorageBuffer;

	mGeomBuffersShared = CreateGeometryBuffers(vk::MemoryPropertyFlagBits::eHostCoherent, manager);
	mGeomBuffersLocal = CreateGeometryBuffers(vk::MemoryPropertyFlagBits::eDeviceLocal, manager);

	memProps = vk::MemoryPropertyFlagBits::eHostCoherent;
	usage = vk::BufferUsageFlagBits::eStorageBuffer;

	mMaterials = manager.CreateBuffer<Material>(usage, memProps);
	mLightProps = manager.CreateBuffer<LightProperties>(usage, memProps);

	mMeshInfos = manager.CreateBuffer<MeshInfo>(usage, memProps);
	mLightInfos = manager.CreateBuffer<LightInfo>(usage, memProps);

	usage = vk::BufferUsageFlagBits::eUniformBuffer;

	mSceneInfo = manager.CreateBuffer<SceneInfo>(usage, memProps);

	mSkybox = LoadImage("C:\\Users\\Navjot Singh\\Desktop\\CubeMaps\\Eq7.png", manager);

	// Set 2...

	memProps = vk::MemoryPropertyFlagBits::eHostCoherent;
	usage = vk::BufferUsageFlagBits::eStorageBuffer;

	mCPU_Feedback = manager.CreateBuffer<uint32_t>(usage, memProps);

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

void EstimatorComputePipeline::UpdateDescriptors()
{
	vkEngine::DescriptorWriter& writer = this->GetDescriptorWriter();

	mSceneData.RandomSeed = rand();
	mSceneData.LightCount = static_cast<uint32_t>(mLightInfos.GetSize());
	mSceneData.MeshCount = static_cast<uint32_t>(mMeshInfos.GetSize());

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
	verticesInfo.Buffer = mGeomBuffersLocal.Vertices.GetNativeHandles().Handle;

	writer.Update({ 1, 1, 0 }, verticesInfo);

	// Normals
	vkEngine::StorageBufferWriteInfo normalsInfo{};
	normalsInfo.Buffer = mGeomBuffersLocal.Normals.GetNativeHandles().Handle;

	writer.Update({ 1, 2, 0 }, normalsInfo);
	//
	//// TexCoords
	//vkEngine::StorageBufferWriteInfo texCoordsInfo{};
	//texCoordsInfo.Buffer = mGeomBuffersLocal.TexCoords.GetNativeHandles().Handle;
	//
	//writer.Update({ 1, 3, 0 }, texCoordsInfo);

	// Indices...
	vkEngine::StorageBufferWriteInfo indicesInfo{};
	indicesInfo.Buffer = mGeomBuffersLocal.Faces.GetNativeHandles().Handle;

	writer.Update({ 1, 4, 0 }, indicesInfo);

	// BVH Nodes
	vkEngine::StorageBufferWriteInfo nodesInfo{};
	nodesInfo.Buffer = mGeomBuffersLocal.Nodes.GetNativeHandles().Handle;

	writer.Update({ 1, 5, 0 }, nodesInfo);

	// Material buffer
	vkEngine::StorageBufferWriteInfo materialsInfo{};
	materialsInfo.Buffer = mMaterials.GetNativeHandles().Handle;

	writer.Update({ 1, 6, 0 }, materialsInfo);

	// Light properties buffer
	vkEngine::StorageBufferWriteInfo lightsInfo{};
	lightsInfo.Buffer = mLightProps.GetNativeHandles().Handle;

	writer.Update({ 1, 7, 0 }, lightsInfo);

	// MeshInfos...
	vkEngine::StorageBufferWriteInfo meshInfos{};
	meshInfos.Buffer = mMeshInfos.GetNativeHandles().Handle;

	writer.Update({ 1, 8, 0 }, meshInfos);

	// LightInfos...
	vkEngine::StorageBufferWriteInfo lightInfos{};
	lightInfos.Buffer = mLightInfos.GetNativeHandles().Handle;

	writer.Update({ 1, 9, 0 }, lightInfos);

	// Scene Info...
	vkEngine::UniformBufferWriteInfo sceneInfo{};
	sceneInfo.Buffer = mSceneInfo.GetNativeHandles().Handle;

	writer.Update({ 1, 10, 0 }, sceneInfo);

	// Skybox...
	vkEngine::CombinedImageSamplerWriteInfo samplerInfo{};
	samplerInfo.ImageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	samplerInfo.ImageView = mSkybox.GetIdentityImageView();
	samplerInfo.Sampler = *mSkyboxSampler;

	writer.Update({ 1, 11, 0 }, samplerInfo);
}

void EstimatorComputePipeline::UpdateCameraPosition(float deltaTime, float speed, CameraMovementFlags movement)
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

void EstimatorComputePipeline::UpdateCameraOrientation(float deltaTime, float speed,
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

void EstimatorComputePipeline::SubmitRenderable(const AquaFlow::MeshData& meshData,
	const Material& color, RenderableType type, uint32_t bvhDepth)
{
	AquaFlow::PhFlux::BVHFactory bvhFactory;
	bvhFactory.SetDepth(bvhDepth);

	AquaFlow::PhFlux::BVH bvhStruct = bvhFactory.Build(
		meshData.aPositions.begin(), meshData.aPositions.end(), 
		meshData.aFaces.begin(), meshData.aFaces.end());

	size_t VertexCount = mGeomBuffersLocal.Vertices.GetSize();
	size_t FaceCount = mGeomBuffersLocal.Faces.GetSize();
	size_t NodeCount = mGeomBuffersLocal.Nodes.GetSize();

	CopyVertexAttrib(mGeomBuffersShared.Vertices, mGeomBuffersLocal.Vertices,
		bvhStruct.Vertices.begin(), bvhStruct.Vertices.end(), 
		[](glm::vec4* BeginDevice, glm::vec4* EndDevice,
			glm::vec3* BeginHost, glm::vec3* EndHost)
	{
		while (BeginDevice != EndDevice)
		{
			*BeginDevice = glm::vec4(*BeginHost, 1.0f);

			BeginDevice++;
			BeginHost++;
		}
	});

	CopyVertexAttrib(mGeomBuffersShared.Normals, mGeomBuffersLocal.Normals,
		meshData.aNormals.begin(), meshData.aNormals.end(), 
		[](glm::vec4* BeginDevice, glm::vec4* EndDevice,
			const glm::vec3* BeginHost, const glm::vec3* EndHost)
	{
		while (BeginDevice != EndDevice)
		{
			*BeginDevice = glm::vec4(*BeginHost, 1.0f);

			BeginDevice++;
			BeginHost++;
		}
	});

	CopyVertexAttrib(mGeomBuffersShared.TexCoords, mGeomBuffersLocal.TexCoords,
		meshData.aTexCoords.begin(), meshData.aTexCoords.end(),
		[](glm::vec2* BeginDevice, glm::vec2* EndDevice,
			const glm::vec3* BeginHost, const glm::vec3* EndHost)
	{
		while (BeginDevice != EndDevice)
		{
			*BeginDevice = glm::vec2(BeginHost->x, BeginHost->y);

			BeginDevice++;
			BeginHost++;
		}
	});

	CopyVertexAttrib(mGeomBuffersShared.Faces, mGeomBuffersLocal.Faces,
		bvhStruct.Faces.begin(), bvhStruct.Faces.end(), 
		[VertexCount](glm::uvec4* BeginDevice, glm::uvec4* EndDevice,
			AquaFlow::Face* BeginHost, AquaFlow::Face* EndHost)
	{
		while (BeginDevice != EndDevice)
		{
			BeginDevice->x = BeginHost->Indices.x + static_cast<uint32_t>(VertexCount);
			BeginDevice->y = BeginHost->Indices.y + static_cast<uint32_t>(VertexCount);
			BeginDevice->z = BeginHost->Indices.z + static_cast<uint32_t>(VertexCount);

			BeginDevice++;
			BeginHost++;
		}
	});

	CopyVertexAttrib(mGeomBuffersShared.Nodes, mGeomBuffersLocal.Nodes,
		bvhStruct.Nodes.begin(), bvhStruct.Nodes.end(), 
		[FaceCount, NodeCount](Node* BeginDevice, Node* EndDevice,
			AquaFlow::PhFlux::Node* BeginHost, AquaFlow::PhFlux::Node* EndHost)
	{
		while (BeginDevice != EndDevice)
		{
			AquaFlow::PhFlux::Node& node = *BeginHost;

			node.BeginIndex += static_cast<uint32_t>(FaceCount);
			node.EndIndex += static_cast<uint32_t>(FaceCount);

			node.FirstChildIndex += static_cast<uint32_t>(NodeCount);
			node.SecondChildIndex += static_cast<uint32_t>(NodeCount);

			BeginDevice->BeginIndex = node.BeginIndex;
			BeginDevice->EndIndex = node.EndIndex;
			BeginDevice->FirstChildIndex = node.FirstChildIndex;
			BeginDevice->SecondChildIndex = node.SecondChildIndex;
			BeginDevice->MinBound = node.MinBound;
			BeginDevice->MaxBound = node.MaxBound;

			BeginDevice++;
			BeginHost++;
		}
	});

	mSceneData.MeshCount++;

	switch (type)
	{
		case RenderableType::eObject:
		{
			mMaterials << std::vector<Material>({ color });

			MeshInfo meshInfo{};
			meshInfo.BeginIndex = static_cast<uint32_t>(NodeCount);
			meshInfo.EndIndex = static_cast<uint32_t>(mGeomBuffersShared.Nodes.GetSize());
			meshInfo.MaterialIndex = static_cast<uint32_t>(mMaterials.GetSize() - 1);

			mMeshInfos << std::vector<MeshInfo>({ meshInfo });
			break;
		}
		case RenderableType::eLightSrc:
		{
			LightProperties props;
			props.Color = color.Albedo;

			mLightProps << std::vector<LightProperties>({ props });

			LightInfo lightInfo{};
			lightInfo.BeginIndex = static_cast<uint32_t>(NodeCount);
			lightInfo.EndIndex = static_cast<uint32_t>(mGeomBuffersShared.Nodes.GetSize());
			lightInfo.LightPropIndex = static_cast<uint32_t>(mLightProps.GetSize() - 1);

			mLightInfos << std::vector<LightInfo>({ lightInfo });
			break;
		}
		default:
			break;
	}
}

vkEngine::Image EstimatorComputePipeline::LoadImage(const std::string& filepath,
	vkEngine::ResourcePool manager) const
{
	int width, height, bpp;
	uint8_t* buffer = stbi_load(filepath.c_str(), &width, &height, &bpp, 4);

	if (!buffer)
		return {};

	vkEngine::Buffer<uint8_t> imageBuffer;
	imageBuffer = manager.CreateBuffer<uint8_t>(vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostCoherent);

	imageBuffer.Resize(width * height * 4);

	imageBuffer.SetBuf(buffer, buffer + imageBuffer.GetSize(), 0);

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

PH_FLUX_NAMESPACE::GeometryBuffers EstimatorComputePipeline::CreateGeometryBuffers(
	vk::MemoryPropertyFlags memProps, vkEngine::ResourcePool manager)
{
	GeometryBuffers buffers;

	buffers.Nodes = manager.CreateBuffer<Node>(vk::BufferUsageFlagBits::eStorageBuffer, memProps);
	buffers.Vertices = manager.CreateBuffer<glm::vec4>(vk::BufferUsageFlagBits::eStorageBuffer, memProps);
	buffers.Normals = manager.CreateBuffer<glm::vec4>(vk::BufferUsageFlagBits::eStorageBuffer, memProps);
	buffers.TexCoords = manager.CreateBuffer<glm::vec2>(vk::BufferUsageFlagBits::eStorageBuffer, memProps);
	buffers.Faces = manager.CreateBuffer<glm::uvec4>(vk::BufferUsageFlagBits::eStorageBuffer, memProps);

	return buffers;
}

PH_END

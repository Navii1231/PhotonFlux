#include "DeferredPipelinesTester.h"
#include "Geometry3D/MeshLoader.h"

#include "Utils/CompilerErrorChecker.h"

DeferredPipelinesTester::DeferredPipelinesTester(const ApplicationCreateInfo& createInfo)
	: Application(createInfo)
{
	CreatePipelines();
	CreateResources();
}

bool DeferredPipelinesTester::OnStart()
{
	AquaFlow::MeshLoader loader(aiPostProcessSteps::aiProcess_CalcTangentSpace);

	AquaFlow::RenderableBuilder builder(*mContext, 
		[](vkEngine::GenericBuffer idxBuf, const AquaFlow::RenderableInfo& info)
	{
		idxBuf.Resize(3 * info.Mesh.aFaces.size() * sizeof(uint32_t));

		auto* mem = idxBuf.MapMemory<uint32_t>(3 * info.Mesh.aFaces.size());

		for (size_t i = 0; i < info.Mesh.aFaces.size(); i++)
		{
			mem[0] = info.Mesh.aFaces[i].Indices[0];
			mem[1] = info.Mesh.aFaces[i].Indices[1];
			mem[2] = info.Mesh.aFaces[i].Indices[2];

			mem += 3;
		}

		idxBuf.UnmapMemory();
	});

	builder["position"] = [](vkEngine::GenericBuffer buf, const AquaFlow::RenderableInfo& info)
	{
		buf << info.Mesh.aPositions;
	};

	builder["normal"] = [](vkEngine::GenericBuffer buf, const AquaFlow::RenderableInfo& info)
	{
		buf << info.Mesh.aNormals;
	};

	builder["tangent"] = [](vkEngine::GenericBuffer buf, const AquaFlow::RenderableInfo& info)
	{
		buf << info.Mesh.aTangents;
	};

	builder["bitangent"] = [](vkEngine::GenericBuffer buf, const AquaFlow::RenderableInfo& info)
	{
		buf << info.Mesh.aBitangents;
	};

	builder["texcoords"] = [](vkEngine::GenericBuffer buf, const AquaFlow::RenderableInfo& info)
	{
		buf << info.Mesh.aTexCoords;
	};

	builder["materialids"] = [](vkEngine::GenericBuffer buf, const AquaFlow::RenderableInfo& info)
	{
		buf.Resize(info.Mesh.aPositions.size() * sizeof(glm::vec2));

		auto* mem = buf.MapMemory<glm::vec2>(info.Mesh.aPositions.size());

		for (size_t i = 0; i < info.Mesh.aPositions.size(); i++)
		{
			*mem = {0.0f, 0.0f};

			mem++;
		}

		buf.UnmapMemory();
	};

	std::string blenderModelGeom = "C:\\Users\\Navjot Singh\\Desktop\\Blender Models\\";

	AquaFlow::Geometry3D sphereGeometry = loader.LoadModel(blenderModelGeom + "Sphere.obj");
	AquaFlow::Geometry3D cubeGeom = loader.LoadModel(blenderModelGeom + "Cube.obj");
	AquaFlow::Geometry3D wineGeom = loader.LoadModel(blenderModelGeom + "WineGlass.obj");
	AquaFlow::Geometry3D cupGeom = loader.LoadModel(blenderModelGeom + "Cup.obj");

	AquaFlow::RenderableInfo renderableInfo{};
	renderableInfo.Mesh = sphereGeometry[0];
	renderableInfo.Usage |= vk::BufferUsageFlagBits::eStorageBuffer;

	auto sphere = builder.CreateRenderable(renderableInfo);

	renderableInfo.Mesh = cubeGeom[0];
	auto cube = builder.CreateRenderable(renderableInfo);

	renderableInfo.Mesh = wineGeom[0];
	auto wine = builder.CreateRenderable(renderableInfo);

	renderableInfo.Mesh = cupGeom[0];
	auto cup = builder.CreateRenderable(renderableInfo);

	vk::MemoryPropertyFlags memProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
	vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer;

	mDeferPipeline["@position_rgba32f"] = mMemoryManager.CreateGenericBuffer(usage, memProps);
	mDeferPipeline["@normal_rgba16f"] = mMemoryManager.CreateGenericBuffer(usage, memProps);
	mDeferPipeline["@tangent_rgba16f"] = mMemoryManager.CreateGenericBuffer(usage, memProps);
	mDeferPipeline["@bitangent_rgba16f"] = mMemoryManager.CreateGenericBuffer(usage, memProps);
	mDeferPipeline["@texcoords_rg8un"] = mMemoryManager.CreateGenericBuffer(usage, memProps);
	mDeferPipeline["@materialids_rg8un"] = mMemoryManager.CreateGenericBuffer(usage, memProps);

	usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer;

	mDeferPipeline.SetIndexBuffer(mMemoryManager.CreateGenericBuffer(usage, memProps));

	mDeferPipeline.SubmitRenderable(*sphere);

	mDeferPipeline.SetCamera(mEditorCamera.GetProjectionMatrix(), mEditorCamera.GetViewMatrix());

	mDeferPipeline.UpdateDescriptors();

	mShadingPipeline["$materials"].Buffer = mMaterials.GetBufferChunk();
	mShadingPipeline["$materials"].Location = { 1, 0, 0 };

	mShadingPipeline["$directional_lights"].Buffer = mDirLightSrcs.GetBufferChunk();
	mShadingPipeline["$directional_lights"].Location = { 1, 1, 0 };

	mShadingPipeline["#camera"].Buffer = mDeferPipeline.GetCamera().GetBufferChunk();
	mShadingPipeline["#camera"].Location = { 1, 2, 0 };

	mShadingPipeline.SetGeometry(mDeferPipeline.GetGeometry());

	mShadingPipeline.SetGeometryDescLocation(TAG_POSITION, { 0, 0, 0 });
	mShadingPipeline.SetGeometryDescLocation(TAG_NORMAL, { 0, 1, 0 });
	mShadingPipeline.SetGeometryDescLocation(TAG_TANGENT, { 0, 2, 0 });
	mShadingPipeline.SetGeometryDescLocation(TAG_BITANGENT, { 0, 3, 0 });
	mShadingPipeline.SetGeometryDescLocation(TAG_TEXTURE_COORDS, { 0, 4, 0 });
	mShadingPipeline.SetGeometryDescLocation(TAG_MATERIAL_IDS, { 0, 5, 0 });

	mShadingPipeline.UpdateDescriptors();

	return true;
}

bool DeferredPipelinesTester::OnUpdate(float ElaspedTime)
{
	auto geomBuffer = mDeferPipeline.GetFramebuffer();

	vk::CommandBuffer cmd = mCmdAlloc.Allocate();

	geomBuffer.TransitionColorAttachmentLayouts(
		vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eTopOfPipe);

	std::vector<vk::ClearValue> clearValues{ 7 };
	clearValues[0] = vk::ClearColorValue(0, 0, 0, 0);
	clearValues[1] = vk::ClearColorValue(0, 0, 0, 0);
	clearValues[2] = vk::ClearColorValue(0, 0, 0, 0);
	clearValues[3] = vk::ClearColorValue(0, 0, 0, 0);
	clearValues[4] = vk::ClearColorValue(0, 0, 0, 0);
	clearValues[5] = vk::ClearColorValue(0, 0, 0, 0);
	clearValues[6] = vk::ClearDepthStencilValue(0.0f, 0);

	cmd.reset();

	cmd.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

	mDeferPipeline.Begin(cmd, clearValues);

	mDeferPipeline.DrawIndexed(0, 0, 0, 1);

	mDeferPipeline.End();

	cmd.end();

	mExec.SubmitWork(cmd);
	mExec.WaitIdle();

	geomBuffer.TransitionColorAttachmentLayouts(
		vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eTopOfPipe);

	cmd.reset();

	cmd.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

	mShadingPipeline.Begin(cmd, clearValues);

	mShadingPipeline.DrawIndexed(0, 0, 0, 1);

	mShadingPipeline.End();

	cmd.end();

	mExec.SubmitWork(cmd);
	mExec.WaitIdle();

	mCmdAlloc.Free(cmd);

	return true;
}

void DeferredPipelinesTester::CreatePipelines()
{
	glm::uvec2 scrSize = mWindow->GetWindowSize();

	vkEngine::PShader deferShader, shadeShader, cpyShader;

	deferShader.SetFilepath("eVertex", mShaderDir + "DeferredPipelines\\Defer.vert");
	deferShader.SetFilepath("eFragment", mShaderDir + "DeferredPipelines\\Defer.frag");

	shadeShader.SetFilepath("eVertex", mShaderDir + "DeferredPipelines\\Shading.vert");
	shadeShader.SetFilepath("eFragment", mShaderDir + "DeferredPipelines\\Shading.frag");

	cpyShader.SetFilepath("eCompute", mShaderDir + "MovePipelines\\CopyIdx.comp");

	shadeShader.AddMacro("SHADING_TOLERENCE", "0.01");
	shadeShader.AddMacro("MATH_PI", "3.1415926");

	std::array<std::vector<vkEngine::ShaderInput>, 2> shaderInputs;

	auto deferErros = deferShader.CompileShaders();
	auto shadeErros = shadeShader.CompileShaders();
	auto cpyErros = cpyShader.CompileShaders();

	AquaFlow::CompileErrorChecker errorChecker("");

	errorChecker.AssertOnError(deferErros);
	errorChecker.AssertOnError(shadeErros);
	errorChecker.AssertOnError(cpyErros);

	AquaFlow::DeferredPipelineCreateInfo dCreateInfo(*mContext, deferShader, cpyShader, scrSize);

	vkEngine::PipelineBuilder builder = mContext->MakePipelineBuilder();

	mDeferPipeline = builder.BuildGraphicsPipeline<AquaFlow::DeferredPipeline>(dCreateInfo);
	mShadingPipeline = builder.BuildGraphicsPipeline<AquaFlow::ShadingPipeline>(*mContext, shadeShader, scrSize);
}

void DeferredPipelinesTester::CreateResources()
{
	mMemoryManager = mContext->CreateResourcePool();
	mCmdAlloc = mContext->CreateCommandPools()[0];

	mExec = mContext->FetchExecutor(0, vkEngine::QueueAccessType::eWorker);

	mMaterials = mMemoryManager.CreateGenericBuffer(vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostCoherent);
	mDirLightSrcs = mMemoryManager.CreateGenericBuffer(vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostCoherent);
}

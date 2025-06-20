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

	AquaFlow::Geometry3D sphereGeometry = loader.LoadModel("C:\\Users\\Navjot Singh\\Desktop\\Blender Models\\Sphere.obj");

	AquaFlow::RenderableBuilder builder(*mDevice, 
		[](vkEngine::Buffer<AquaFlow::GBufferVertexType>& buffer, const AquaFlow::RenderableInfo& info)->
		vkEngine::Buffer<AquaFlow::GBufferVertexType>
	{
		buffer.Resize(info.Mesh.aPositions.size());

		auto* mem = buffer.MapMemory(info.Mesh.aPositions.size());

		for (size_t i = 0; i < info.Mesh.aPositions.size(); i++)
		{
			mem->Position = glm::vec4(info.Mesh.aPositions[i], 1.0f);
			mem->Normals = glm::vec4(info.Mesh.aNormals[i], 1.0f);
			mem->Tangents = glm::vec4(info.Mesh.aTangents[i], 1.0f);
			mem->BiTangents = glm::vec4(info.Mesh.aBitangents[i], 1.0f);
			mem->TexCoords = glm::vec2(info.Mesh.aTexCoords[i].x, info.Mesh.aTexCoords[i].y);
			mem->MaterialIDs = glm::vec2(0.0f, 0);

			mem++;
		}

		buffer.UnmapMemory();

		return buffer;
	});

	AquaFlow::RenderableInfo renderableInfo{};
	renderableInfo.Mesh = sphereGeometry.GetMeshData()[0];

	auto sphere = builder.CreateRenderable(renderableInfo);

	return true;
}

bool DeferredPipelinesTester::OnUpdate(float ElaspedTime)
{
	return true;
}

void DeferredPipelinesTester::CreatePipelines()
{
	glm::uvec2 scrSize = mWindow->GetWindowSize();

	std::array<std::vector<vkEngine::ShaderInput>, 2> shaderInputs;

	shaderInputs[0].resize(2);
	shaderInputs[1].resize(2);

	vkEngine::ReadFile(mShaderDir + "DeferredPipelines\\GBuffer.vert", shaderInputs[0][0].SrcCode);
	vkEngine::ReadFile(mShaderDir + "DeferredPipelines\\GBuffer.frag", shaderInputs[0][1].SrcCode);
	vkEngine::ReadFile(mShaderDir + "DeferredPipelines\\Shading.vert", shaderInputs[1][0].SrcCode);
	vkEngine::ReadFile(mShaderDir + "DeferredPipelines\\Shading.frag", shaderInputs[1][1].SrcCode);

	shaderInputs[0][0].OptimizationFlag = vkEngine::OptimizerFlag::eNone;
	shaderInputs[0][1].OptimizationFlag = vkEngine::OptimizerFlag::eNone;
	shaderInputs[1][0].OptimizationFlag = vkEngine::OptimizerFlag::eNone;
	shaderInputs[1][1].OptimizationFlag = vkEngine::OptimizerFlag::eNone;

	shaderInputs[0][0].Stage = vk::ShaderStageFlagBits::eVertex;
	shaderInputs[0][1].Stage = vk::ShaderStageFlagBits::eFragment;
	shaderInputs[1][0].Stage = vk::ShaderStageFlagBits::eVertex;
	shaderInputs[1][1].Stage = vk::ShaderStageFlagBits::eFragment;

	AquaFlow::CompileErrorChecker errorChecker("");

	AquaFlow::GBufferPipelineContext gCtx(*mDevice, scrSize);
	auto errors = gCtx.CompileShaders(shaderInputs[0]);

	errorChecker.AssertOnError(errors);

	AquaFlow::ShadingPipelineCtx sCtx(*mDevice, scrSize);

	sCtx.AddMacro("SHADING_TOLERENCE", "0.01");
	sCtx.AddMacro("MATH_PI", "3.1415926");

	errors = sCtx.CompileShaders(shaderInputs[1]);

	errorChecker.AssertOnError(errors);

	sCtx.SetGeometry(gCtx.GetGeometry());

	vkEngine::PipelineBuilder builder = mDevice->MakePipelineBuilder();

	mGBufferPipeline = builder.BuildGraphicsPipeline(gCtx);
	mShadingPipeline = builder.BuildGraphicsPipeline(sCtx);
}

void DeferredPipelinesTester::CreateResources()
{

}

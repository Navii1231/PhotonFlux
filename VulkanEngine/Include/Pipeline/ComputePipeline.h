#pragma once
#include "PipelineConfig.h"
#include "PShader.h"
#include "BasicPipeline.h"

#include "../Core/Logger.h"

VK_BEGIN

struct ComputePipelineHandles : public PipelineHandles, public PipelineInfo
{
	glm::uvec3 WorkGroupSize = { 0, 0, 0 }; // By default, invalid workgroup
};

template <typename BasePipeline>
class BasicComputePipeline : public BasePipeline
{
public:
	BasicComputePipeline() = default;
	virtual ~BasicComputePipeline() = default;

	virtual void Begin(vk::CommandBuffer commandBuffer);

	// Binds the pipeline to a compute bind point
	// In contrast to graphics pipeline, compute pipeline might be dispatched
	// multiple times (asynchronously) within a single command buffer
	// This function is separately provided to support that functionality
	void BindPipeline();

	template <typename T>
	void SetShaderConstant(const std::string& name, const T& constant);

	// Async Dispatch...
	void Dispatch(const glm::uvec3& workGroups);

	virtual void End();

	virtual const PShader& GetShader() const override { return mShader; }

	glm::uvec3 GetWorkGroupSize() const { return mHandles->WorkGroupSize; }

	explicit operator bool() const { return static_cast<bool>(mHandles); }

private:
	Core::Ref<ComputePipelineHandles> mHandles;
	vkEngine::PShader mShader;

protected:
	void SetShader(const PShader& shader) { mShader = shader; }

	friend class PipelineBuilder;
};

using ComputePipeline = BasicComputePipeline<BasicPipeline>;

template<typename BasePipeline>
inline void BasicComputePipeline<BasePipeline>::Begin(vk::CommandBuffer commandBuffer)
{
	this->BeginPipeline(commandBuffer);
}

template<typename BasePipeline>
inline void BasicComputePipeline<BasePipeline>::BindPipeline()
{
	vk::CommandBuffer commandBuffer = ((BasePipeline*) this)->GetCommandBuffer();
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, mHandles->Handle);
}

template<typename BasePipeline>
template<typename T>
inline void BasicComputePipeline<BasePipeline>::SetShaderConstant(
	const std::string& name, const T& constant)
{
	_VK_ASSERT(this->GetPipelineState() == PipelineState::eRecording,
		"Pipeline must be in the recording state to set a shader constant (i.e within a Begin and End scope)!");

	vk::CommandBuffer commandBuffer = this->GetCommandBuffer();

	const PushConstantSubrangeInfos& subranges = GetShader().GetPushConstantSubranges();

	_VK_ASSERT(subranges.find(name) != subranges.end(),
		"Failed to find the push constant field \"" << name << "\" in the shader source code\n"
		"Note: If you turned on shader optimizations (vkEngine::OptimizerFlag::eO3) "
		"or not using the field in the shader, it won't appear in the reflections"
	);

	const vk::PushConstantRange range = subranges.at(name);

	_VK_ASSERT(range.size == sizeof(constant),
		"Input field size of the push constant does not match with the expected size!\n"
		"Possible causes might be:\n"
		"* Alignment mismatch between GPU and CPU structs\n"
		"* Data type mismatch between shader and C++ declarations\n"
		"* The constant has been optimized away in the shader\n"
	);

	commandBuffer.pushConstants(mHandles->LayoutData.Layout, range.stageFlags,
		range.offset, range.size, reinterpret_cast<const void*>(&constant));
}

template<typename BasePipeline>
inline void BasicComputePipeline<BasePipeline>::Dispatch(const glm::uvec3& WorkGroups)
{
	vk::CommandBuffer commandBuffer = ((BasePipeline*) this)->GetCommandBuffer();

	if (!mHandles->SetCache.empty())
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
			mHandles->LayoutData.Layout, 0, mHandles->SetCache, nullptr);

	commandBuffer.dispatch(WorkGroups.x, WorkGroups.y, WorkGroups.z);
}

template<typename BasePipeline>
inline void BasicComputePipeline<BasePipeline>::End()
{
	this->EndPipeline();
}

VK_END

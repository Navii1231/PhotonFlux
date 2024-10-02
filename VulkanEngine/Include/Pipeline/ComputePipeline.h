#pragma once
#include "PipelineConfig.h"
#include "ComputeContext.h"
#include "BasicPipeline.h"

#include "../Core/Logger.h"

VK_BEGIN

template <typename PipelineContextType, typename BasePipeline = BasicPipeline<PipelineContextType>>
class ComputePipeline : public BasePipeline
{
public:
	using MyContext = typename BasePipeline::MyContext;

public:
	ComputePipeline() = default;

	void Begin(vk::CommandBuffer commandBuffer);

	// Binds the pipeline to a compute bind point
	// In contrast to graphics pipeline, compute pipeline might be dispatched
	// multiple times (asynchronously) within a single command buffer
	// This function is separately provided to support that functionality
	void BindPipeline();

	template <typename T>
	void SetShaderConstant(const std::string& name, const T& constant);
	
	// Async Dispatch...
	void Dispatch(const glm::uvec3& workGroups);

	void End();

private:
	Core::Ref<ComputePipelineHandles> mPipelineHandles;

	friend class PipelineBuilder;
};

template<typename PipelineContextType, typename BasePipeline>
inline void ComputePipeline<PipelineContextType, BasePipeline>::Begin(vk::CommandBuffer commandBuffer)
{
	this->BeginPipeline(commandBuffer);
}

template<typename PipelineContextType, typename BasePipeline>
inline void ComputePipeline<PipelineContextType, BasePipeline>::BindPipeline()
{
	vk::CommandBuffer commandBuffer = ((BasePipeline*) this)->GetCommandBuffer();
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, mPipelineHandles->Handle);
}

template<typename PipelineContextType, typename BasePipeline>
template<typename T>
inline void ComputePipeline<PipelineContextType, BasePipeline>::SetShaderConstant(
	const std::string& name, const T& constant)
{
	_VK_ASSERT(this->GetPipelineState() == PipelineState::eRecording,
		"Pipeline must be in the recording state to set a shader constant (i.e within a Begin and End scope)!");

	vk::CommandBuffer commandBuffer = this->GetCommandBuffer();

	MyContext& Context = this->GetPipelineContext();

	const PushConstantSubrangeInfos& subranges = Context.GetPushConstantSubranges();

	_VK_ASSERT(subranges.find(name) != subranges.end(),
		"Failed to find the push constant field \"" << name << "\" in the shader source code\n"
		"Note: If you have turned on shader optimizations (vkEngine::OptimizerFlag::eO3) "
		"and not using the field in the shader, then the field will not appear in the reflections"
	);

	const vk::PushConstantRange range = subranges.at(name);

	_VK_ASSERT(range.size == sizeof(constant),
		"Input field size of the push constant does not match with the expected size!\n"
		"Possible causes might be:\n"
		"* Alignment mismatch between GPU and CPU structs\n"
		"* Data type mismatch between shader and C++ declarations\n"
		"* The constant has been optimized away in the shader\n"
	);

	commandBuffer.pushConstants(mPipelineHandles->LayoutData.Layout, range.stageFlags,
		range.offset, range.size, reinterpret_cast<const void*>(&constant));
}

template<typename PipelineContextType, typename BasePipeline>
inline void ComputePipeline<PipelineContextType, BasePipeline>::Dispatch(const glm::uvec3& WorkGroups)
{
	vk::CommandBuffer commandBuffer = ((BasePipeline*) this)->GetCommandBuffer();

	// TODO: might be a good idea to bind the pipeline every time before dispatching...
	// Or the headache for binding the pipeline may be left on the client
	// Haven't decided yet
	//commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, mPipelineHandles->Handle);

	if (!mPipelineHandles->SetCache.empty())
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
			mPipelineHandles->LayoutData.Layout, 0, mPipelineHandles->SetCache, nullptr);

	commandBuffer.dispatch(WorkGroups.x, WorkGroups.y, WorkGroups.z);
}

template<typename PipelineContextType, typename BasePipeline>
inline void ComputePipeline<PipelineContextType, BasePipeline>::End()
{
	this->EndPipeline();
}

VK_END

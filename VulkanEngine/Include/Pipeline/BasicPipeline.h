#pragma once
#include "PipelineConfig.h"
#include "../Descriptors/DescriptorWriter.h"
#include "PShader.h"

#include "../Core/Logger.h"

VK_BEGIN

enum class PipelineState
{
	eNull            = 0,
	eRecording       = 1,
};

// NOTE: The context must be a copyable or movable entity
struct BasicPipelineSpec
{
	// Writing API to upload descriptors
	DescriptorWriter DescWriter;

	// Command buffer
	vk::CommandBuffer PipelineCommands{};

	// Pipeline state
	std::atomic<PipelineState> State{ PipelineState::eNull };

	BasicPipelineSpec(DescriptorWriter&& descWriter)
		: DescWriter(std::move(descWriter)) {}
};

// Maybe updated and added more stuff later...
// TODO: no support for the push constants yet...
class BasicPipeline
{
public:
	virtual ~BasicPipeline() = default;

	// Provides an interface to update the vulkan descriptor sets
	virtual void UpdateDescriptors() = 0;

	void InsertExecutionBarrier(
		vk::PipelineStageFlags srcStage,
		vk::PipelineStageFlags dstStage,
		vk::DependencyFlags dependencyFlags = vk::DependencyFlagBits());

	void InsertMemoryBarrier(
		vk::PipelineStageFlags srcStage,
		vk::PipelineStageFlags dstStage,
		vk::AccessFlags srcAccessMasks,
		vk::AccessFlags dstAccessMasks,
		vk::DependencyFlags dependencyFlags = vk::DependencyFlagBits());

	virtual const PShader& GetShader() const = 0;

	vkEngine::DescriptorWriter& GetDescriptorWriter() { return mPipelineSpecs->DescWriter; }

	vk::CommandBuffer GetCommandBuffer() const { return mPipelineSpecs->PipelineCommands; }
	PipelineState GetPipelineState() const { return mPipelineSpecs->State.load(); }

protected:
	// Context given by the user
	// TODO: Don't have to put the command buffer in the shared_ptr
	// We can take this command buffer outside, this will allow multi-threading
	std::shared_ptr<BasicPipelineSpec> mPipelineSpecs;

	// Only PipelineBuilder is allowed to create an instance of this class
	friend class PipelineBuilder;

	BasicPipeline() = default;

	// NOTE: Calling BeginDefault MUST be accompanied by EndDefault at some point
	void BeginPipeline(vk::CommandBuffer commandBuffer);
	void EndPipeline();
};

inline void BasicPipeline::InsertExecutionBarrier(
	vk::PipelineStageFlags srcStage,
	vk::PipelineStageFlags dstStage,
	vk::DependencyFlags dependencyFlags /*= vk::DependencyFlagBits()*/)
{
	mPipelineSpecs->PipelineCommands.pipelineBarrier(srcStage, dstStage, dependencyFlags, {}, {}, {});
}

inline void BasicPipeline::InsertMemoryBarrier(vk::PipelineStageFlags srcStage, 
	vk::PipelineStageFlags dstStage, vk::AccessFlags srcAccessMasks, 
	vk::AccessFlags dstAccessMasks, vk::DependencyFlags dependencyFlags)
{
	vk::MemoryBarrier memoryBarrier{};

	memoryBarrier.setSrcAccessMask(srcAccessMasks);
	memoryBarrier.setDstAccessMask(dstAccessMasks);

	mPipelineSpecs->PipelineCommands.pipelineBarrier(srcStage, dstStage, 
		dependencyFlags, memoryBarrier, {}, {});
}

inline void BasicPipeline::BeginPipeline(vk::CommandBuffer commandBuffer)
{
	_VK_ASSERT(mPipelineSpecs->State.load() == PipelineState::eNull, 
		"Can't begin a pipeline scope as pipeline is already in recording state!");

	mPipelineSpecs->PipelineCommands = commandBuffer;
	mPipelineSpecs->State.store(PipelineState::eRecording);
}

inline void BasicPipeline::EndPipeline()
{
	_VK_ASSERT(mPipelineSpecs->State.load() == PipelineState::eRecording, 
		"Can't end pipeline scope as no command buffer has been recorded!");

	mPipelineSpecs->PipelineCommands = nullptr;
	mPipelineSpecs->State.store(PipelineState::eNull);
}

VK_END


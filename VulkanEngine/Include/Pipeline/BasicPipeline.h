#pragma once
#include "PipelineConfig.h"
#include "../Descriptors/DescriptorWriter.h"

#include "../Core/Logger.h"

#include <atomic>
#include <deque>

VK_BEGIN

enum class PipelineState
{
	eNull            = 0,
	eRecording       = 1,
};

// NOTE: The context must be a copyable or movable entity
template <typename PipelineContextType>
struct BasicPipelineSpec
{
	// User pipeline contect
	PipelineContextType Context;

	// Writing API to upload descriptors
	DescriptorWriter DescWriter;

	// Command buffer
	vk::CommandBuffer PipelineCommands{};

	// Pipeline state
	std::atomic<PipelineState> State{ PipelineState::eNull };

	BasicPipelineSpec(const PipelineContextType& context, DescriptorWriter&& descWriter)
		: Context(context), DescWriter(std::move(descWriter)) {}

	BasicPipelineSpec(PipelineContextType&& context, DescriptorWriter&& descWriter)
		: Context(std::forward(context)), DescWriter(std::move(descWriter)) {}
};

// Maybe updated and added more stuff later...
// TODO: no support for the push constants yet...
template <typename PipelineContextType>
class BasicPipeline
{
public:
	using MyContext = PipelineContextType;

public:
	virtual ~BasicPipeline() = default;

	// Provides an interface to update the vulkan descriptor sets
	void UpdateDescriptors() { mPipelineSpecs->Context.UpdateDescriptors(mPipelineSpecs->DescWriter); }

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

	MyContext& GetPipelineContext() { return mPipelineSpecs->Context; }
	const MyContext& GetPipelineContext() const { return mPipelineSpecs->Context; }

	vk::CommandBuffer GetCommandBuffer() const { return mPipelineSpecs->PipelineCommands; }
	PipelineState GetPipelineState() const { return mPipelineSpecs->State.load(); }

protected:
	// Context given by the user
	std::shared_ptr<BasicPipelineSpec<MyContext>> mPipelineSpecs;

	// Only PipelineBuilder is allowed to create an instance of this class
	friend class PipelineBuilder;

	BasicPipeline() = default;

	// NOTE: Calling BeginDefault MUST be accompanied by EndDefault at some point
	void BeginPipeline(vk::CommandBuffer commandBuffer);
	void EndPipeline();
};

template<typename PipelineContextType>
inline void BasicPipeline<PipelineContextType>::InsertExecutionBarrier(
	vk::PipelineStageFlags srcStage,
	vk::PipelineStageFlags dstStage,
	vk::DependencyFlags dependencyFlags /*= vk::DependencyFlagBits()*/)
{
	mPipelineSpecs->PipelineCommands.pipelineBarrier(srcStage, dstStage, dependencyFlags, {}, {}, {});
}

template<typename PipelineContextType>
inline void BasicPipeline<PipelineContextType>::InsertMemoryBarrier(vk::PipelineStageFlags srcStage, 
	vk::PipelineStageFlags dstStage, vk::AccessFlags srcAccessMasks, 
	vk::AccessFlags dstAccessMasks, vk::DependencyFlags dependencyFlags)
{
	vk::MemoryBarrier memoryBarrier{};

	memoryBarrier.setSrcAccessMask(srcAccessMasks);
	memoryBarrier.setDstAccessMask(dstAccessMasks);

	mPipelineSpecs->PipelineCommands.pipelineBarrier(srcStage, dstStage, 
		dependencyFlags, memoryBarrier, {}, {});
}

template<typename PipelineContextType>
inline void BasicPipeline<PipelineContextType>::BeginPipeline(vk::CommandBuffer commandBuffer)
{
	_VK_ASSERT(mPipelineSpecs->State.load() == PipelineState::eNull, 
		"Can't begin a pipeline scope as pipeline is already in recording state!");

	mPipelineSpecs->PipelineCommands = commandBuffer;
	mPipelineSpecs->State.store(PipelineState::eRecording);
}

template<typename PipelineContextType>
inline void BasicPipeline<PipelineContextType>::EndPipeline()
{
	_VK_ASSERT(mPipelineSpecs->State.load() == PipelineState::eRecording, 
		"Can't end pipeline scope as no command buffer has been recorded!");

	mPipelineSpecs->PipelineCommands = nullptr;
	mPipelineSpecs->State.store(PipelineState::eNull);
}

VK_END


#pragma once
#include "PipelineConfig.h"
#include "ComputeSettings.h"

VK_BEGIN

template <typename PipelineSettingsType>
class ComputePipeline
{
public:
	using PipelineSettings = PipelineSettingsType;

public:
	ComputePipeline() = default;
	
	void Dispatch(vk::CommandBuffer commandBuffer, const glm::uvec3& WorkGroups);

	PipelineSettings& GetConfigSettings() { return *mPipelineSettings; }
	const PipelineSettings& GetConfigSettings() const { return *mPipelineSettings; }

private:
	Core::Ref<ComputePipelineHandles> mPipelineHandles;
	std::shared_ptr<PipelineSettings> mPipelineSettings;

	DescriptorWriter mDescWriter;

	friend class PipelineBuilder;
};

template<typename PipelineSettingsType>
inline void ComputePipeline<PipelineSettingsType>::Dispatch(
	vk::CommandBuffer commandBuffer, const glm::uvec3& WorkGroups)
{
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, mPipelineHandles->Handle);

	mPipelineSettings->UpdateDescriptors(mDescWriter);

	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
		mPipelineHandles->LayoutData.Layout, 0, mPipelineHandles->SetCache, nullptr);

	commandBuffer.dispatch(WorkGroups.x, WorkGroups.y, WorkGroups.z);
}

VK_END

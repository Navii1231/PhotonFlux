#pragma once
#include "PipelineSettingsBase.h"

VK_BEGIN

class ComputePipelineSettingsBase : public PipelineSettingsBase
{
public:
	ComputePipelineSettingsBase() 
		: PipelineSettingsBase(PipelineType::eCompute) {}

	virtual void UpdateDescriptors(DescriptorWriter& writer) = 0;

	glm::uvec3 GetWorkGroupSize() const { return mWorkGroupSize; }
};

VK_END

#pragma once
#include "PipelineContext.h"

VK_BEGIN

class ComputePipelineContext : public PipelineContext
{
public:
	ComputePipelineContext()
		: PipelineContext(PipelineType::eCompute) {}

	virtual void UpdateDescriptors(DescriptorWriter& writer) = 0;

	glm::uvec3 GetWorkGroupSize() const { return mWorkGroupSize; }
};

VK_END

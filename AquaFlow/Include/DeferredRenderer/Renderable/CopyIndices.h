#pragma once
#include "DeferredRenderable.h"

AQUA_BEGIN

struct CopyIdxPipeline : public vkEngine::ComputePipeline
{
	CopyIdxPipeline() = default;

	CopyIdxPipeline(vkEngine::PShader copyComp)
	{ this->SetShader(copyComp); }

	inline virtual void UpdateDescriptors() override;

	vkEngine::GenericBuffer mSrcIdx;
	vkEngine::GenericBuffer mIdxBuf;
};

void CopyIdxPipeline::UpdateDescriptors()
{
	vkEngine::DescriptorWriter& writer = this->GetDescriptorWriter();

	vkEngine::StorageBufferWriteInfo idx{};
	idx.Buffer = mSrcIdx.GetNativeHandles().Handle;

	writer.Update({ 0, 0, 0 }, idx);

	idx.Buffer = mIdxBuf.GetNativeHandles().Handle;

	writer.Update({ 0, 1, 0 }, idx);
}

AQUA_END

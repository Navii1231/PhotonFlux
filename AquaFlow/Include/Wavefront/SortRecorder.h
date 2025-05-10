#pragma once
#include "LocalRadixSortPipeline.h"
#include "MergeSorterPipeline.h"

AQUA_BEGIN
PH_BEGIN

// Temporary for debugging...
#define _DEBUG_RAY_SORTER 0

template <typename CompType>
class SortRecorder
{
public:
	using PassContext = MergeSorterPassContext<CompType>;
	using SorterPipeline = MergeSorterPass<CompType>;
	using ArrayRef = typename PassContext::ArrayRef;

public:
	SortRecorder(vkEngine::PipelineBuilder builder, vkEngine::MemoryResourceManager manager);

	SortRecorder(const SortRecorder& Other);
	SortRecorder& operator =(const SortRecorder& Other);

	void CompileSorterPipeline(uint32_t workGroupSize);

	void SetBuffer(const vkEngine::Buffer<ArrayRef>& buffer);
	uint32_t Run(vk::CommandBuffer commandBuffer);

	void CopyOutput(vkEngine::Buffer<ArrayRef> buffer, uint32_t bufferIndex);

	void ResizeBuffer(uint32_t NewSize);
	vkEngine::Buffer<ArrayRef> GetBuffer() const { return mBuffer; }

private:
	vkEngine::Buffer<ArrayRef> mBuffer;

	uint32_t mWorkGroupSize = 0;

	SorterPipeline mMergePass;

	vkEngine::PipelineBuilder mPipelineBuilder;
	vkEngine::MemoryResourceManager mMemoryManager;

private:
	void CreateBuffer();
};

template<typename CompType>
inline SortRecorder<CompType>::SortRecorder(vkEngine::PipelineBuilder builder, 
	vkEngine::MemoryResourceManager manager)
	: mPipelineBuilder(builder), mMemoryManager(manager)
{
	CreateBuffer();
}

template<typename CompType>
inline SortRecorder<CompType>::SortRecorder(const SortRecorder& Other)
	: mPipelineBuilder(Other.mPipelineBuilder), mMemoryManager(Other.mMemoryManager), 
	mWorkGroupSize(Other.mWorkGroupSize)
{
	CreateBuffer();
	ResizeBuffer(static_cast<uint32_t>(Other.mBuffer.GetSize()));

	if (Other.mMergePass)
	{
		PassContext context = Other.mMergePass.GetPipelineContext();
		mMergePass = mPipelineBuilder.BuildComputePipeline(context);

		mMergePass.GetPipelineContext().UploadBuffer(mBuffer);
		mMergePass.UpdateDescriptors();
	}
}

template<typename CompType>
inline SortRecorder<CompType>& SortRecorder<CompType>::operator=(const SortRecorder& Other)
{
	mWorkGroupSize = Other.mWorkGroupSize;

	CreateBuffer();
	mBuffer.Resize(Other.mBuffer.GetSize());

	if (Other.mMergePass)
	{
		PassContext context = Other.mMergePass.GetPipelineContext();
		mMergePass = mPipelineBuilder.BuildComputePipeline(context);

		mMergePass.GetPipelineContext().UploadBuffer(mBuffer);
		mMergePass.UpdateDescriptors();
	}

	return *this;
}

template<typename CompType>
inline void SortRecorder<CompType>::CompileSorterPipeline(uint32_t workGroupSize)
{
	mWorkGroupSize = workGroupSize;

	PassContext context;

	context.CompileShader(workGroupSize);
	mMergePass = mPipelineBuilder.BuildComputePipeline(context);

	mMergePass.GetPipelineContext().UploadBuffer(mBuffer);
	mMergePass.UpdateDescriptors();
}

template<typename CompType>
inline void SortRecorder<CompType>::SetBuffer(const vkEngine::Buffer<ArrayRef>& buffer)
{
	mBuffer.Resize(2 * buffer.GetSize());

	vkEngine::CopyBuffer(mBuffer, buffer);

	vk::BufferCopy copyBuffer{};

	copyBuffer.size = buffer.GetSize();
	copyBuffer.dstOffset = copyBuffer.size;

	//vkEngine::CopyBufferRegions(mBuffer, buffer, { copyBuffer });

	mMergePass.GetPipelineContext().UploadBuffer(mBuffer);
}

template<typename CompType>
inline uint32_t SortRecorder<CompType>::Run(vk::CommandBuffer commandBuffer)
{
	uint32_t Size = static_cast<uint32_t>(mBuffer.GetSize() / 2);

	mMergePass.Begin(commandBuffer);
	mMergePass.BindPipeline();

	/*   Push constant layout...
	* 
		layout(push_constant) uniform MetaData
		{
			uint pBufferSize;
			uint pActiveBuffer;
			uint pTreeDepth;
			uint pSectionLength;
		};
	*/

	mMergePass.SetShaderConstant("eCompute.MetaData.Index_0", (uint32_t) Size);

	uint32_t ActiveBuffer = 0;
	uint32_t TreeDepth = 1;

	for (uint32_t SequenceSize = 1; SequenceSize < Size; SequenceSize <<= 1)
	{
		mMergePass.InsertMemoryBarrier(vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eComputeShader,
			vk::AccessFlagBits::eShaderWrite,
			vk::AccessFlagBits::eShaderRead);

		mMergePass.SetShaderConstant("eCompute.MetaData.Index_1", (uint32_t) ActiveBuffer);
		mMergePass.SetShaderConstant("eCompute.MetaData.Index_2", (uint32_t) TreeDepth++);
		mMergePass.SetShaderConstant("eCompute.MetaData.Index_3", (uint32_t) SequenceSize);

		mMergePass.Dispatch({ Size / mWorkGroupSize + 1, 1, 1 });

		ActiveBuffer = 1 - ActiveBuffer;
	}

	mMergePass.End();

	return ActiveBuffer;
}

template<typename CompType>
inline void SortRecorder<CompType>::CopyOutput(vkEngine::Buffer<ArrayRef> buffer, uint32_t bufferIndex)
{
	vk::BufferCopy copyBuffer{};
	copyBuffer.srcOffset = bufferIndex * mBuffer.GetSize() / 2;
	copyBuffer.size = mBuffer.GetSize() / 2;

	vkEngine::CopyBufferRegions(buffer, mBuffer, { copyBuffer });
}

template<typename CompType>
inline void SortRecorder<CompType>::ResizeBuffer(uint32_t NewSize)
{
	mBuffer.Resize(NewSize);
	mMergePass.GetPipelineContext().UploadBuffer(mBuffer);

	mMergePass.UpdateDescriptors();
}

template<typename CompType>
inline void SortRecorder<CompType>::CreateBuffer()
{
	vkEngine::BufferCreateInfo bufferInfo{};
	bufferInfo.Usage = vk::BufferUsageFlagBits::eStorageBuffer;
	bufferInfo.MemProps = vk::MemoryPropertyFlagBits::eDeviceLocal;

#if _DEBUG_RAY_SORTER
	bufferInfo.MemProps = vk::MemoryPropertyFlagBits::eHostCoherent;
#endif

	mBuffer = mMemoryManager.CreateBuffer<ArrayRef>(bufferInfo);
}

PH_END
AQUA_END

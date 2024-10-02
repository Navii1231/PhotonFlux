#pragma once
#include "MemoryConfig.h"
#include "../Process/ProcessManager.h"
#include "../Process/RecordableResource.h"

#include <initializer_list>
#include <vector>
#include <array>

VK_BEGIN

template <typename T>
class Buffer : public RecordableResource
{
public:
	using Type = T;

public:
	Buffer() = default;

	// Warning: Iterators must form a contiguous region in memory
	template <typename Iter>
	void AppendBuf(Iter Begin, Iter End);

	// Warning: Iterators must form a contiguous region in memory
	template <typename Iter>
	void SetBuf(Iter Begin, Iter End, size_t Offset = 0);

	void FetchMemory(Type* Begin, Type* End, size_t Offset = 0);

	T* MapMemory(size_t Count, size_t Offset = 0) const;
	void UnmapMemory() const;

	void InsertMemoryBarrier(vk::CommandBuffer commandBuffer, const MemoryBarrierInfo& pipelineBarrierInfo);

	// TODO: Routine can optimized further
	void TransferOwnership(uint32_t DstQueueFamilyIndex) const;
	void TransferOwnership(vk::QueueFlagBits optimizedCaps) const;

	void Clear() { mChunk.BufferHandles->ElemCount = 0; }

	const Core::Buffer& GetNativeHandles() const { return *mChunk.BufferHandles; }
	Core::BufferConfig GetBufferConfig() const { return mChunk.BufferHandles->Config; }

	size_t GetSize() const { return mChunk.BufferHandles->ElemCount; }
	vk::DeviceSize GetDeviceSize() const { return mChunk.BufferHandles->ElemCount * sizeof(T); }
	size_t GetCapacity() const { return mChunk.BufferHandles->Config.ElemCount; }

	void Reserve(size_t NewCap);
	void Resize(size_t NewSize);

	explicit operator bool() const { return static_cast<bool>(mChunk.BufferHandles); }

private:
	Core::BufferChunk mChunk;

private:
	explicit Buffer(Core::BufferChunk&& InputChunk)
		: mChunk(std::move(InputChunk)) {}

	friend class MemoryResourceManager;
	
	template <typename V>
	friend void CopyBufferRegions(Buffer<V>& DstBuf, const Buffer<V>& SrcBuf,
		const std::vector<vk::BufferCopy>& CopyRegions);

private:
	// Helper Functions...
	void ScaleCapacity(size_t NewSize);
	void ScaleCapacityWithoutLoss(size_t NewSize);
	void PerformCopyTaskGPU(Core::Buffer& DstBuffer, const Core::Buffer& SrcBuffer, 
		const vk::ArrayProxy<vk::BufferCopy>& CopyRegions);

	// Helper methods for ownership transfer
	void ReleaseBuffer(uint32_t dstIndex, vk::Semaphore bufferReleased) const;
	void AcquireBuffer(uint32_t acquiringFamily, vk::Semaphore bufferReleased) const;

	void MakeHollow();
};

template <typename T>
void RecordCopyBufferRegions(vk::CommandBuffer buffer, Buffer<T>& DstBuf,
	const Buffer<T>& SrcBuf, const std::vector<vk::BufferCopy>&  CopyRegions)
{
	std::vector<vk::BufferCopy> CopyRegionsTemp = CopyRegions;

	for (auto& Region : CopyRegionsTemp)
	{
		Region.srcOffset *= sizeof(T);
		Region.dstOffset *= sizeof(T);
		Region.size *= sizeof(T);
	}

	buffer.copyBuffer(
		SrcBuf.GetNativeHandles().Handle,
		DstBuf.GetNativeHandles().Handle,
		CopyRegionsTemp);
}

template <typename T>
void RecordCopyBuffer(vk::CommandBuffer buffer, Buffer<T>& DstBuf, const Buffer<T>& SrcBuf)
{
	size_t MinSize = std::min(DstBuf.GetSize(), SrcBuf.GetSize());

	vk::BufferCopy CopyRegion;
	CopyRegion.setSrcOffset(0);
	CopyRegion.setDstOffset(0);
	CopyRegion.setSize(MinSize);

	RecordCopyBufferRegions<T>(buffer, DstBuf, SrcBuf, { CopyRegion });
}

template <typename T>
void CopyBufferRegions(Buffer<T>& DstBuf, const Buffer<T>& SrcBuf,
	const std::vector<vk::BufferCopy>& CopyRegions)
{
	std::vector<vk::BufferCopy> CopyRegionsTemp = CopyRegions;

	for (auto& Region : CopyRegionsTemp)
	{
		Region.srcOffset *= sizeof(T);
		Region.dstOffset *= sizeof(T);
		Region.size *= sizeof(T);
	}

	DstBuf.PerformCopyTaskGPU(*DstBuf.mChunk.BufferHandles, *SrcBuf.mChunk.BufferHandles,
		{ (uint32_t) CopyRegionsTemp.size(), CopyRegionsTemp.data() });
}

template <typename T>
void CopyBuffer(Buffer<T>& DstBuf, const Buffer<T>& SrcBuf)
{
	size_t MinSize = std::min(DstBuf.GetSize(), SrcBuf.GetSize());
	vk::BufferCopy CopyRegion;
	CopyRegion.setSrcOffset(0);
	CopyRegion.setDstOffset(0);
	CopyRegion.setSize(MinSize);

	CopyBufferRegions<T>(DstBuf, SrcBuf, std::vector<vk::BufferCopy>({ CopyRegion }) );
}

template<typename T>
void Buffer<T>::FetchMemory(Type* Begin, Type* End, size_t Offset)
{
	if (Begin == End)
		return;

	size_t Count = End - Begin;

	void* Memory = MapMemory(Count, Offset);
	std::memcpy(Begin, Memory, Count * sizeof(T));
	UnmapMemory();
}

template<typename T>
T* Buffer<T>::MapMemory(size_t Count, size_t Offset) const
{
	return (T*) mChunk.Device->mapMemory(mChunk.BufferHandles->Memory,
		Offset * sizeof(T), Count * sizeof(T));
}

template<typename T>
void Buffer<T>::UnmapMemory() const
{
	mChunk.Device->unmapMemory(mChunk.BufferHandles->Memory);

	if (mChunk.BufferHandles->Config.MemProps & vk::MemoryPropertyFlagBits::eHostCoherent)
		return;

	// Synchronize manually in case the underlying memory isn't HostCoherent
	vk::MappedMemoryRange range{};
	range.setMemory(mChunk.BufferHandles->Memory);
	range.setOffset(0);
	range.setSize(mChunk.BufferHandles->ElemCount);

	mChunk.Device->flushMappedMemoryRanges(range);
}

template<typename T>
void Buffer<T>::InsertMemoryBarrier(vk::CommandBuffer commandBuffer, const MemoryBarrierInfo& pipelineBarrierInfo)
{
	vk::BufferMemoryBarrier barrier;
	barrier.setBuffer(mChunk.BufferHandles->Handle);
	barrier.setSrcAccessMask(pipelineBarrierInfo.SrcAccessMasks);
	barrier.setDstAccessMask(pipelineBarrierInfo.DstAccessMasks);
	barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
	barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
	barrier.setSize(VK_WHOLE_SIZE);

	commandBuffer.pipelineBarrier(pipelineBarrierInfo.SrcPipeleinStages, pipelineBarrierInfo.DstPipelineStages,
		pipelineBarrierInfo.DependencyFlags, nullptr, barrier, nullptr);
}

template<typename T>
void Buffer<T>::TransferOwnership(uint32_t DstQueueFamilyIndex) const
{
	uint32_t& SrcOwner = mChunk.BufferHandles->Config.ResourceOwner;

	if (SrcOwner == DstQueueFamilyIndex)
		return;

	vk::Semaphore bufferReleased = mChunk.Device->createSemaphore({});

	ReleaseBuffer(DstQueueFamilyIndex, bufferReleased);
	AcquireBuffer(DstQueueFamilyIndex, bufferReleased);

	mChunk.Device->destroySemaphore(bufferReleased);

	SrcOwner = DstQueueFamilyIndex;
}

template<typename T>
void Buffer<T>::TransferOwnership(vk::QueueFlagBits optimizedCaps) const
{
	uint32_t DstQueueFamily = mProcessHandler
		.GetQueueManager().FindOptimalQueueFamilyIndex(optimizedCaps);
	TransferOwnership(DstQueueFamily);
}

template<typename T>
void Buffer<T>::Reserve(size_t NewCap)
{
	if (NewCap > mChunk.BufferHandles->Config.ElemCount)
		ScaleCapacityWithoutLoss(NewCap);
}

template<typename T>
void Buffer<T>::Resize(size_t NewSize)
{
	Reserve(NewSize);
	mChunk.BufferHandles->ElemCount = NewSize;
}

template<typename T>
void Buffer<T>::ScaleCapacity(size_t NewSize)
{
	mChunk.BufferHandles->Config.ElemCount = NewSize;
	Core::Buffer NewBuffer = Core::Utils::CreateBuffer(mChunk.BufferHandles->Config);

	auto Device = mChunk.Device;

	mChunk.BufferHandles.ReplaceValue(NewBuffer);
}

template<typename T>
void Buffer<T>::ScaleCapacityWithoutLoss(size_t NewSize)
{
	mChunk.BufferHandles->Config.ElemCount = NewSize;
	Core::Buffer NewBuffer = Core::Utils::CreateBuffer(mChunk.BufferHandles->Config);

	vk::BufferCopy CopyRegion{};
	CopyRegion.setSize(mChunk.BufferHandles->ElemCount * sizeof(T));

	PerformCopyTaskGPU(NewBuffer, *mChunk.BufferHandles, CopyRegion);

	auto Device = mChunk.Device;

	mChunk.BufferHandles.ReplaceValue(NewBuffer);
}

template<typename T>
template<typename Iter>
void Buffer<T>::AppendBuf(Iter Begin, Iter End)
{
	SetBuf(Begin, End, mChunk.BufferHandles->ElemCount);
}

template<typename T>
template<typename Iter>
void Buffer<T>::SetBuf(Iter Begin, Iter End, size_t Offset)
{
	size_t Count = End - Begin;
	Reserve(Count + Offset);

	void* Memory = MapMemory(Count, Offset);
	std::memcpy(Memory, &(*Begin), Count * sizeof(T));
	UnmapMemory();

	mChunk.BufferHandles->ElemCount = Count + Offset;
}

template <typename T>
Buffer<T>& operator <<(Buffer<T>& vkBuf, const std::vector<T>& cpuBuf)
{
	vkBuf.AppendBuf(cpuBuf.begin(), cpuBuf.end());
	return vkBuf;
}

template <typename T>
Buffer<T>& operator <<(Buffer<T>& vkBuf, std::initializer_list<T> cpuBuf)
{
	vkBuf.AppendBuf(cpuBuf.begin(), cpuBuf.end());
	return vkBuf;
}

template <typename T, size_t Size>
Buffer<T>& operator <<(Buffer<T>& vkBuf, const std::array<T, Size>& cpuBuf)
{
	vkBuf.AppendBuf(cpuBuf.begin(), cpuBuf.end());
	return vkBuf;
}

template <typename T>
Buffer<T>& operator >>(Buffer<T>& vkBuf, std::vector<T>& cpuBuf)
{
	size_t offset = cpuBuf.size();
	cpuBuf.resize(cpuBuf.size() + vkBuf.GetSize());

	vkBuf.FetchMemory(cpuBuf.begin()._Unwrapped() + offset, cpuBuf.end()._Unwrapped());
	return vkBuf;
}

template <typename T>
Buffer<T>& operator <<(Buffer<T>& vkBuf, const T& cpuVal)
{
	vkBuf.AppendBuf(&cpuVal, &cpuVal + 1);
	return vkBuf;
}

template <typename T>
void Buffer<T>::PerformCopyTaskGPU(Core::Buffer& DstBuffer, const Core::Buffer& SrcBuffer,
	const vk::ArrayProxy<vk::BufferCopy>& CopyRegions)
{
	if (CopyRegions.front().size == 0)
		return;

	uint32_t DstOwner = DstBuffer.Config.ResourceOwner;

	auto [CommandsAllocator, Executor] = mProcessHandler.FetchProcessHandler(DstOwner, QueueAccessType::eWorker);

	auto CmdBuffer = CommandsAllocator.BeginOneTimeCommands();

	CmdBuffer.copyBuffer(SrcBuffer.Handle, DstBuffer.Handle, CopyRegions);

	CommandsAllocator.EndOneTimeCommands(CmdBuffer, Executor);
}

template<typename T>
inline void Buffer<T>::ReleaseBuffer(uint32_t dstIndex, vk::Semaphore bufferReleased) const
{
	uint32_t OwnerIndex = mChunk.BufferHandles->Config.ResourceOwner;

	auto [CommandsAllocator, executor] = mProcessHandler.FetchProcessHandler(OwnerIndex, QueueAccessType::eWorker);

	auto CmdBuffer = CommandsAllocator.Allocate();

	Core::BufferOwnershipTransferInfo transferInfo{};

	transferInfo.Buffer = *mChunk.BufferHandles;
	transferInfo.CmdBuffer = CmdBuffer;
	transferInfo.SrcFamilyIndex = OwnerIndex;
	transferInfo.DstFamilyIndex = dstIndex;
	
	Core::Utils::FillBufferReleaseMasks(transferInfo,
		mProcessHandler.GetQueueManager().GetFamilyCapabilities(OwnerIndex));

	CmdBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

	Core::Utils::RecordBufferTransferBarrier(transferInfo);

	CmdBuffer.end();

	vk::SubmitInfo submitInfo{};
	submitInfo.setCommandBuffers(CmdBuffer);
	submitInfo.setSignalSemaphores(bufferReleased);

	auto queue = executor.SubmitWork(submitInfo);
	executor[queue]->WaitIdle();
	
	CommandsAllocator.Free(CmdBuffer);
}

template<typename T>
inline void Buffer<T>::AcquireBuffer(uint32_t acquiringFamily, vk::Semaphore bufferReleased) const
{
	uint32_t OwnerIndex = mChunk.BufferHandles->Config.ResourceOwner;
	uint32_t DstIndex = acquiringFamily;

	auto [CommandsAllocator, executor] =
		mProcessHandler.FetchProcessHandler(DstIndex, QueueAccessType::eWorker);

	auto CmdBuffer = CommandsAllocator.Allocate();

	Core::BufferOwnershipTransferInfo transferInfo{};

	transferInfo.Buffer = *mChunk.BufferHandles;
	transferInfo.CmdBuffer = CmdBuffer;
	transferInfo.SrcFamilyIndex = OwnerIndex;
	transferInfo.DstFamilyIndex = DstIndex;

	Core::Utils::FillBufferAcquireMasks(transferInfo,
		mProcessHandler.GetQueueManager().GetFamilyCapabilities(DstIndex));

	CmdBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

	Core::Utils::RecordBufferTransferBarrier(transferInfo);

	CmdBuffer.end();

	vk::PipelineStageFlags waitFlags[] = { vk::PipelineStageFlagBits::eTopOfPipe };

	vk::SubmitInfo submitInfo{};
	submitInfo.setCommandBuffers(CmdBuffer);
	submitInfo.setWaitSemaphores(bufferReleased);
	submitInfo.setWaitDstStageMask(waitFlags);

	auto queue = executor.SubmitWork(submitInfo);
	executor[queue]->WaitIdle();

	CommandsAllocator.Free(CmdBuffer);
}

template<typename T>
void Buffer<T>::MakeHollow()
{
	mChunk.BufferHandles.Reset();
	mChunk.BufferHandles->ElemCount = 0;
}


VK_END
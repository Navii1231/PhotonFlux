#pragma once
#include "MemoryConfig.h"
#include "../Process/RecordableResource.h"

VK_BEGIN

// TODO: Could be simplified and duplication could be avoided
template <typename T>
class Buffer : public RecordableResource
{
public:
	using Type = T;

public:
	Buffer() = default;

	template <typename Iter>
	void AppendBuf(Iter Begin, Iter End);

	template <typename Iter>
	void SetBuf(Iter Begin, Iter End, size_t Offset = 0);

	template <typename Iter>
	void FetchMemory(Iter Begin, Iter End, size_t Offset = 0);

	T* MapMemory(size_t Count, size_t Offset = 0) const;
	void UnmapMemory() const;

	void InsertMemoryBarrier(vk::CommandBuffer commandBuffer, const MemoryBarrierInfo& pipelineBarrierInfo);

	// TODO: Routine can be optimized further
	void TransferOwnership(uint32_t DstQueueFamilyIndex) const;
	void TransferOwnership(vk::QueueFlagBits optimizedCaps) const;

	void Clear() { mChunk.BufferHandles->ElemCount = 0; }

	Core::BufferChunk GetBufferChunk() const { return mChunk; }
	const Core::Buffer& GetNativeHandles() const { return *mChunk.BufferHandles; }
	Core::BufferConfig GetBufferConfig() const { return mChunk.BufferHandles->Config; }

	size_t GetSize() const { return mChunk.BufferHandles->ElemCount; }
	vk::DeviceSize GetDeviceSize() const { return mChunk.BufferHandles->ElemCount * sizeof(T); }
	size_t GetCapacity() const { return mChunk.BufferHandles->Config.ElemCount; }

	void Reserve(size_t NewCap);
	void Resize(size_t NewSize);
	void ShrinkToFit();

	explicit operator bool() const { return static_cast<bool>(mChunk.BufferHandles); }

private:
	Core::BufferChunk mChunk;

private:
	explicit Buffer(Core::BufferChunk&& InputChunk)
		: mChunk(std::move(InputChunk)) {}

	friend class ResourcePool;
	
	template <typename V>
	friend void CopyBufferRegions(Buffer<V>& DstBuf, const Buffer<V>& SrcBuf,
		const std::vector<vk::BufferCopy>& CopyRegions);

private:
	// Helper Functions...
	void ScaleCapacity(size_t NewSize);
	void ScaleCapacityWithoutLoss(size_t NewSize);
	void CopyGPU(Core::Buffer& DstBuffer, const Core::Buffer& SrcBuffer, 
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

	DstBuf.CopyGPU(*DstBuf.mChunk.BufferHandles, *SrcBuf.mChunk.BufferHandles,
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
template <typename Iter>
void Buffer<T>::FetchMemory(Iter Begin, Iter End, size_t Offset)
{
	std::span range(Begin, End - Begin);

	static_assert(std::ranges::contiguous_range<decltype(range)>, 
		"vkEngine::Buffer::SetBuf only accepts contiguous memory");

	if (Begin == End)
		return;

	size_t Count = End - Begin;

	T* Memory = MapMemory(Count, Offset);
	std::memcpy(&(*Begin), Memory, Count * sizeof(T));
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
	uint32_t DstQueueFamily = mQueueManager->FindOptimalQueueFamilyIndex(optimizedCaps);

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

template <typename T>
void VK_NAMESPACE::Buffer<T>::ShrinkToFit()
{
	if (mChunk.BufferHandles->ElemCount != mChunk.BufferHandles->Config.ElemCount)
		Resize(mChunk.BufferHandles->ElemCount);
}

template<typename T>
void Buffer<T>::ScaleCapacity(size_t NewSize)
{
	mChunk.BufferHandles->Config.ElemCount = NewSize;
	Core::Buffer NewBuffer = Core::Utils::CreateBuffer(mChunk.BufferHandles->Config);

	auto Device = mChunk.Context;

	mChunk.BufferHandles.SetValue(NewBuffer);
}

template<typename T>
void Buffer<T>::ScaleCapacityWithoutLoss(size_t NewSize)
{
	mChunk.BufferHandles->Config.ElemCount = NewSize;
	Core::Buffer NewBuffer = Core::Utils::CreateBuffer(mChunk.BufferHandles->Config);

	vk::BufferCopy CopyRegion{};
	CopyRegion.setSize(mChunk.BufferHandles->ElemCount * sizeof(T));

	CopyGPU(NewBuffer, *mChunk.BufferHandles, CopyRegion);

	auto Device = mChunk.Device;

	mChunk.BufferHandles.SetValue(NewBuffer);
}

template<typename T>
template<typename Iter>
void Buffer<T>::AppendBuf(Iter Begin, Iter End)
{
	std::span range(Begin, End - Begin);

	static_assert(std::ranges::contiguous_range<decltype(range)>,
		"vkEngine::Buffer::SetBuf only accepts contiguous memory");

	SetBuf<Iter>(Begin, End, mChunk.BufferHandles->ElemCount);
}

template<typename T>
template<typename Iter>
void Buffer<T>::SetBuf(Iter Begin, Iter End, size_t Offset)
{
	std::span range(Begin, End - Begin);

	static_assert(std::ranges::contiguous_range<decltype(range)>,
		"vkEngine::Buffer::SetBuf only accepts contiguous memory");

	size_t Count = End - Begin;

	Reserve(Count + Offset);

	void* Memory = MapMemory(Count, Offset);
	std::memcpy(Memory, &(*Begin), Count * sizeof(T));
	UnmapMemory();

	mChunk.BufferHandles->ElemCount = Count + Offset;
}

template <typename T>
void Buffer<T>::CopyGPU(Core::Buffer& DstBuffer, const Core::Buffer& SrcBuffer,
	const vk::ArrayProxy<vk::BufferCopy>& CopyRegions)
{
	if (CopyRegions.front().size == 0)
		return;

	uint32_t DstOwner = DstBuffer.Config.ResourceOwner;

	InvokeOneTimeProcess(DstOwner,[DstOwner, &SrcBuffer, &DstBuffer, &CopyRegions, this]
	(vk::CommandBuffer CmdBuffer)
		{
			CmdBuffer.copyBuffer(SrcBuffer.Handle, DstBuffer.Handle, CopyRegions);
		});
}

template<typename T>
inline void Buffer<T>::ReleaseBuffer(uint32_t dstIndex, vk::Semaphore bufferReleased) const
{
	uint32_t OwnerIndex = mChunk.BufferHandles->Config.ResourceOwner;

	InvokeProcess(OwnerIndex, [bufferReleased, OwnerIndex, dstIndex, this](
		vk::CommandBuffer CmdBuffer)->vk::SubmitInfo
		{
			Core::BufferOwnershipTransferInfo transferInfo{};

			transferInfo.Buffer = *mChunk.BufferHandles;
			transferInfo.CmdBuffer = CmdBuffer;
			transferInfo.SrcFamilyIndex = OwnerIndex;
			transferInfo.DstFamilyIndex = dstIndex;

			Core::Utils::FillBufferReleaseMasks(transferInfo,
				mQueueManager->GetFamilyCapabilities(OwnerIndex));


			CmdBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

			Core::Utils::RecordBufferTransferBarrier(transferInfo);

			CmdBuffer.end();

			vk::SubmitInfo submitInfo{};
			submitInfo.setCommandBuffers(CmdBuffer);
			submitInfo.setSignalSemaphores(bufferReleased);

			return submitInfo;
		});
}

template<typename T>
inline void Buffer<T>::AcquireBuffer(uint32_t acquiringFamily, vk::Semaphore bufferReleased) const
{
	uint32_t OwnerIndex = mChunk.BufferHandles->Config.ResourceOwner;
	uint32_t DstIndex = acquiringFamily;

	InvokeProcess(DstIndex, [bufferReleased, OwnerIndex, DstIndex, this](
		vk::CommandBuffer CmdBuffer)->vk::SubmitInfo
		{
			Core::BufferOwnershipTransferInfo transferInfo{};

			transferInfo.Buffer = *mChunk.BufferHandles;
			transferInfo.CmdBuffer = CmdBuffer;
			transferInfo.SrcFamilyIndex = OwnerIndex;
			transferInfo.DstFamilyIndex = DstIndex;

			Core::Utils::FillBufferAcquireMasks(transferInfo,
				mQueueManager->GetFamilyCapabilities(DstIndex));


			CmdBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

			Core::Utils::RecordBufferTransferBarrier(transferInfo);

			CmdBuffer.end();

			vk::PipelineStageFlags waitFlags[] = { vk::PipelineStageFlagBits::eTopOfPipe };

			vk::SubmitInfo submitInfo{};
			submitInfo.setCommandBuffers(CmdBuffer);
			submitInfo.setWaitSemaphores(bufferReleased);
			submitInfo.setWaitDstStageMask(waitFlags);

			return submitInfo;
		});
}

template<typename T>
void Buffer<T>::MakeHollow()
{
	mChunk.BufferHandles.Reset();
	mChunk.BufferHandles->ElemCount = 0;
}

// Boolean specialization

using GenericBuffer = Buffer<bool>;

template<>
class Buffer<bool> : public RecordableResource
{
public:
	using Byte = uint8_t;
	using Type = bool;

public:
	Buffer() = default;

	template <typename Iter>
	void AppendBuf(Iter Begin, Iter End);

	template <typename Iter>
	void SetBuf(Iter Begin, Iter End, size_t Offset = 0);

	template <typename Iter>
	void FetchMemory(Iter Begin, Iter End, size_t Offset = 0);

	template <typename T>
	T* MapMemory(size_t Count, size_t Offset = 0) const;
	void UnmapMemory() const;

	void InsertMemoryBarrier(vk::CommandBuffer commandBuffer, const MemoryBarrierInfo& pipelineBarrierInfo);

	// TODO: Routine can be further optimized
	void TransferOwnership(uint32_t DstQueueFamilyIndex) const;
	void TransferOwnership(vk::QueueFlagBits optimizedCaps) const;

	void Clear() { mChunk.BufferHandles->ElemCount = 0; }

	Core::BufferChunk GetBufferChunk() const { return mChunk; }
	const Core::Buffer& GetNativeHandles() const { return *mChunk.BufferHandles; }
	Core::BufferConfig GetBufferConfig() const { return mChunk.BufferHandles->Config; }

	size_t GetSize() const { return mChunk.BufferHandles->ElemCount; }
	vk::DeviceSize GetDeviceSize() const { return mChunk.BufferHandles->ElemCount; }
	size_t GetCapacity() const { return mChunk.BufferHandles->Config.ElemCount; }

	void Reserve(size_t NewCap);
	void Resize(size_t NewSize);
	void ShrinkToFit();

	explicit operator bool() const { return static_cast<bool>(mChunk.BufferHandles); }

private:
	Core::BufferChunk mChunk;

private:
	explicit Buffer(Core::BufferChunk&& InputChunk)
		: mChunk(std::move(InputChunk)) {}

	friend class ResourcePool;

	template <typename V>
	friend void CopyBufferRegions(Buffer<V>& DstBuf, const Buffer<V>& SrcBuf,
		const std::vector<vk::BufferCopy>& CopyRegions);

private:
	// Helper Functions...
	void ScaleCapacity(size_t NewSize);
	void ScaleCapacityWithoutLoss(size_t NewSize);
	void CopyGPU(Core::Buffer& DstBuffer, const Core::Buffer& SrcBuffer,
		const vk::ArrayProxy<vk::BufferCopy>& CopyRegions);

	// Helper methods for ownership transfer
	void ReleaseBuffer(uint32_t dstIndex, vk::Semaphore bufferReleased) const;
	void AcquireBuffer(uint32_t acquiringFamily, vk::Semaphore bufferReleased) const;

	void MakeHollow();
};

template <typename T, typename Cont>
Buffer<T>& operator <<(Buffer<T>& vkBuf, const Cont& cpuBuf)
{
	vkBuf.AppendBuf(cpuBuf.begin(), cpuBuf.end());
	return vkBuf;
}

template <typename T>
Buffer<T>& operator <<(Buffer<T>& vkBuf, const T& cpuVal)
{
	vkBuf.AppendBuf(&cpuVal, &cpuVal + 1);
	return vkBuf;
}

template <typename T, typename Cont>
Buffer<T>& operator >>(Buffer<T>& vkBuf, Cont& cpuBuf)
{
	size_t offset = cpuBuf.size();
	cpuBuf.resize(cpuBuf.size() + vkBuf.GetSize());

	vkBuf.FetchMemory(cpuBuf.begin() + offset, cpuBuf.end());
	return vkBuf;
}

template<typename Iter>
void Buffer<bool>::AppendBuf(Iter Begin, Iter End)
{
	std::span range(Begin, End - Begin);

	static_assert(std::ranges::contiguous_range<decltype(range)>,
		"vkEngine::Buffer::SetBuf only accepts contiguous memory");

	SetBuf<Iter>(Begin, End, mChunk.BufferHandles->ElemCount);
}

template<typename Iter>
void Buffer<bool>::SetBuf(Iter Begin, Iter End, size_t Offset)
{
	std::span range(Begin, End - Begin);

	static_assert(std::ranges::contiguous_range<decltype(range)>,
		"vkEngine::Buffer::SetBuf only accepts contiguous memory");

	constexpr size_t ElemSize = sizeof(decltype(*Begin));

	// Converting the units to bytes instead of count
	Offset *= ElemSize;
	size_t Count = (End - Begin) * ElemSize;

	Reserve(Count + Offset);

	Byte* Memory = MapMemory<Byte>(Count, Offset);
	std::memcpy(Memory, &(*Begin), Count);
	UnmapMemory();

	mChunk.BufferHandles->ElemCount = Count + Offset;
}

template <typename Iter>
void Buffer<bool>::FetchMemory(Iter Begin, Iter End, size_t Offset /*= 0*/)
{
	std::span range(Begin, End - Begin);

	static_assert(std::ranges::contiguous_range<decltype(range)>,
		"vkEngine::Buffer::SetBuf only accepts contiguous memory");

	if (Begin == End)
		return;

	size_t Count = End - Begin;

	Byte* Memory = MapMemory<Byte>(Count * sizeof(Type), Offset * sizeof(Type));
	std::memcpy(&(*Begin), Memory, Count * sizeof(decltype(*Begin)));
	UnmapMemory();
}

template<typename T>
T* Buffer<bool>::MapMemory(size_t Count, size_t Offset) const
{
	return (T*) mChunk.Device->mapMemory(mChunk.BufferHandles->Memory,
		Offset * sizeof(T), Count * sizeof(T));
}

VK_END
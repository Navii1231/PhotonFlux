#pragma once
#include "ProcessConfig.h"
#include "Process/Queues.h"
#include "Process/Executor.h"

VK_BEGIN

class CommandPools;

// Thread safe
class CommandBufferAllocator
{
public:
	CommandBufferAllocator() = default;
	~CommandBufferAllocator();

	vk::CommandBuffer BeginOneTimeCommands(vk::CommandBufferLevel level =
		vk::CommandBufferLevel::ePrimary) const;

	void EndOneTimeCommands(vk::CommandBuffer CmdBuffer, Core::Executor Executor) const;

	vk::CommandBuffer Allocate(vk::CommandBufferLevel level =
		vk::CommandBufferLevel::ePrimary) const;

	void Free(vk::CommandBuffer CmdBuffer) const;

	const CommandPools* GetPoolManager() const { return mParentReservoir; }
	uint32_t GetFamilyIndex() const { return mFamilyIndex; }

	bool operator ==(const CommandBufferAllocator& Other) const
		{ return mCommandPool == Other.mCommandPool; }

	bool operator !=(const CommandBufferAllocator& Other) const
		{ return !(*this == Other); }

	explicit operator bool() const { return static_cast<bool>(mCommandPool); }

private:
	// Fields...
	Core::Ref<Core::CommandPoolData> mCommandPool;
	uint32_t mFamilyIndex = -1;

	const CommandPools* mParentReservoir = nullptr;

	Core::Ref<vk::Device> mDevice;

private:
	// Debug...
#if _DEBUG
	mutable std::shared_ptr<Core::CommandBufferSet> mAllocatedInstances = nullptr;
#endif

	void AddInstanceDebug(vk::CommandBuffer CmdBuffer) const;
	void RemoveInstanceDebug(vk::CommandBuffer CmdBuffer) const;

	// Not necessary...
	void DestructionChecksDebug() const;

	friend class CommandPools;
};

using CommandAllocatorMap = std::unordered_map<uint32_t, CommandBufferAllocator>;

// Thread safe
class CommandPools
{
public:
	CommandPools() = default;

	const CommandBufferAllocator& FindCmdPool(uint32_t familyIndex) const;
	const CommandBufferAllocator& operator[](uint32_t familyIndex) const { return FindCmdPool(familyIndex); }

	vk::CommandPoolCreateFlags GetCreationFlags() const { return mCreationFlags; }
	Core::Ref<vk::Device> GetDeviceHandle() const { return mDevice; }
	Core::QueueFamilyIndices GetQueueFamilyIndices() const { return mIndices; }

	bool operator ==(const CommandPools& Other) const
		{ return mCommandPools == Other.mCommandPools; }

	bool operator !=(const CommandPools& Other) const
		{ return !(*this == Other); }

	explicit operator bool() const { return !mCommandPools.empty(); }

private:
	CommandAllocatorMap mCommandPools;

	vk::CommandPoolCreateFlags mCreationFlags;
	Core::QueueFamilyIndices mIndices;
	Core::Ref<vk::Device> mDevice;

private:
	CommandPools(Core::Ref<vk::Device> device, const Core::QueueFamilyIndices& indices,
		vk::CommandPoolCreateFlags flags);

	Core::Ref<Core::CommandPoolData> CreateCommandPool(uint32_t index);

private:
	void AssignTrackerDebug(CommandBufferAllocator& Allocator)const;
	void DoCopyChecksDebug(const CommandPools* Other) const;

	CommandBufferAllocator CreateAllocator(uint32_t familyIndex);

	friend class Context;
};


VK_END

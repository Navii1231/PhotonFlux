#pragma once
#include "ProcessConfig.h"
#include "Process/Queues.h"
#include "Process/Executor.h"

VK_BEGIN

using CommandPoolMap = std::unordered_map<uint32_t, Core::Ref<Core::CommandPoolData>>;
class CommandPoolManager;

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

	const CommandPoolManager* GetPoolManager() const { return mPoolManager; }
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

	const CommandPoolManager* mPoolManager = nullptr;

	Core::Ref<vk::Device> mDevice;

private:
	// Debug...
#if _DEBUG
	mutable std::shared_ptr<std::set<vk::CommandBuffer>> mAllocatedInstances = nullptr;
#endif

	void AddInstanceDebug(vk::CommandBuffer CmdBuffer) const;
	void RemoveInstanceDebug(vk::CommandBuffer CmdBuffer) const;

	void DoDestructionChecksDebug() const;
	void DoCopyChecksDebug(const CommandBufferAllocator* Other) const;

	friend class CommandPoolManager;
};

class CommandPoolManager
{
public:
	CommandPoolManager() = default;

	CommandBufferAllocator GetAllocator(uint32_t familyIndex) const
	{
		CommandBufferAllocator Allocator{};
		Allocator.mDevice = mDevice;
		Allocator.mCommandPool = FindCmdPool(familyIndex);
		Allocator.mFamilyIndex = familyIndex;
		Allocator.mPoolManager = this;

		AssignTrackerDebug(Allocator);

		return Allocator;
	}

	vk::CommandPoolCreateFlags GetCreationFlags() const { return mCreationFlags; }
	Core::Ref<vk::Device> GetDeviceHandle() const { return mDevice; }
	Core::QueueFamilyIndices GetQueueFamilyIndices() const { return mIndices; }

	bool operator ==(const CommandPoolManager& Other) const
		{ return mCommandPools == Other.mCommandPools; }

	bool operator !=(const CommandPoolManager& Other) const
		{ return !(*this == Other); }

	explicit operator bool() const { return !mCommandPools.empty(); }

private:
	CommandPoolMap mCommandPools;

	vk::CommandPoolCreateFlags mCreationFlags;
	Core::QueueFamilyIndices mIndices;
	Core::Ref<vk::Device> mDevice;

private:
	CommandPoolManager(Core::Ref<vk::Device> device, const Core::QueueFamilyIndices& indices,
		vk::CommandPoolCreateFlags flags);

	Core::Ref<Core::CommandPoolData> CreateCommandPool(uint32_t index);
	Core::Ref<Core::CommandPoolData> FindCmdPool(uint32_t FamilyIndex) const;

private:
	void AssignTrackerDebug(CommandBufferAllocator& Allocator)const;
	void DoCopyChecksDebug(const CommandPoolManager* Other) const;

	friend class Device;
};


VK_END

#pragma once
#include "Config.h"
#include <mutex>
#include <atomic>
#include <functional>

VK_BEGIN
VK_CORE_BEGIN

template <typename T, typename Deleter>
class ControlBlock
{
public:
	using MyType = T;
	using MyDeleter = Deleter;

public:
	ControlBlock(const T& handle, Deleter deleter)
		: mHandle(handle), mDeleter(deleter), mRefCount(1) {}

	void Inc() { mRefCount++; }
	void Dec() { mRefCount--; }

	void DeleteObject() { mDeleter(mHandle); }

	T& GetHandle() { return mHandle; }
	const T& GetHandle() const { return mHandle; }

private:
	std::atomic<size_t> mRefCount;
	T mHandle;
	Deleter mDeleter;

	template <typename T>
	friend class Ref;
};

template <typename T>
class Ref
{
public:
	using MyBlock = ControlBlock<T, std::function<void(T)>>;
	using MyHandle = typename MyBlock::MyType;
	using MyDeleter = typename MyBlock::MyDeleter;

public:
	Ref() = default;

	Ref(T handle, const MyDeleter& deleter)
		: mBlock(new MyBlock(handle, deleter)) {}

	Ref(const Ref& Other)
		: mBlock(Other.mBlock) { if(mBlock) mBlock->Inc(); }

	Ref& operator =(const Ref& Other);

	// Sets a new payload and deletes the existing one
	void SetValue(const T& handle);

	// Same has operator = with right hand side having the payload to be given handle
	void ReplaceValue(const T& handle);


	void Reset();
	
	MyHandle* operator->() const { return &mBlock->mHandle; }

	MyHandle& operator*() { return mBlock->mHandle; }
	const MyHandle& operator*() const { return mBlock->mHandle; }

	bool operator==(const Ref& Other) const { return mBlock->mHandle == Other.mBlock->mHandle; }
	bool operator!=(const Ref& Other) const { return mBlock->mHandle != Other.mBlock->mHandle; }

	explicit operator bool() const { return mBlock; }

	~Ref() { Reset(); }

private:
	MyBlock* mBlock = nullptr;

private:
	// Helper functions...
	void TryDeleting();
	bool TryDetaching();
};

template<typename T>
inline void Ref<T>::SetValue(const T& handle)
{
	// Ref must already exist in order for this to work
	_STL_ASSERT(mBlock, "Core::Ref must already exist for Ref::SetValue(T&&) to work!");

	mBlock->DeleteObject();

	mBlock->mHandle = handle;
}

template <typename T>
void Ref<T>::ReplaceValue(const T& handle)
{
	// Ref must already exist in order for this to work
	_STL_ASSERT(mBlock, "Core::Ref must already exist for Ref::ReplaceValue(const T&) to work!");

	bool Success = TryDetaching();

	if (!Success)
	{
		// Another owner of the control block exists so creating a new control block
		mBlock = new MyBlock(handle, mBlock->mDeleter);
		return;
	}

	// No owner of the control block exists which means we can recycle it for our new value
	std::_Construct_in_place(*mBlock, handle, mBlock->mDeleter);
}

template<typename T>
void Ref<T>::Reset()
{
	if (mBlock)
	{
		mBlock->Dec();
		TryDeleting();
		mBlock = nullptr;
	}
}

template<typename T>
void Ref<T>::TryDeleting()
{
	if (mBlock->mRefCount.load() == 0)
	{
		mBlock->DeleteObject();
		delete mBlock;
	}
}

template<typename T>
bool Ref<T>::TryDetaching()
{
	mBlock->Dec();

	if (mBlock->mRefCount.load() == 0)
	{
		mBlock->DeleteObject();
		return true;
	}

	return false;
}

template <typename T>
Ref<T>& Ref<T>::operator=(const Ref& Other)
{
	Reset();

	mBlock = Other.mBlock;

	if (mBlock)
		mBlock->Inc();

	return *this;
}

template <typename T, typename Fn>
inline Ref<T> CreateRef(T handle, Fn&& deleter)
{
	return Ref<T>(handle, std::move(deleter));
}

VK_CORE_END
VK_END

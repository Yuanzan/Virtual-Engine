#pragma once
#include <atomic>
#include <mutex>
#include <vector>
#include "Runnable.h"
#include "d3dUtil.h"
class PtrLink;

class MObject
{
	friend class PtrLink;
private:
	std::vector<Runnable<void(MObject*)>> disposeFunc;
	static std::atomic<unsigned int> CurrentID;
	unsigned int instanceID;
protected:
	MObject()
	{
		instanceID = CurrentID++;
	}
public:
	void AddEventBeforeDispose(const Runnable<void(MObject*)>& func)
	{
		if (disposeFunc.capacity() == 0) disposeFunc.reserve(20);
		disposeFunc.push_back(func);
	}
	void RemoveEventBeforeDispose(const Runnable<void(MObject*)>& func)
	{
		for (auto ite = disposeFunc.begin(); ite != disposeFunc.end(); ++ite)
		{
			if (*ite == func)
			{
				disposeFunc.erase(ite);
				return;
			}
		}
	}
	unsigned int GetInstanceID() const { return instanceID; }
	virtual ~MObject() noexcept;
};



struct LinkHeap
{
	void* ptr;
	void(*disposor)(void*);
	std::atomic<int> refCount;
	std::atomic<int> looseRefCount;
	static bool initialized;
	static std::vector<LinkHeap*> heapPtrs;
	static std::mutex mtx;
	static void Resize() noexcept;
	static void Initialize() noexcept;
	static LinkHeap* GetHeap(void* obj, void(*disp)(void*), size_t refCount) noexcept;
	static void ReturnHeap(LinkHeap* value) noexcept;
};

class VEngine;
class PtrWeakLink;
class PtrLink
{
	friend class VEngine;
	friend class PtrWeakLink;
	static bool globalEnabled;
private:
	template <typename T>
	struct TypeCollector
	{
		T t;
	};
public:
	LinkHeap* heapPtr;
	PtrLink() noexcept : heapPtr(nullptr)
	{

	}
	void Dispose() noexcept;
	template <typename T>
	PtrLink(T* obj) noexcept
	{
		void(*disposor)(void*) = [](void* ptr)->void
		{
			delete ((T*)ptr);
		};
		heapPtr = LinkHeap::GetHeap(obj, disposor, 1);
	}
	template <typename T>
	PtrLink(T* obj, bool) noexcept
	{
		void(*disposor)(void*) = [](void* ptr)->void
		{
			((TypeCollector<T>*)ptr)->~TypeCollector();
		};
		heapPtr = LinkHeap::GetHeap(obj, disposor, 1);
	}
	PtrLink(const PtrLink& p) noexcept
	{
		if (p.heapPtr && p.heapPtr->ptr) {
			++p.heapPtr->refCount;
			++p.heapPtr->looseRefCount;
			heapPtr = p.heapPtr;
		}
		else
		{
			heapPtr = nullptr;
		}
	}
	inline PtrLink(const PtrWeakLink& link) noexcept;
	void operator=(const PtrLink& p) noexcept
	{
		if (p.heapPtr && p.heapPtr->ptr) {
			++p.heapPtr->refCount;
			++p.heapPtr->looseRefCount;
			Dispose();
			heapPtr = p.heapPtr;
		}
		else
		{
			Dispose();
		}
	}
	void Destroy() noexcept;
	~PtrLink() noexcept
	{
		Dispose();
	}
};
class PtrWeakLink
{
public:
	LinkHeap* heapPtr;
	PtrWeakLink() noexcept : heapPtr(nullptr)
	{

	}

	void Dispose() noexcept;
	PtrWeakLink(const PtrLink& p) noexcept
	{
		if (p.heapPtr && p.heapPtr->ptr) {
			++p.heapPtr->looseRefCount;
			heapPtr = p.heapPtr;
		}
		else
		{
			heapPtr = nullptr;
		}
	}

	PtrWeakLink(const PtrWeakLink& p) noexcept
	{
		if (p.heapPtr && p.heapPtr->ptr) {
			++p.heapPtr->looseRefCount;
			heapPtr = p.heapPtr;
		}
		else
		{
			heapPtr = nullptr;
		}
	}

	void operator=(const PtrLink& p) noexcept
	{
		if (p.heapPtr && p.heapPtr->ptr) {
			++p.heapPtr->looseRefCount;
			Dispose();
			heapPtr = p.heapPtr;
		}
		else
		{
			Dispose();
		}
	}
	void Destroy() noexcept;
	void operator=(const PtrWeakLink& p) noexcept
	{
		if (p.heapPtr && p.heapPtr->ptr) {
			++p.heapPtr->looseRefCount;
			Dispose();
			heapPtr = p.heapPtr;
		}
		else
		{
			Dispose();
		}
	}
	~PtrWeakLink() noexcept
	{
		Dispose();
	}
};


template <typename T>
class ObjWeakPtr;
template <typename T>
class ObjectPtr
{
private:
	friend class ObjWeakPtr<T>;
	PtrLink link;
	inline constexpr ObjectPtr(T* ptr) noexcept :
		link(ptr)
	{

	}
	inline constexpr ObjectPtr(T* ptr, bool) noexcept :
		link(ptr, false)
	{

	}
public:
	constexpr ObjectPtr(const PtrLink& link) noexcept : link(link)
	{
	}
	inline constexpr ObjectPtr() noexcept :
		link() {}
	inline constexpr ObjectPtr(std::nullptr_t) noexcept : link()
	{

	}
	inline constexpr ObjectPtr(const ObjectPtr<T>& ptr) noexcept :
		link(ptr.link)
	{

	}
	inline constexpr ObjectPtr(const ObjWeakPtr<T>& ptr) noexcept;
	static ObjectPtr<T> MakePtr(T* ptr) noexcept
	{
		return ObjectPtr<T>(ptr);
	}
	static ObjectPtr<T> MakePtrNoMemoryFree(T* ptr) noexcept
	{
		return ObjectPtr<T>(ptr, false);
	}
	static ObjectPtr<T> MakePtr(ObjectPtr<T>) noexcept = delete;


	inline constexpr operator bool() const noexcept
	{
		return link.heapPtr != nullptr && link.heapPtr->ptr != nullptr;
	}

	inline constexpr operator T* () const noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return (T*)link.heapPtr->ptr;
	}

	inline constexpr void Destroy() noexcept
	{
		link.Destroy();
	}

	template<typename F>
	inline constexpr ObjectPtr<F> CastTo() const noexcept
	{
		return ObjectPtr<F>(link);
	}

	inline constexpr void operator=(const ObjectPtr<T>& other) noexcept
	{
		link = other.link;
	}

	inline constexpr void operator=(T* other) noexcept = delete;
	inline constexpr void operator=(void* other) noexcept = delete;
	inline constexpr void operator=(std::nullptr_t t) noexcept
	{
		link.Dispose();
	}

	inline constexpr T* operator->() const noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return (T*)link.heapPtr->ptr;
	}

	inline constexpr T& operator*() noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return *(T*)link.heapPtr->ptr;
	}

	inline constexpr bool operator==(const ObjectPtr<T>& ptr) const noexcept
	{
		return link.heapPtr == ptr.link.heapPtr;
	}
	inline constexpr bool operator!=(const ObjectPtr<T>& ptr) const noexcept
	{
		return link.heapPtr != ptr.link.heapPtr;
	}
};

template <typename T>
class ObjWeakPtr
{
private:
	friend class ObjectPtr<T>;
	PtrWeakLink link;
public:
	inline constexpr ObjWeakPtr() noexcept :
		link() {}
	inline constexpr ObjWeakPtr(std::nullptr_t) noexcept : link()
	{

	}
	inline constexpr ObjWeakPtr(const ObjWeakPtr<T>& ptr) noexcept :
		link(ptr.link)
	{

	}
	inline constexpr ObjWeakPtr(const ObjectPtr<T>& ptr) noexcept :
		link(ptr.link)
	{

	}

	inline constexpr operator bool() const noexcept
	{
		return link.heapPtr != nullptr && link.heapPtr->ptr != nullptr;
	}

	inline constexpr operator T* () const noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return (T*)link.heapPtr->ptr;
	}

	inline constexpr void Destroy() noexcept
	{
		link.Destroy();
	}

	template<typename F>
	inline constexpr ObjWeakPtr<F> CastTo() const noexcept
	{
		return ObjWeakPtr<F>(link);
	}

	inline constexpr void operator=(const ObjWeakPtr<T>& other) noexcept
	{
		link = other.link;
	}

	inline constexpr void operator=(const ObjectPtr<T>& other) noexcept
	{
		link = other.link;
	}

	inline constexpr void operator=(T* other) noexcept = delete;
	inline constexpr void operator=(void* other) noexcept = delete;
	inline constexpr void operator=(std::nullptr_t t) noexcept
	{
		link.Dispose();
	}

	inline constexpr T* operator->() const noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return (T*)link.heapPtr->ptr;
	}

	inline constexpr T& operator*() noexcept
	{
#ifdef _DEBUG
		//Null Check!
		assert(link.heapPtr != nullptr);
#endif
		return *(T*)link.heapPtr->ptr;
	}

	inline constexpr bool operator==(const ObjWeakPtr<T>& ptr) const noexcept
	{
		return link.heapPtr == ptr.link.heapPtr;
	}
	inline constexpr bool operator!=(const ObjWeakPtr<T>& ptr) const noexcept
	{
		return link.heapPtr != ptr.link.heapPtr;
	}

};
template<typename T>
inline constexpr ObjectPtr<T>::ObjectPtr(const ObjWeakPtr<T>& ptr) noexcept :
	link(ptr.link)
{

}



inline PtrLink::PtrLink(const PtrWeakLink& p) noexcept
{
	if (p.heapPtr && p.heapPtr->ptr) {
		++p.heapPtr->refCount;
		++p.heapPtr->looseRefCount;
		heapPtr = p.heapPtr;
	}
	else
	{
		heapPtr = nullptr;
	}
}
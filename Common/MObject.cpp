#include "MObject.h"
std::atomic<unsigned int> MObject::CurrentID = 0;
bool LinkHeap::initialized = false;
std::vector<LinkHeap*> LinkHeap::heapPtrs;
std::mutex LinkHeap::mtx;
bool PtrLink::globalEnabled = true;

MObject::~MObject() noexcept
{
	for (auto ite = disposeFunc.begin(); ite != disposeFunc.end(); ++ite)
	{
		(*ite)(this);
	}

}

void PtrWeakLink::Dispose() noexcept
{
	auto a = heapPtr;
	heapPtr = nullptr;
	if (a && PtrLink::globalEnabled && (--a->looseRefCount == 0))
	{
		LinkHeap::ReturnHeap(a);
	}
}

void PtrLink::Destroy() noexcept
{
	auto bb = heapPtr;
	heapPtr = nullptr;
	if (bb && globalEnabled)
	{
		auto a = bb->ptr;
		if (a)
		{
			auto func = bb->disposor;
			bb->ptr = nullptr;
			bb->disposor = nullptr;
			func(a);
		}
	}
}

void PtrWeakLink::Destroy() noexcept
{
	auto bb = heapPtr;
	heapPtr = nullptr;
	if (bb && PtrLink::globalEnabled)
	{
		auto a = bb->ptr;
		if (a)
		{
			auto func = bb->disposor;
			bb->ptr = nullptr;
			bb->disposor = nullptr;
			func(a);
		}
	}
}

void PtrLink::Dispose() noexcept
{
	auto a = heapPtr;
	heapPtr = nullptr;
	if (a && globalEnabled &&
		(--a->refCount == 0))
	{
		auto bb = a->ptr;
		if (bb)
		{
			auto func = a->disposor;
			a->ptr = nullptr;
			a->disposor = nullptr;
			func(bb);
			if (--a->looseRefCount == 0)
				LinkHeap::ReturnHeap(a);
		}
	}

}

void LinkHeap::Resize() noexcept
{
	if (heapPtrs.empty())
	{
		LinkHeap* ptrs = (LinkHeap*)malloc(sizeof(LinkHeap) * 512);
		for (uint32_t i = 0; i < 100; ++i)
		{
			heapPtrs.push_back(ptrs + i);
		}
	}
}
void  LinkHeap::Initialize() noexcept
{
	if (initialized) return;
	initialized = true;
	heapPtrs.reserve(1024);
}
LinkHeap* LinkHeap::GetHeap(void* obj, void(*disp)(void*), size_t refCount) noexcept
{
	LinkHeap* ptr = nullptr;
	{
		std::lock_guard<std::mutex> lck(mtx);
		Initialize();
		Resize();
		auto ite = heapPtrs.end() - 1;
		ptr = *ite;
		heapPtrs.erase(ite);
	}
	ptr->ptr = obj;
	ptr->disposor = disp;
	ptr->refCount = refCount;
	ptr->looseRefCount = refCount;
	return ptr;
}
void  LinkHeap::ReturnHeap(LinkHeap* value) noexcept
{
	std::lock_guard<std::mutex> lck(mtx);
	heapPtrs.push_back(value);
}
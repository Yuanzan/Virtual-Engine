#include "CBufferPool.h" 
#include <math.h>
void CBufferPool::Add(ID3D12Device* device)
{
	ObjectPtr<UploadBuffer> newBuffer = ObjectPtr<UploadBuffer>::MakePtr(new UploadBuffer(device, capacity, true, stride));
	arr.push_back(newBuffer);
	for (UINT i = 0; i < capacity; ++i)
	{
		ConstBufferElement ele;
		ele.buffer = newBuffer;
		ele.element = i;
		poolValue.push_back(ele);
	}
}

CBufferPool::CBufferPool(UINT stride, UINT initCapacity) :
	capacity(initCapacity),
	stride(stride)
{
	poolValue.reserve(initCapacity);
	arr.reserve(10);
}

CBufferPool::~CBufferPool()
{
}

ConstBufferElement CBufferPool::Get(ID3D12Device* device)
{
	UINT value = capacity;
	if (poolValue.empty())
	{
		Add(device);
	}
	auto ite = poolValue.end() - 1;
	ConstBufferElement pa = *ite;
	poolValue.erase(ite);
	return pa;

}

void CBufferPool::Return(const ConstBufferElement& target)
{
	poolValue.push_back(target);
}

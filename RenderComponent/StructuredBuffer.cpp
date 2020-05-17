#include "StructuredBuffer.h"
#include "../Singleton/FrameResource.h"
StructuredBuffer::StructuredBuffer(
	ID3D12Device* device,
	StructuredBufferElement* elementsArray,
	UINT elementsCount,
	bool isIndirect,
	bool isReadable
) : elements(elementsCount), offsets(elementsCount)
{
	memcpy(elements.data(), elementsArray, sizeof(StructuredBufferElement) * elementsCount);
	uint64_t offst = 0;
	for (UINT i = 0; i < elementsCount; ++i)
	{
		offsets[i] = offst;
		auto& ele = elements[i];
		offst += ele.stride * ele.elementCount;
	}
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(offst, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		isReadable ? D3D12_RESOURCE_STATE_GENERIC_READ : (isIndirect ? D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT : D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		nullptr,
		IID_PPV_ARGS(&Resource)
	));
}

void StructuredBuffer::TransformBufferState(ID3D12GraphicsCommandList* commandList, StructuredBufferState beforeState, StructuredBufferState afterState) const
{
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		Resource.Get(),
		(D3D12_RESOURCE_STATES)beforeState,
		(D3D12_RESOURCE_STATES)afterState
	));
}

uint64_t StructuredBuffer::GetStride(UINT index) const
{
	return elements[index].stride;
}
uint64_t StructuredBuffer::GetElementCount(UINT index) const
{
	return elements[index].elementCount;
}
D3D12_GPU_VIRTUAL_ADDRESS StructuredBuffer::GetAddress(UINT element, UINT index) const
{

#ifdef NDEBUG
	auto& ele = elements[element];
	return Resource->GetGPUVirtualAddress() + offsets[element] + ele.stride * index;
#else
	if (element >= elements.size())
	{
		throw "Element Out of Range Exception";
	}
	auto& ele = elements[element];
	if (index >= ele.elementCount)
	{
		throw "Index Out of Range Exception";
	}

	return Resource->GetGPUVirtualAddress() + offsets[element] + ele.stride * index;
#endif
}

uint64_t StructuredBuffer::GetAddressOffset(UINT element, UINT index) const
{
#ifdef NDEBUG
	auto& ele = elements[element];
	return offsets[element] + ele.stride * index;
#else
	if (element >= elements.size())
	{
		throw "Element Out of Range Exception";
	}
	auto& ele = elements[element];
	if (index >= ele.elementCount)
	{
		throw "Index Out of Range Exception";
	}

	return offsets[element] + ele.stride * index;
#endif
}

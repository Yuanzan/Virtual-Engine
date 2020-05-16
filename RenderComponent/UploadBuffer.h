#pragma once

#include "IGPUResource.h"
class FrameResource;
class UploadBuffer final : public IGPUResource
{
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, size_t stride);
	UploadBuffer() :
		mMappedData(0),
		mStride(0),
		mElementCount(0),
		mElementByteSize(0),
		mIsConstantBuffer(false) {}
	void Create(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, size_t stride);
	virtual ~UploadBuffer()
	{
		if (Resource != nullptr)
			Resource->Unmap(0, nullptr);
	}
	inline void CopyData(UINT elementIndex, const void* data) const
	{
		char* dataPos = (char*)mMappedData;
		size_t offset = (size_t)elementIndex * mElementByteSize;
		dataPos += offset;
		memcpy(dataPos, data, mStride);
	}
	inline void CopyDataInside(UINT from, UINT to) const
	{
		char* dataPos = (char*)mMappedData;
		size_t fromOffset = from * (size_t)mElementByteSize;
		size_t toOffset = to * (size_t)mElementByteSize;
		memcpy(dataPos + toOffset, dataPos + fromOffset, mStride);
	}
	inline void CopyDatas(UINT startElementIndex, UINT elementCount, const void* data) const
	{
		char* dataPos = (char*)mMappedData;
		size_t offset = startElementIndex * (size_t)mElementByteSize;
		dataPos += offset;
		memcpy(dataPos, data, (elementCount - 1) * (size_t)mElementByteSize + mStride);
	}
	inline void CopyFrom(UploadBuffer* otherBuffer, UINT selfStartIndex, UINT otherBufferStartIndex, UINT elementCount) const
	{
		char* otherPtr = (char*)otherBuffer->mMappedData;
		char* currPtr = (char*)mMappedData;
		otherPtr += otherBuffer->mElementByteSize * (size_t)otherBufferStartIndex;
		currPtr += mElementByteSize * (size_t)selfStartIndex;
		memcpy(currPtr, otherPtr, elementCount * (size_t)mElementByteSize);
	}
	inline D3D12_GPU_VIRTUAL_ADDRESS GetAddress(UINT elementCount) const
	{
		return Resource->GetGPUVirtualAddress() + elementCount * (size_t)mElementByteSize;
	}
	constexpr void* GetMappedDataPtr(UINT element) const
	{
		char* dataPos = (char*)mMappedData;
		size_t offset = element * (size_t)mElementByteSize;
		dataPos += offset;
		return dataPos;
	}
	constexpr size_t GetStride() const { return mStride; }
	constexpr size_t GetAlignedStride() const { return mElementByteSize; }

	inline UINT GetElementCount() const
	{
		return mElementCount;
	}
	UploadBuffer(const UploadBuffer& another) = delete;
private:
	struct UploadCommand
	{
		UINT startIndex;
		UINT count;
	};
	void* mMappedData;
	size_t mStride;
	UINT mElementCount;
    UINT mElementByteSize;
    bool mIsConstantBuffer;

};
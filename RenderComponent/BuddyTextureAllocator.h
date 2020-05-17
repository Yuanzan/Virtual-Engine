#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MetaLib.h"
#include "../Common/MObject.h"
#include "Utility/BinaryAllocator.h"
#include "TextureHeap.h"
class ITexture;
class BuddyTextureAllocator
{
private:
	struct AllocateUnit
	{
		std::unique_ptr<BinaryAllocator> allocator;
		std::unique_ptr<TextureHeap> texHeap;
	};
	uint64_t chunkSize;
	uint64_t fullSize;
	uint allocatorLayer;
	std::vector<AllocateUnit> alloc;
	std::unordered_map<size_t, std::pair<BinaryAllocator*, BuddyAllocatorHandle>> allocatedMap;
	uint mipCount;
	bool isRenderTexture;
	static uint BitCount(uint64_t x);
public:
	BuddyTextureAllocator(
		ID3D12Device* device,
		uint32_t maximumResolution,
		uint32_t minimumResolution,
		uint32_t mipLevel,
		DXGI_FORMAT format,
		bool isRenderTexture); 
	ObjectPtr<ITexture> GetNewTexture(
		ID3D12Device* device,
		DXGI_FORMAT format,
		uint32_t resolution);

	void ReturnTexture(ITexture* tex);

	~BuddyTextureAllocator();
};
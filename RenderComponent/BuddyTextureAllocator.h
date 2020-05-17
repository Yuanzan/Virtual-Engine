#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MetaLib.h"
#include "../Common/MObject.h"
#include "Utility/BinaryAllocator.h"
class ITexture;
class BuddyTextureAllocator
{
private:
	uint allocTreeLayer;
	bool isRenderTexture;
	std::vector<StackObject<BinaryAllocator, true>> alloc;
	std::unordered_map<size_t, BuddyAllocHandle> allocatedMap;
public:
	BuddyTextureAllocator(
		ID3D12Device* device,
		uint32_t maximumResolution,
		uint32_t mipLevel,
		DXGI_FORMAT format,
		bool isRenderTexture); 
	ObjectPtr<ITexture> GetNewTexture(
		uint32_t resolution);

	void ReturnTexture(ITexture* tex);

	~BuddyTextureAllocator();
};
#include "BuddyTextureAllocator.h"
#include "RenderTexture.h"
#include "Texture.h"
BuddyTextureAllocator:: BuddyTextureAllocator(
	ID3D12Device* device,
	uint32_t maximumResolution,
	uint32_t mipLevel,
	DXGI_FORMAT format,
	bool isRenderTexture) : 
	isRenderTexture(isRenderTexture)
{
	uint64_t size;
	if (isRenderTexture)
	{
		size = RenderTexture::GetSizeFromProperty(
			device,
			maximumResolution,
			maximumResolution,
			RenderTextureFormat::GetColorFormat(format),
			TextureDimension::Tex2D,
			1,
			mipLevel,
			RenderTextureState::Generic_Read);
	}
	else
	{
		size = Texture::GetSizeFromProperty(
			device,
			maximumResolution,
			maximumResolution,
			1,
			TextureDimension::Tex2D,
			mipLevel,
			format);
	}
	size /= 65536;
	allocatedMap.reserve(1031);
	alloc.reserve(10);
	auto bitCount = [](uint64_t x)->uint
	{
		auto bitCount_Inside = [](uint64_t x)->uint
		{
			for (uint i = 0; i < 63; ++i)
			{
				x >>= 1;
				if (x == 0)
					return x;
			}
			return 63;
		};
		return bitCount_Inside(x - 1) + 1;
	};
	StackObject<BinaryAllocator, true>& firstAlloc = alloc.emplace_back();
	allocTreeLayer = bitCount(size);
	firstAlloc.New(allocTreeLayer);
}

BuddyTextureAllocator::~BuddyTextureAllocator()
{
	
}
/*
ObjectPtr<ITexture> BuddyTextureAllocator::GetNewTexture(
	uint32_t resolution)
{
	for (auto ite = alloc.begin(); ite != alloc.end(); ++ite)
	{
		//(*ite)->TryAllocate()
	}
}*/

void BuddyTextureAllocator::ReturnTexture(ITexture* tex)
{

}
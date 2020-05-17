#include "BuddyTextureAllocator.h"
#include "RenderTexture.h"
#include "Texture.h"
uint BuddyTextureAllocator::BitCount(uint64_t x)
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

BuddyTextureAllocator:: BuddyTextureAllocator(
	ID3D12Device* device,
	uint32_t maximumResolution,
	uint32_t minimumResolution,
	uint32_t mipLevel,
	DXGI_FORMAT format,
	bool isRenderTexture) : 
	isRenderTexture(isRenderTexture),
	mipCount(mipLevel)
{
	minimumResolution = Min(maximumResolution, minimumResolution);
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
		chunkSize = RenderTexture::GetSizeFromProperty(
			device,
			minimumResolution,
			minimumResolution,
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
		chunkSize = Texture::GetSizeFromProperty(
			device,
			minimumResolution,
			minimumResolution,
			1,
			TextureDimension::Tex2D,
			mipLevel,
			format);
	}
	fullSize = size;
	
	size /= chunkSize;
	
	allocatedMap.reserve(1031);
	alloc.reserve(10);
	
	AllocateUnit& firstAlloc = alloc.emplace_back();
	allocatorLayer = BitCount(size) + 1;
	firstAlloc.allocator = std::unique_ptr<BinaryAllocator>(new BinaryAllocator(allocatorLayer));
	firstAlloc.texHeap = std::unique_ptr<TextureHeap>(new TextureHeap(device, fullSize));
}

BuddyTextureAllocator::~BuddyTextureAllocator()
{
	
}

ObjectPtr<ITexture> BuddyTextureAllocator::GetNewTexture(
	ID3D12Device* device,
	DXGI_FORMAT format,
	uint32_t resolution)
{
	uint64_t allocateSize;
	if (isRenderTexture)
	{
		allocateSize = RenderTexture::GetSizeFromProperty(
			device,
			resolution,
			resolution,
			RenderTextureFormat::GetColorFormat(format),
			TextureDimension::Tex2D,
			1,
			mipCount,
			RenderTextureState::Generic_Read);
	}
	else
	{
		allocateSize = Texture::GetSizeFromProperty(
			device,
			resolution,
			resolution,
			1,
			TextureDimension::Tex2D,
			mipCount,
			format);

	}

	if (allocateSize > fullSize) return nullptr;
	if (allocateSize % chunkSize) return nullptr;
	uint64_t neededChunkCount = allocateSize / chunkSize;
	uint targetLayer = allocatorLayer - BitCount(neededChunkCount);
	BuddyAllocatorHandle handle;
	TextureHeap* texHeap = nullptr;
	BinaryAllocator* bAlloc = nullptr;
	bool avaliable = false;
	for (auto ite = alloc.begin(); ite != alloc.end(); ++ite)
	{
		if (ite->allocator->TryAllocate(targetLayer, handle))
		{
			avaliable = true;
			texHeap = ite->texHeap.get();
			bAlloc = ite->allocator.get();
			break;
		}
	}
	if (!avaliable)
	{
		auto& newAllocator = alloc.emplace_back();
		newAllocator.allocator = std::unique_ptr<BinaryAllocator>(new BinaryAllocator(allocatorLayer));
		newAllocator.texHeap = std::unique_ptr<TextureHeap>(new TextureHeap(device, fullSize));
		if (!newAllocator.allocator->TryAllocate(targetLayer, handle))
		{
			return nullptr;
		}
		else
		{
			texHeap = newAllocator.texHeap.get();
			bAlloc = newAllocator.allocator.get();
		}
	}
	ITexture* tex = nullptr;
	if (isRenderTexture)
	{
		tex = new RenderTexture(
			device,
			resolution,
			resolution,
			RenderTextureFormat::GetColorFormat(format),
			TextureDimension::Tex2D,
			1,
			mipCount,
			RenderTextureState::Render_Target,
			texHeap,
			handle.GetOffset() * chunkSize
		);
	}
	else
	{
		tex = new Texture(
			device,
			resolution,
			resolution,
			1,
			TextureDimension::Tex2D,
			mipCount,
			format,
			texHeap,
			handle.GetOffset() * chunkSize
		);
	}
	allocatedMap.insert(std::pair<size_t, std::pair<BinaryAllocator*, BuddyAllocatorHandle>>((size_t)tex, {bAlloc, handle}));
	return ObjectPtr<ITexture>::MakePtr(tex);
}

void BuddyTextureAllocator::ReturnTexture(ITexture* tex)
{
	auto ite = allocatedMap.find((size_t)tex);
	if (ite == allocatedMap.end()) return;
	ite->second.first->Return(ite->second.second);
	allocatedMap.erase(ite);
}
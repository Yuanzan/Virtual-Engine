#pragma once
#include "../Common/d3dUtil.h"
#include <string>
#include <vector>
#include "../Common/MObject.h"
#include "ITexture.h"
class DescriptorHeap;
class UploadBuffer;
class FrameResource;
class TextureHeap;

struct TextureData
{
	UINT width;
	UINT height;
	UINT depth;
	TextureDimension textureType;
	UINT mipCount;
	enum LoadFormat
	{
		LoadFormat_RGBA8 = 0,
		LoadFormat_RGBA16 = 1,
		LoadFormat_RGBAFloat16 = 2,
		LoadFormat_RGBAFloat32 = 3,
		LoadFormat_RGFLOAT16 = 4,
		LoadFormat_RG16 = 5,
		LoadFormat_BC7 = 6,
		LoadFormat_BC6H = 7,
		LoadFormat_UINT = 8,
		LoadFormat_UINT2 = 9,
		LoadFormat_UINT4 = 10,
		LoadFormat_UNorm16 = 11,
		LoadFormat_BC5U = 12,
		LoadFormat_BC5S = 13,
		LoadFormat_Num = 14
	};
	LoadFormat format;
};

class Texture final : public ITexture
{
private:
	void GetResourceViewDescriptor(D3D12_SHADER_RESOURCE_VIEW_DESC& desc) const;
	bool loaded = false;
	Texture() {}
	D3D12_RESOURCE_DESC CreateWithoutResource(
		std::vector<char>& dataResults,
		TextureData& data,
		ID3D12Device* device,
		const std::string& filePath,
		TextureDimension type = TextureDimension::Tex2D,
		uint32_t maximumLoadMipmap = 20,
		uint32_t startMipMap = 0
	);
	void GenerateResources(
		ID3D12Device* device,
		std::vector<char>& dataResults,
		TextureData& data,
		D3D12_RESOURCE_DESC& desc,
		TextureHeap* placedHeap,
		size_t placedOffset);
public:
	static TextureHeap* GenerateTextureFromFiles(
		std::vector<ObjectPtr<Texture>>& texs,
		ID3D12Device* device,
		const std::vector<std::string>& filePaths,
		size_t placedOffset,
		TextureDimension type = TextureDimension::Tex2D,
		uint32_t maximumLoadMipmap = 20,
		uint32_t startMipMap = 0
	);

	//Async Load
	Texture(
		ID3D12Device* device,
		const std::string& filePath,
		TextureDimension type = TextureDimension::Tex2D,
		uint32_t maximumLoadMipmap = 20,
		uint32_t startMipMap = 0,
		TextureHeap* placedHeap = nullptr,
		size_t placedOffset = 0
	);
	Texture(
		ID3D12Device* device,
		UINT width,
		UINT height,
		UINT depth,
		TextureDimension textureType,
		UINT mipCount,
		DXGI_FORMAT format,
		TextureHeap* placedHeap = nullptr,
		size_t placedOffset = 0
	);

	//Sync Copy
	Texture(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource,
		UploadBuffer* buffer,
		UINT width,
		UINT height,
		UINT depth,
		TextureDimension textureType,
		UINT mipCount,
		TextureData::LoadFormat format,
		TextureHeap* placedHeap = nullptr,
		size_t placedOffset = 0
	);
	static uint64_t GetSizeFromProperty(
		ID3D12Device* device,
		uint width,
		uint height,
		uint depth,
		TextureDimension textureType,
		uint mipCount,
		DXGI_FORMAT format);
	
	bool IsLoaded() const { return loaded; }
	virtual void BindSRVToHeap(const DescriptorHeap* targetHeap, UINT index, ID3D12Device* device) const;
};


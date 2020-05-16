#pragma once
#include "../Common/MObject.h"
#include "../Common/d3dUtil.h"
#include "ITexture.h"
#include "../RenderComponent/DescriptorHeap.h"
class TextureHeap;
enum CubeMapFace
{
	CubeMapFace_PositiveX = 0,
	CubeMapFace_NegativeX = 1,
	CubeMapFace_PositiveY = 2,
	CubeMapFace_NegativeY = 3,
	CubeMapFace_PositiveZ = 4,
	CubeMapFace_NegativeZ = 5
};
enum RenderTextureDepthSettings
{
	RenderTextureDepthSettings_None,
	RenderTextureDepthSettings_Depth16,
	RenderTextureDepthSettings_Depth32,
	RenderTextureDepthSettings_DepthStencil
};

enum class RenderTextureUsage : bool
{
	ColorBuffer = false,
	DepthBuffer = true
};

enum class RenderTextureState : UCHAR
{
	Render_Target = 0,
	Unordered_Access = 1,
	Generic_Read = 2
};

struct RenderTextureFormat
{
	RenderTextureUsage usage;
	union {
		DXGI_FORMAT colorFormat;
		RenderTextureDepthSettings depthFormat;
	};
	static RenderTextureFormat GetColorFormat(DXGI_FORMAT format)
	{
		RenderTextureFormat f;
		f.usage = RenderTextureUsage::ColorBuffer;
		f.colorFormat = format;
		return f;
	}

	static RenderTextureFormat GetDepthFormat(RenderTextureDepthSettings depthFormat)
	{
		RenderTextureFormat f;
		f.usage = RenderTextureUsage::DepthBuffer;
		f.depthFormat = depthFormat;
		return f;
	}
};

struct RenderTextureDescriptor
{
	UINT width;
	UINT height;
	UINT depthSlice;
	TextureDimension type;
	RenderTextureFormat rtFormat;
	constexpr bool operator==(const RenderTextureDescriptor& other) const
	{
		bool value = width == other.width &&
			height == other.height &&
			depthSlice == other.depthSlice &&
			type == other.type &&
			rtFormat.usage == other.rtFormat.usage;
		if (value)
		{
			if (rtFormat.usage == RenderTextureUsage::ColorBuffer)
			{
				return rtFormat.colorFormat == other.rtFormat.colorFormat;
			}
			else
			{
				return rtFormat.depthFormat == other.rtFormat.depthFormat;
			}
		}
		return false;
	}

	constexpr bool operator!=(const RenderTextureDescriptor& other) const
	{
		return !operator==(other);
	}
};

class RenderTexture  final : public ITexture
{
private:
	RenderTextureUsage usage;
	D3D12_RESOURCE_STATES initState;
	D3D12_RESOURCE_STATES writeState;
	D3D12_RESOURCE_STATES readState;
	DescriptorHeap rtvHeap;
	std::vector<uint> uavDescIndices;
	float clearColor;
	void GetColorViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)const;
	void GetColorUAVDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, UINT targetMipLevel)const;
public:
	inline static const D3D12_RESOURCE_STATES ColorReadState = D3D12_RESOURCE_STATE_GENERIC_READ;
	inline static const D3D12_RESOURCE_STATES DepthReadState = D3D12_RESOURCE_STATE_DEPTH_READ;
	inline static const D3D12_RESOURCE_STATES RenderTargetState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	inline static const D3D12_RESOURCE_STATES DepthWriteState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	inline static const D3D12_RESOURCE_STATES UnorderedAccessState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	inline static const D3D12_RESOURCE_STATES CopyDestState = D3D12_RESOURCE_STATE_COPY_DEST;

	uint GetGlobalUAVDescIndex(uint mipLevel) const;
	static size_t GetSizeFromProperty(
		ID3D12Device* device,
		UINT width,
		UINT height,
		RenderTextureFormat rtFormat,
		TextureDimension type,
		UINT depthCount,
		UINT mipCount,
		RenderTextureState initState);
	RenderTexture(
		ID3D12Device* device,
		UINT width,
		UINT height,
		RenderTextureFormat rtFormat,
		TextureDimension type,
		UINT depthCount,
		UINT mipCount,
		RenderTextureState initState = RenderTextureState::Render_Target,
		TextureHeap* targetHeap = nullptr,
		size_t placedOffset = 0,
		float clearColor = 0
	);
	~RenderTexture();
	D3D12_RESOURCE_STATES GetInitState() const
	{
		return initState;
	}
	D3D12_RESOURCE_STATES GetWriteState() const
	{
		return writeState;
	}
	D3D12_RESOURCE_STATES GetReadState() const
	{
		return readState;
	}
	RenderTextureUsage GetUsage() const { return usage; }
	void BindRTVToHeap(const DescriptorHeap* targetHeap, UINT index, ID3D12Device* device, UINT slice, UINT mip)const;
	void SetViewport(ID3D12GraphicsCommandList* commandList, uint mipCount = 0)const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetColorDescriptor(UINT slice, UINT mip)const;
	virtual void BindSRVToHeap(const DescriptorHeap* targetHeap, UINT index, ID3D12Device* device)const;
	void BindUAVToHeap(const DescriptorHeap* targetHeap, UINT index, ID3D12Device* device, UINT targetMipLevel)const;
	void ClearRenderTarget(ID3D12GraphicsCommandList* commandList, UINT slice, UINT mip)const;
};

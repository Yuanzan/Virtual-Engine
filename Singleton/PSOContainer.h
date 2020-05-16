#pragma once
#include "../RenderComponent/Shader.h"
#include <unordered_map>
#include <mutex>
struct PSODescriptor
{
	const Shader* shaderPtr;
	UINT shaderPass;
	UINT meshLayoutIndex;
	size_t hash;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topology;
	bool operator==(const PSODescriptor& other)const;
	PSODescriptor() :
		topology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
	{

	}
	void GenerateHash()
	{
		size_t value = shaderPass;
		value <<= 8;
		value &= meshLayoutIndex;
		value <<= 4;
		value &= topology;
		value <<= 4;
		value ^= reinterpret_cast<size_t>(shaderPtr);
		std::hash<size_t> h;
		hash = h(value);
	}
};

namespace std
{
	template <>
	struct hash<PSODescriptor>
	{
		size_t operator()(const PSODescriptor& key) const
		{
			return key.hash;
		}
	};
	template <>
	struct hash<std::pair<uint, PSODescriptor>>
	{
		size_t operator()(const std::pair<uint, PSODescriptor>& key) const
		{
			return key.first ^ key.second.hash;
		}
	};

}

class PSOContainer
{
private:
	struct PSORTSetting
	{
		DXGI_FORMAT depthFormat;
		UINT rtCount;
		DXGI_FORMAT rtFormat[8];
		PSORTSetting() {}
		PSORTSetting(const std::initializer_list<DXGI_FORMAT>& lsts, DXGI_FORMAT depthFormat) noexcept;
		bool operator==(const PSORTSetting& a) const noexcept;
		bool operator != (const PSORTSetting& a) const noexcept
		{
			return !operator==(a);
		}
	};

	struct hash_RTSetting
	{
		size_t operator()(const PSORTSetting& key) const
		{
			std::hash<DXGI_FORMAT> fmtHash;
			size_t h = fmtHash(key.depthFormat);
			for (uint i = 0; i < key.rtCount; ++i)
			{
				h >>= 4;
				h ^= fmtHash(key.rtFormat[i]);
			}
			return h;
		}
	};

	std::unordered_map<std::pair<uint, PSODescriptor>, Microsoft::WRL::ComPtr<ID3D12PipelineState>> allPSOState;
	std::unordered_map<PSORTSetting, uint, hash_RTSetting> rtDict;
	std::vector<PSORTSetting> settings;
public:
	PSOContainer(DXGI_FORMAT depthFormat, UINT rtCount, DXGI_FORMAT* allRTFormat);
	PSOContainer(PSORTSetting* formats, uint formatCount);
	PSOContainer(const std::initializer_list<PSORTSetting>& formats);
	PSOContainer();
	uint GetIndex(const std::initializer_list<DXGI_FORMAT>& lsts, DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN);

	ID3D12PipelineState* GetState(PSODescriptor& desc, ID3D12Device* device, uint index);
};
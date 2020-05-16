#include "PSOContainer.h"
#include "MeshLayout.h"
PSOContainer::PSOContainer(DXGI_FORMAT depthFormat, UINT rtCount, DXGI_FORMAT* allRTFormat) :
	settings(1)
{
	PSORTSetting& set = settings[0];
	set.depthFormat = depthFormat;
	set.rtCount = rtCount;
	memcpy(set.rtFormat, allRTFormat, sizeof(DXGI_FORMAT) * rtCount);
	allPSOState.reserve(23);
	rtDict.reserve(11);
	rtDict.insert(std::pair<PSORTSetting, uint>(set, 0));
}
PSOContainer::PSOContainer(PSORTSetting* formats, uint formatCount) :
	settings(formatCount)
{
	memcpy(settings.data(), formats, sizeof(PSORTSetting) * formatCount);
	allPSOState.reserve(23);
	rtDict.reserve(11);
	settings.reserve(11);
	for (uint i = 0; i < formatCount; ++i)
	{
		rtDict.insert(std::pair<PSORTSetting, uint>(formats[i], i));
	}
}
PSOContainer::PSOContainer(const std::initializer_list<PSORTSetting>& formats) :
	settings(formats.size())
{

	memcpy(settings.data(), formats.begin(), sizeof(PSORTSetting) * formats.size());
	allPSOState.reserve(23);
	rtDict.reserve(11);
	settings.reserve(11);
	const PSORTSetting* rt = formats.begin();
	for (uint i = 0; i < formats.size(); ++i)
	{
		rtDict.insert(std::pair<PSORTSetting, uint>(rt[i], i));
	}
}
PSOContainer::PSOContainer()
{
	rtDict.reserve(11);
	settings.reserve(11);
	allPSOState.reserve(23);
}
uint PSOContainer::GetIndex(const std::initializer_list<DXGI_FORMAT>& lsts, DXGI_FORMAT depthFormat)
{
	PSORTSetting rtSetting = PSORTSetting(lsts, depthFormat);
	auto ite = rtDict.find(rtSetting);
	if (ite != rtDict.end())
	{
		return ite->second;
	}
	size_t count = settings.size();
	rtDict.insert(std::pair<PSORTSetting, uint>(rtSetting, count));
	settings.push_back(rtSetting);
	return count;
}
bool PSODescriptor::operator==(const PSODescriptor& other) const
{
	return other.shaderPtr == shaderPtr && other.shaderPass == shaderPass && other.meshLayoutIndex == meshLayoutIndex;
}

ID3D12PipelineState* PSOContainer::GetState(PSODescriptor& desc, ID3D12Device* device, uint index)
{
	desc.GenerateHash();
	PSORTSetting& set = settings[index];
	auto ite = allPSOState.find(std::pair<uint, PSODescriptor>(index, desc));
	if (ite == allPSOState.end())
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
		ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		std::vector<D3D12_INPUT_ELEMENT_DESC>* inputElement = MeshLayout::GetMeshLayoutValue(desc.meshLayoutIndex);
		opaquePsoDesc.InputLayout = { inputElement->data(), (UINT)inputElement->size() };
		desc.shaderPtr->GetPassPSODesc(desc.shaderPass, &opaquePsoDesc);
		opaquePsoDesc.SampleMask = UINT_MAX;
		opaquePsoDesc.PrimitiveTopologyType = desc.topology;
		opaquePsoDesc.NumRenderTargets = set.rtCount;
		memcpy(&opaquePsoDesc.RTVFormats, set.rtFormat, set.rtCount * sizeof(DXGI_FORMAT));
		opaquePsoDesc.SampleDesc.Count = 1;
		opaquePsoDesc.SampleDesc.Quality = 0;
		opaquePsoDesc.DSVFormat = set.depthFormat;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> result = nullptr;
		HRESULT testResult = device->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(result.GetAddressOf()));
		ThrowIfFailed(testResult);
		allPSOState.insert(std::pair< std::pair<uint, PSODescriptor>, Microsoft::WRL::ComPtr<ID3D12PipelineState>>( std::pair<uint, PSODescriptor>(index, desc), result));
		return result.Get();
	};
	return ite->second.Get();

}

PSOContainer::PSORTSetting::PSORTSetting(const std::initializer_list<DXGI_FORMAT>& lsts, DXGI_FORMAT depthFormat) noexcept
{
	memcpy(rtFormat, lsts.begin(), std::min((size_t)8, lsts.size()) * sizeof(DXGI_FORMAT));
	this->depthFormat = depthFormat;
	rtCount = lsts.size();
}
bool PSOContainer::PSORTSetting::operator==(const PSORTSetting& a) const noexcept
{
	if (depthFormat == a.depthFormat && rtCount == a.rtCount)
	{
		for (uint i = 0; i < rtCount; ++i)
		{
			if (rtFormat[i] != a.rtFormat[i]) return false;
		}
		return true;
	}
	return false;
}
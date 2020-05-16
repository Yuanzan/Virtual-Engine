#include "Mesh.h"
#include "../Singleton/FrameResource.h"
#include "../Singleton/MeshLayout.h"
#include "../Singleton/Graphics.h"
#include "../RenderComponent/RenderCommand.h"
using namespace DirectX;
using Microsoft::WRL::ComPtr;
//CPP
void CreateDefaultBuffer(
	ID3D12Device* device,
	UINT64 byteSize,
	ComPtr<ID3D12Resource>& uploadBuffer,
	ComPtr<ID3D12Resource>& defaultBuffer)
{

	if (!defaultBuffer)
		// Create the actual default buffer resource.
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 
	if (!uploadBuffer)
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(uploadBuffer.GetAddressOf())));
}

void CopyToBuffer(
	UINT64 byteSize,
	ID3D12GraphicsCommandList* cmdList,
	ComPtr<ID3D12Resource>& uploadBuffer,
	ComPtr<ID3D12Resource>& defaultBuffer,
	TransitionBarrierBuffer* barrierBuffer,
	bool bufferReadState,
	uint64_t defaultOffset, uint64_t uploadOffset)
{
	cmdList->CopyBufferRegion(defaultBuffer.Get(), defaultOffset, uploadBuffer.Get(), uploadOffset, byteSize);
}

class MeshLoadCommand : public RenderCommand
{
public:
	ComPtr<ID3D12Resource> uploadResource;
	ComPtr<ID3D12Resource> defaultResource;
	UINT64 byteSize;
	bool bufferIsReadState;
	uint64_t defaultOffset;
	uint64_t uploadOffset;
	MeshLoadCommand(
		ComPtr<ID3D12Resource>& uploadResource,
		ComPtr<ID3D12Resource>& defaultResource,
		UINT64 byteSize,
		bool bufferIsReadState,
		uint64_t defaultOffset,
		uint64_t uploadOffset
	) : byteSize(byteSize), uploadResource(uploadResource), defaultResource(defaultResource), bufferIsReadState(bufferIsReadState),
		defaultOffset(defaultOffset),
		uploadOffset(uploadOffset)
	{}

	virtual void operator()(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource,
		TransitionBarrierBuffer* barrierBuffer)
	{
		resource->ReleaseResourceAfterFlush(uploadResource);
		//TODO
		CopyToBuffer(byteSize, commandList, uploadResource, defaultResource, barrierBuffer, bufferIsReadState, defaultOffset, uploadOffset);
	}
};
size_t Mesh::GetStride(
	bool positions,
	bool normals,
	bool tangents,
	bool colors,
	bool uv,
	bool uv1,
	bool uv2,
	bool uv3,
	bool boneIndex,
	bool boneWeight)
{
	size_t stride = 0;
	auto cumulate = [&](bool ptr, size_t size) -> void
	{
		if (ptr) stride += size;
	};
	cumulate(positions, 12);
	cumulate(normals, 12);
	cumulate(tangents, 16);
	cumulate(colors, 16);
	cumulate(uv, 8);
	cumulate(uv1, 8);
	cumulate(uv2, 8);
	cumulate(uv3, 8);
	cumulate(boneIndex, 16);
	cumulate(boneWeight, 16);
	return stride;
}
size_t Mesh::GetMeshSize(
	uint vertexCount,
	bool positions,
	bool normals,
	bool tangents,
	bool colors,
	bool uv,
	bool uv1,
	bool uv2,
	bool uv3,
	bool boneIndex,
	bool boneWeight,
	uint indexCount,
	DXGI_FORMAT indexFormat)
{
	return GetStride(
		positions,
		normals,
		tangents,
		colors,
		uv,
		uv1,
		uv2,
		uv3,
		boneIndex,
		boneWeight) * vertexCount + indexCount * ((indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4);
}
struct MeshData
{
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;
	std::vector<XMFLOAT3> vertex;
	std::vector<XMFLOAT3> normal;
	std::vector<XMFLOAT4> tangent;
	std::vector<XMFLOAT2> uv;
	std::vector<XMFLOAT2> uv2;
	std::vector<XMFLOAT2> uv3;
	std::vector<XMFLOAT2> uv4;
	std::vector<XMFLOAT4> color;
	std::vector<XMINT4> boneIndex;
	std::vector<XMFLOAT4> boneWeight;
	std::vector<char> indexData;
	XMFLOAT3 boundingCenter;
	XMFLOAT3 boundingExtent;
};

struct MeshLoadData
{
	StackObject<MeshData, true> meshData;
	bool decoded = false;
	size_t offsets = 0;
	void Delete()
	{
		if (decoded)
		{
			meshData.Delete();
			decoded = false;
		}
	}
	MeshLoadData() {}
	MeshLoadData(const MeshLoadData& data) :
		meshData(data.meshData),
		decoded(data.decoded),
		offsets(data.offsets)
	{

	}
	~MeshLoadData()
	{
		Delete();
	}
};
Mesh::Mesh(
	int vertexCount,
	float3* positions,
	float3* normals,
	float4* tangents,
	float4* colors,
	float2* uv,
	float2* uv1,
	float2* uv2,
	float2* uv3,
	int4* boneIndex,
	float4* boneWeight,
	ID3D12Device* device,
	DXGI_FORMAT indexFormat,
	UINT indexCount,
	void* indexArrayPtr,
	const Microsoft::WRL::ComPtr<ID3D12Resource>& defaultBuffer,
	size_t defaultOffset,
	const Microsoft::WRL::ComPtr<ID3D12Resource>& inputUploadBuffer,
	size_t uploadOffset
) : mVertexCount(vertexCount),
indexFormat(indexFormat),
indexCount(indexCount),
indexArrayPtr(indexArrayPtr)
{

	//IndexFormat = indexFormat == DXGI_FORMAT_R16_UINT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	//TODO
	meshLayoutIndex = MeshLayout::GetMeshLayoutIndex(
		normals,
		tangents,
		colors,
		uv,
		uv1,
		uv2,
		uv3,
		boneIndex || boneWeight
	);
	std::vector<D3D12_INPUT_ELEMENT_DESC>* meshLayouts = MeshLayout::GetMeshLayoutValue(meshLayoutIndex);
	size_t stride = GetStride(
		positions,
		normals,
		tangents,
		colors,
		uv,
		uv1,
		uv2,
		uv3,
		boneIndex,
		boneWeight);
	VertexByteStride = stride;
	VertexBufferByteSize = stride * vertexCount;
	//IndexBufferByteSize = (IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4) * indexCount;
	size_t indexSize = indexCount * ((indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4);
	char* dataPtr = reinterpret_cast<char*>(malloc(VertexBufferByteSize + indexSize));
	std::unique_ptr<char> dataPtrGuard(dataPtr);
	auto vertBufferCopy = [&](char* buffer, char* ptr, UINT size, int& offset) -> void
	{
		if ((size_t)ptr < 2048)
		{
			if (ptr) offset += size;
			return;
		}
		for (int i = 0; i < vertexCount; ++i)
		{
			memcpy(buffer + i * stride + offset, ptr + size * i, size);
		}
		offset += size;
	};
	int offset = 0;
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(positions),
		12,
		offset
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(normals),
		12,
		offset
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(tangents),
		16,
		offset
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(colors),
		16,
		offset
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(uv),
		8,
		offset
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(uv1),
		8,
		offset
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(uv2),
		8,
		offset
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(uv3),
		8,
		offset
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(boneIndex),
		16,
		offset
	);
	vertBufferCopy(
		dataPtr,
		reinterpret_cast<char*>(boneWeight),
		16,
		offset
	);
	char* indexBufferStart = dataPtr + VertexBufferByteSize;
	memcpy(indexBufferStart, indexArrayPtr, indexCount * ((indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4));
	if (!defaultBuffer) defaultOffset = 0;
	if (!inputUploadBuffer) uploadOffset = 0;
	ComPtr<ID3D12Resource> uploadBuffer = inputUploadBuffer;
	Resource = defaultBuffer;
	CreateDefaultBuffer(device, indexSize + VertexBufferByteSize, uploadBuffer, Resource);
	char* mappedPtr = nullptr;
	ThrowIfFailed(uploadBuffer->Map(0, nullptr, (void**)&mappedPtr));
	memcpy(mappedPtr + uploadOffset, dataPtr, indexSize + VertexBufferByteSize);
	uploadBuffer->Unmap(0, nullptr);
	MeshLoadCommand* meshLoadCommand = new MeshLoadCommand(
		uploadBuffer,
		Resource,
		indexSize + VertexBufferByteSize,
		defaultBuffer,
		defaultOffset,
		uploadOffset
	);

	RenderCommand::AddLoadingCommand(meshLoadCommand);
	vertexView.BufferLocation = Resource->GetGPUVirtualAddress() + defaultOffset;
	vertexView.StrideInBytes = VertexByteStride;
	vertexView.SizeInBytes = VertexBufferByteSize;
	indexView.BufferLocation = Resource->GetGPUVirtualAddress() + VertexBufferByteSize + defaultOffset;
	indexView.Format = indexFormat;
	indexView.SizeInBytes = indexCount * ((indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4);
}

struct IndexSettings
{
	enum IndexFormat
	{
		IndexFormat_16Bit = 0,
		IndexFormat_32Bit = 1
	};
	IndexFormat indexFormat;
	UINT indexCount;
};
struct MeshHeader
{
	enum MeshDataType
	{
		MeshDataType_Vertex = 0,
		MeshDataType_Index = 1,
		MeshDataType_Normal = 2,
		MeshDataType_Tangent = 3,
		MeshDataType_UV = 4,
		MeshDataType_UV2 = 5,
		MeshDataType_UV3 = 6,
		MeshDataType_UV4 = 7,
		MeshDataType_Color = 8,
		MeshDataType_BoneIndex = 9,
		MeshDataType_BoneWeight = 10,
		MeshDataType_BoundingBox = 11,
		MeshDataType_Num = 12
	};
	MeshDataType type;
	union
	{
		IndexSettings indexSettings;
		UINT vertexCount;
		UINT normalCount;
		UINT tangentCount;
		UINT uvCount;
		UINT colorCount;
		UINT boneCount;
	};
};


bool DecodeMesh(
	const std::string& filePath,
	MeshLoadData& meshLoadData)
{
	std::ifstream ifs(filePath, std::ios::binary);
	if (!ifs)
	{
		return false;//File Read Error!
	}
	UINT chunkCount = 0;
	ifs.read((char*)&chunkCount, sizeof(UINT));
	if (chunkCount >= MeshHeader::MeshDataType_Num) return false;	//Too many types
	auto& meshData = meshLoadData.meshData;
	meshData.New();
	meshLoadData.decoded = true;
	for (UINT i = 0; i < chunkCount; ++i)
	{
		MeshHeader header;
		ifs.read((char*)&header, sizeof(MeshHeader));
		if (header.type >= MeshHeader::MeshDataType_Num) return false;	//Illegal Data Type
		size_t indexSize;
		switch (header.type)
		{
		case MeshHeader::MeshDataType_Vertex:
			meshData->vertex.resize(header.vertexCount);
			ifs.read((char*)meshData->vertex.data(), sizeof(XMFLOAT3) * header.vertexCount);
			break;
		case MeshHeader::MeshDataType_Normal:
			meshData->normal.resize(header.normalCount);
			ifs.read((char*)meshData->normal.data(), sizeof(XMFLOAT3) * header.normalCount);
			break;
		case MeshHeader::MeshDataType_Tangent:
			meshData->tangent.resize(header.tangentCount);
			ifs.read((char*)meshData->tangent.data(), sizeof(XMFLOAT4) * header.tangentCount);
			break;
		case MeshHeader::MeshDataType_UV:
			meshData->uv.resize(header.uvCount);
			ifs.read((char*)meshData->uv.data(), sizeof(XMFLOAT2) * header.uvCount);
			break;
		case MeshHeader::MeshDataType_UV2:
			meshData->uv2.resize(header.uvCount);
			ifs.read((char*)meshData->uv2.data(), sizeof(XMFLOAT2) * header.uvCount);
			break;
		case MeshHeader::MeshDataType_UV3:
			meshData->uv3.resize(header.uvCount);
			ifs.read((char*)meshData->uv3.data(), sizeof(XMFLOAT2) * header.uvCount);
			break;
		case MeshHeader::MeshDataType_UV4:
			meshData->uv4.resize(header.uvCount);
			ifs.read((char*)meshData->uv4.data(), sizeof(XMFLOAT2) * header.uvCount);
			break;
		case MeshHeader::MeshDataType_BoneIndex:
			meshData->boneIndex.resize(header.boneCount);
			ifs.read((char*)meshData->boneIndex.data(), sizeof(XMUINT4) * header.boneCount);
			break;
		case MeshHeader::MeshDataType_BoneWeight:
			meshData->boneWeight.resize(header.boneCount);
			ifs.read((char*)meshData->boneWeight.data(), sizeof(XMFLOAT4) * header.boneCount);
			break;
		case MeshHeader::MeshDataType_Index:
			indexSize = header.indexSettings.indexFormat == IndexSettings::IndexFormat_16Bit ? 2 : 4;
			meshData->indexFormat = indexSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
			meshData->indexData.resize(indexSize * header.indexSettings.indexCount);
			ifs.read(meshData->indexData.data(), meshData->indexData.size());
			break;
		case MeshHeader::MeshDataType_BoundingBox:
			ifs.read((char*)&meshData->boundingCenter, sizeof(XMFLOAT3) * 2);
			break;
		}
	}
	return true;
}

bool DecodeMesh(
	const std::string& filePath,
	MeshData& meshData)
{
	std::ifstream ifs(filePath, std::ios::binary);
	if (!ifs)
	{
		return false;//File Read Error!
	}
	UINT chunkCount = 0;
	ifs.read((char*)&chunkCount, sizeof(UINT));
	if (chunkCount >= MeshHeader::MeshDataType_Num) return false;	//Too many types
	for (UINT i = 0; i < chunkCount; ++i)
	{
		MeshHeader header;
		ifs.read((char*)&header, sizeof(MeshHeader));
		if (header.type >= MeshHeader::MeshDataType_Num) return false;	//Illegal Data Type
		size_t indexSize;
		switch (header.type)
		{
		case MeshHeader::MeshDataType_Vertex:
			meshData.vertex.resize(header.vertexCount);
			ifs.read((char*)meshData.vertex.data(), sizeof(XMFLOAT3) * header.vertexCount);
			break;
		case MeshHeader::MeshDataType_Normal:
			meshData.normal.resize(header.normalCount);
			ifs.read((char*)meshData.normal.data(), sizeof(XMFLOAT3) * header.normalCount);
			break;
		case MeshHeader::MeshDataType_Tangent:
			meshData.tangent.resize(header.tangentCount);
			ifs.read((char*)meshData.tangent.data(), sizeof(XMFLOAT4) * header.tangentCount);
			break;
		case MeshHeader::MeshDataType_UV:
			meshData.uv.resize(header.uvCount);
			ifs.read((char*)meshData.uv.data(), sizeof(XMFLOAT2) * header.uvCount);
			break;
		case MeshHeader::MeshDataType_UV2:
			meshData.uv2.resize(header.uvCount);
			ifs.read((char*)meshData.uv2.data(), sizeof(XMFLOAT2) * header.uvCount);
			break;
		case MeshHeader::MeshDataType_UV3:
			meshData.uv3.resize(header.uvCount);
			ifs.read((char*)meshData.uv3.data(), sizeof(XMFLOAT2) * header.uvCount);
			break;
		case MeshHeader::MeshDataType_UV4:
			meshData.uv4.resize(header.uvCount);
			ifs.read((char*)meshData.uv4.data(), sizeof(XMFLOAT2) * header.uvCount);
			break;
		case MeshHeader::MeshDataType_BoneIndex:
			meshData.boneIndex.resize(header.boneCount);
			ifs.read((char*)meshData.boneIndex.data(), sizeof(XMUINT4) * header.boneCount);
			break;
		case MeshHeader::MeshDataType_BoneWeight:
			meshData.boneWeight.resize(header.boneCount);
			ifs.read((char*)meshData.boneWeight.data(), sizeof(XMFLOAT4) * header.boneCount);
			break;
		case MeshHeader::MeshDataType_Index:
			indexSize = header.indexSettings.indexFormat == IndexSettings::IndexFormat_16Bit ? 2 : 4;
			meshData.indexFormat = indexSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
			meshData.indexData.resize(indexSize * header.indexSettings.indexCount);
			ifs.read(meshData.indexData.data(), meshData.indexData.size());
			break;
		case MeshHeader::MeshDataType_BoundingBox:
			ifs.read((char*)&meshData.boundingCenter, sizeof(XMFLOAT3) * 2);
			break;
		}
	}
	return true;
}

Mesh* Mesh::LoadMeshFromFile(const std::string& str, ID3D12Device* device,
	bool normals,
	bool tangents,
	bool colors,
	bool uv,
	bool uv1,
	bool uv2,
	bool uv3,
	bool bone)
{
	Mesh* result = nullptr;
	MeshData meshData;
	if (!DecodeMesh(str, meshData)) return result;
	result = new Mesh(
		meshData.vertex.size(),
		meshData.vertex.data(),
		normals ? (meshData.normal.data() ? meshData.normal.data() : (XMFLOAT3*)1) : nullptr,
		tangents ? (meshData.tangent.data() ? meshData.tangent.data() : (XMFLOAT4*)1) : nullptr,
		colors ? (meshData.color.data() ? meshData.color.data() : (XMFLOAT4*)1) : nullptr,
		uv ? (meshData.uv.data() ? meshData.uv.data() : (XMFLOAT2*)1) : nullptr,
		uv1 ? (meshData.uv2.data() ? meshData.uv2.data() : (XMFLOAT2*)1) : nullptr,
		uv2 ? (meshData.uv3.data() ? meshData.uv3.data() : (XMFLOAT2*)1) : nullptr,
		uv3 ? (meshData.uv4.data() ? meshData.uv4.data() : (XMFLOAT2*)1) : nullptr,
		bone ? (meshData.boneIndex.data() ? meshData.boneIndex.data() : (int4*)1) : nullptr,
		bone ? (meshData.boneWeight.data() ? meshData.boneWeight.data() : (float4*)1) : nullptr,
		device,
		meshData.indexFormat,
		meshData.indexData.size() / ((meshData.indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4),
		meshData.indexData.data()
	);
	result->boundingCenter = meshData.boundingCenter;
	result->boundingExtent = meshData.boundingExtent;
	return result;

}

void Mesh::LoadMeshFromFiles(const std::vector<std::string>& str, ID3D12Device* device,
	bool normals,
	bool tangents,
	bool colors,
	bool uv,
	bool uv1,
	bool uv2,
	bool uv3,
	bool bone,
	std::vector<ObjectPtr<Mesh>>& results)
{
	results.resize(str.size());
	std::vector<MeshLoadData> meshLoadDatas(str.size());
	size_t bufferSize = 0;
	for (size_t i = 0; i < str.size(); ++i)
	{
		const std::string& s = str[i];
		MeshLoadData& a = meshLoadDatas[i];
		StackObject<MeshData, true>& meshData = a.meshData;
		if (DecodeMesh(s, meshLoadDatas[i]))
		{
			a.offsets = bufferSize;
			bufferSize += GetMeshSize(
				meshData->vertex.size(),
				meshData->vertex.data(),
				normals ? (meshData->normal.data() ? meshData->normal.data() : (XMFLOAT3*)1) : nullptr,
				tangents ? (meshData->tangent.data() ? meshData->tangent.data() : (XMFLOAT4*)1) : nullptr,
				colors ? (meshData->color.data() ? meshData->color.data() : (XMFLOAT4*)1) : nullptr,
				uv ? (meshData->uv.data() ? meshData->uv.data() : (XMFLOAT2*)1) : nullptr,
				uv1 ? (meshData->uv2.data() ? meshData->uv2.data() : (XMFLOAT2*)1) : nullptr,
				uv2 ? (meshData->uv3.data() ? meshData->uv3.data() : (XMFLOAT2*)1) : nullptr,
				uv3 ? (meshData->uv4.data() ? meshData->uv4.data() : (XMFLOAT2*)1) : nullptr,
				bone ? (meshData->boneIndex.data() ? meshData->boneIndex.data() : (int4*)1) : nullptr,
				bone ? (meshData->boneWeight.data() ? meshData->boneWeight.data() : (float4*)1) : nullptr,
				meshData->indexData.size() / ((meshData->indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4),
				meshData->indexFormat);
		}
		else
		{
			meshData.Delete();
		}
	}
	if (bufferSize == 0) return;
	ComPtr<ID3D12Resource> defaultBuffer;
	ComPtr<ID3D12Resource> uploadBuffer;
	// Create the actual default buffer resource.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())));
	for (size_t i = 0; i < meshLoadDatas.size(); ++i)
	{
		MeshLoadData& a = meshLoadDatas[i];
		if (a.decoded)
		{
			StackObject<MeshData, true>& meshData = a.meshData;
			Mesh* result = new Mesh(
				meshData->vertex.size(),
				meshData->vertex.data(),
				normals ? (meshData->normal.data() ? meshData->normal.data() : (XMFLOAT3*)1) : nullptr,
				tangents ? (meshData->tangent.data() ? meshData->tangent.data() : (XMFLOAT4*)1) : nullptr,
				colors ? (meshData->color.data() ? meshData->color.data() : (XMFLOAT4*)1) : nullptr,
				uv ? (meshData->uv.data() ? meshData->uv.data() : (XMFLOAT2*)1) : nullptr,
				uv1 ? (meshData->uv2.data() ? meshData->uv2.data() : (XMFLOAT2*)1) : nullptr,
				uv2 ? (meshData->uv3.data() ? meshData->uv3.data() : (XMFLOAT2*)1) : nullptr,
				uv3 ? (meshData->uv4.data() ? meshData->uv4.data() : (XMFLOAT2*)1) : nullptr,
				bone ? (meshData->boneIndex.data() ? meshData->boneIndex.data() : (int4*)1) : nullptr,
				bone ? (meshData->boneWeight.data() ? meshData->boneWeight.data() : (float4*)1) : nullptr,
				device,
				meshData->indexFormat,
				meshData->indexData.size() / ((meshData->indexFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4),
				meshData->indexData.data(),
				defaultBuffer,
				a.offsets,
				uploadBuffer,
				a.offsets
			);
			result->boundingCenter = meshData->boundingCenter;
			result->boundingExtent = meshData->boundingExtent;
			results[i] = ObjectPtr<Mesh>::MakePtr(result);
		}
		else
			results[i] = nullptr;
	}
}
#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/MetaLib.h"
#include "../Common/BitArray.h"
#include "../Common/RandomVector.h"
#include "../RenderComponent/Mesh.h"
class PSOContainer;
class Shader;
class RenderTexture;
class UploadBuffer;
class DescriptorHeap;
enum BackBufferState
{
	BackBufferState_Present = 0,
	BackBufferState_RenderTarget = 1
};

struct RenderTarget
{
	RenderTexture* rt;
	uint depthSlice;
	uint mipCount;
	RenderTarget(RenderTexture* rt = nullptr, uint depthSlice = 0, uint mipCount = 0) :
		rt(rt),
		depthSlice(depthSlice),
		mipCount(mipCount)
	{}
};
class TransitionBarrierBuffer;
class Graphics
{
private:
	static std::mutex mtx;
	static StackObject<Mesh, true> fullScreenMesh;
	static ObjectPtr<DescriptorHeap> globalDescriptorHeap;
	static BitArray usedDescs;
	static std::vector<UINT> unusedDescs;
	static ObjectPtr<Mesh> cubeMesh;
	static ObjectPtr<Mesh> sphereMesh;
public:
	static inline constexpr DescriptorHeap const* GetGlobalDescHeap()
	{
		return globalDescriptorHeap.operator->();
	}
	static Mesh const* GetFullScreenMesh()
	{
		return fullScreenMesh;
	}
	static void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	static void Blit(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		D3D12_CPU_DESCRIPTOR_HANDLE* renderTarget,
		UINT renderTargetCount,
		D3D12_CPU_DESCRIPTOR_HANDLE* depthTarget,
		PSOContainer* container, uint containerIndex,
		UINT width, UINT height,
		const Shader* shader, UINT pass);
	static void Blit(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		const std::initializer_list<RenderTarget>& renderTarget,
		const RenderTarget& depthTarget,
		PSOContainer* container, uint containerIndex,
		const Shader* shader, UINT pass);
	/*
	template <uint count>
	inline constexpr static void UAVBarriers(
		ID3D12GraphicsCommandList* commandList,
		const std::initializer_list<ID3D12Resource*>& resources)
	{
		D3D12_RESOURCE_BARRIER barrier[count];
		ID3D12Resource* const* ptr = resources.begin();
		auto func = [&](uint i)->void
		{
			barrier[i].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier[i].UAV = { ptr[i] };
		};
		InnerLoop<decltype(func), count>(func);
		commandList->ResourceBarrier(count, barrier);
	}*/
	static void CopyTexture(
		ID3D12GraphicsCommandList* commandList,
		RenderTexture* source, UINT sourceSlice, UINT sourceMipLevel,
		RenderTexture* dest, UINT destSlice, UINT destMipLevel);

	static void CopyBufferToTexture(
		ID3D12GraphicsCommandList* commandList,
		UploadBuffer* sourceBuffer, size_t sourceBufferOffset,
		ID3D12Resource* textureResource, UINT targetMip,
		UINT width, UINT height, UINT depth, DXGI_FORMAT targetFormat, UINT pixelSize, TransitionBarrierBuffer* barrierBuffer);

	static void CopyBufferToBC5Texture(
		ID3D12GraphicsCommandList* commandList,
		UploadBuffer* sourceBuffer, size_t sourceBufferOffset,
		ID3D12Resource* textureResource, UINT targetMip,
		UINT width, UINT height, UINT depth, DXGI_FORMAT targetFormat, UINT pixelSize, TransitionBarrierBuffer* barrierBuffer);

	static void CopyBufferToTexture(
		ID3D12GraphicsCommandList* commandList,
		UploadBuffer* sourceBuffer, size_t sourceBufferOffset,
		ID3D12Resource* textureResource, UINT targetMip,
		UINT width, UINT height, UINT depth, DXGI_FORMAT targetFormat, UINT pixelSize);
	static void CopyBufferToBCTexture(
		ID3D12GraphicsCommandList* commandList,
		UploadBuffer* sourceBuffer, size_t sourceBufferOffset,
		ID3D12Resource* textureResource, UINT targetMip,
		UINT width, UINT height, UINT depth, DXGI_FORMAT targetFormat, UINT pixelSize);
	static void DrawMesh(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		const Mesh* mesh,
		const Shader* shader, uint pass,
		PSOContainer* psoContainer, uint psoIndex, uint instanceCount = 1
	);
	static void SetRenderTarget(
		ID3D12GraphicsCommandList* commandList,
		RenderTexture*const * renderTargets,
		uint rtCount,
		RenderTexture* depthTex = nullptr);
	static void SetRenderTarget(
		ID3D12GraphicsCommandList* commandList,
		const std::initializer_list<RenderTexture*>& renderTargets,
		RenderTexture* depthTex = nullptr);
	static void SetRenderTarget(
		ID3D12GraphicsCommandList* commandList,
		const RenderTarget* renderTargets,
		uint rtCount,
		const RenderTarget& depth);
	static void SetRenderTarget(
		ID3D12GraphicsCommandList* commandList,
		const std::initializer_list<RenderTarget>& init,
		const RenderTarget& depth);
	static void SetRenderTarget(
		ID3D12GraphicsCommandList* commandList,
		const RenderTarget* renderTargets,
		uint rtCount);
	static void SetRenderTarget(
		ID3D12GraphicsCommandList* commandList,
		const std::initializer_list<RenderTarget>& init);
	static void DrawMeshes(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		Mesh** mesh, uint meshCount,
		const Shader* shader, uint pass,
		PSOContainer* psoContainer, uint psoIndex, bool sort
	);
	static UINT GetDescHeapIndexFromPool();
	static void ReturnDescHeapIndexToPool(UINT targetIndex);
	static void ForceCollectAllHeapIndex();
	static Mesh const* GetCubeMesh() { return cubeMesh; }
	static Mesh const* GetSphereMesh() { return sphereMesh; }
};
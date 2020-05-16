#pragma once
#include "TempRTAllocator.h"
#include "../Common/d3dUtil.h"
#include <unordered_map>
#include "../Common/MetaLib.h"
#include "../JobSystem/JobSystem.h"
#include "CommandBuffer.h"
class FrameResource;
class Camera;
class World;
class PipelineComponent;
struct RenderPipelineData
{
	ID3D12Device* device;
	ID3D12CommandQueue* commandQueue;
	ID3D12Resource* backBufferResource;
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle;
	FrameResource* lastResource;
	FrameResource* resource;
	std::vector<ObjectPtr<Camera>>* allCameras;
	ID3D12Fence* fence;
	UINT64* fenceIndex;
	World* world;
	float time;
	float deltaTime;
	uint ringFrameIndex;
	float3 worldMovingDir;
	bool isMovingTheWorld;
};
class RenderPipeline final
{
private:
	static RenderPipeline* current;
	struct RenderTextureMark
	{
		UINT id;
		UINT rtIndex;
		ResourceDescriptor desc;
		UINT startComponent;
		UINT endComponent;
	};
	UINT initCount = 0;
	std::vector<PipelineComponent*> components;
	TempRTAllocator tempRTAllocator;
	static std::unordered_map<Type, PipelineComponent*> componentsLink;
	std::vector<std::vector<PipelineComponent*>> renderPathComponents;
	Dictionary<UINT, RenderTextureMark> renderTextureMarks;
	void mInit(PipelineComponent* ptr, const std::type_info& tp);

	template <typename ... Args>
	void Init()
	{
		char c[] = { (mInit(new Args(), typeid(Args)), 0)... };
	}
	RenderPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
public:
	static RenderPipeline* GetInstance(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	static void DestroyInstance();
	~RenderPipeline();
	void PrepareRendering(RenderPipelineData& data, JobSystem* jobSys, std::vector <JobBucket*>& bucketArray) noexcept;
	HRESULT ExecuteRendering(RenderPipelineData& data, IDXGISwapChain* swapChain) noexcept;

	
	template <typename T>
	static T* GetComponent()
	{
		auto ite = componentsLink.find(typeid(T));
		if (ite != componentsLink.end())
			return (T*)ite->second;
		else return nullptr;
	}
};
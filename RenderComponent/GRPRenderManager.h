#pragma once
#include "../Common/MObject.h"
#include "Utility/CommandSignature.h"
#include "../Common/MetaLib.h"
#include "CBufferPool.h"
#include <mutex>
#include "Utility/CullingMask.h"
class DescriptorHeap;
class Mesh;
class Transform;
class FrameResource;
class ComputeShader;
class PSOContainer;
class StructuredBuffer;
class RenderTexture;
class JobBucket;
class JobHandle;
class TransitionBarrierBuffer;

class GRPRenderManager final
{
public:
	struct CullData
	{
		float4x4 _LastVPMatrix;
		float4x4 _VPMatrix;
		float4 planes[6];
		//Align
		float3 _FrustumMinPoint;
		uint _Count;
		//Align
		float3 _FrustumMaxPoint;
		uint _CullingMask;
	};
	struct RenderElement
	{
		ObjectPtr<Transform> transform;
		float3 boundingCenter;
		float3 boundingExtent;
		RenderElement(
			const ObjectPtr<Transform>& anotherTrans,
			float3 boundingCenter,
			float3 boundingExtent
		);
	};
private:
	CommandSignature cmdSig;
	const Shader* shader;
	std::unique_ptr<StructuredBuffer> cullResultBuffer;
	std::unique_ptr<StructuredBuffer> dispatchIndirectBuffer;;
	std::vector<RenderElement> elements;
	std::unordered_map<Transform*, uint> dicts;
	uint meshLayoutIndex;
	uint capacity;
	uint _InputBuffer;
	uint _InputDataBuffer;
	uint _OutputBuffer;
	uint _InputIndexBuffer;
	uint _DispatchIndirectBuffer;
	uint _HizDepthTex;
	uint _CountBuffer;
	uint CullBuffer;
	const ComputeShader* cullShader;
	CBufferPool objectPool;
public:
	GRPRenderManager(
		uint meshLayoutIndex,
		uint initCapacity,
		const Shader* shader,
		ID3D12Device* device
	);
	~GRPRenderManager();
	void AddRenderElement(
		ID3D12Device* device,
		const ObjectPtr<Transform>& targetTrans,
		const ObjectPtr<Mesh>& mesh,
		int shaderID,
		int materialID,
		int lightmapID,
		uint cullingMask
	);
	inline const Shader* GetShader() const { return shader; }

	void RemoveElement(Transform* trans, ID3D12Device* device);
	void UpdateRenderer(Transform* targetTrans, const ObjectPtr<Mesh>& mesh, ID3D12Device* device);
	CommandSignature* GetCmdSignature() { return &cmdSig; }
	void UpdateFrame(FrameResource*, ID3D12Device*);//Should be called Per frame
	void UpdateTransform(
		const ObjectPtr<Transform>& targetTrans,
		ID3D12Device* device,
		int shaderID,
		int materialID,
		int lightmapID,
		uint cullingMask);
	void OcclusionRecheck(
		ID3D12GraphicsCommandList* commandList,
		TransitionBarrierBuffer& barrierBuffer,
		ID3D12Device* device,
		FrameResource* targetResource,
		const ConstBufferElement& cullDataBuffer,
		RenderTexture* hizDepth);
	void Culling(
		ID3D12GraphicsCommandList* commandList,
		TransitionBarrierBuffer& barrierBuffer,
		ID3D12Device* device,
		FrameResource* targetResource,
		const ConstBufferElement& cullDataBuffer,
		uint cullingMask,
		float4* frustumPlanes,
		float3 frustumMinPoint,
		float3 frustumMaxPoint,
		const float4x4* vpMatrix,			//HI-Z Occ
		const float4x4* lastVPMatrix,		//HI-Z Occ,
		RenderTexture* hizDepth,			//HI-Z Occ
		bool occlusion						//HI-Z Occ
	);
	void DrawCommand(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Device* device,
		uint targetShaderPass,
		PSOContainer* container, uint containerIndex
	);

	void SetWorldMoving(
		ID3D12Device* device,
		const double3& movingDir);

	void MoveTheWorld(
		ID3D12Device* device,
		FrameResource* resource,
		JobBucket* bucket);
};

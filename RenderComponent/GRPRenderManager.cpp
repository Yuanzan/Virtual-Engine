#include "GRPRenderManager.h"
#include "DescriptorHeap.h"
#include "Mesh.h"
#include "../LogicComponent/Transform.h"
#include "../Singleton/ShaderCompiler.h"
#include "ComputeShader.h"
#include "../Singleton/FrameResource.h"
#include "StructuredBuffer.h"
#include "../Common/MetaLib.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/PSOContainer.h"
#include <mutex>
#include "../PipelineComponent/IPipelineResource.h"
#include "RenderTexture.h"
#include "../Singleton/Graphics.h"
#include "../LogicComponent/World.h"
#include "../Common/Input.h"
#include "../JobSystem/JobInclude.h"
using namespace Microsoft::WRL;
using namespace Math;


struct ObjectCullData
{
	float4x3 localToWorld;
	float3 boundingCenter;
	float3 boundingExtent;
	uint maskID;
};



void GetFloat4x3(const float4x4& f, float4x3& result)
{
	memcpy(&result._11, &f._11, sizeof(float3));
	memcpy(&result._21, &f._21, sizeof(float3));
	memcpy(&result._31, &f._31, sizeof(float3));
	memcpy(&result._41, &f._41, sizeof(float3));
}

void GetFloat4x4(const float4x3& f, float4x4& result)
{
	memcpy(&result._11, &f._11, sizeof(float3));
	memcpy(&result._21, &f._21, sizeof(float3));
	memcpy(&result._31, &f._31, sizeof(float3));
	memcpy(&result._41, &f._41, sizeof(float3));
	result._14 = 0;
	result._24 = 0;
	result._34 = 0;
	result._44 = 1;
}
void GetMultiDrawCommand(MultiDrawCommand& cmd, D3D12_GPU_VIRTUAL_ADDRESS cbufferAddress, const ObjWeakPtr<Mesh>& meshPtr)
{
	if (!meshPtr) return;
	Mesh* mesh = meshPtr;
	cmd.objectCBufferAddress = cbufferAddress;
	cmd.vertexBuffer = mesh->VertexBufferView();
	cmd.indexBuffer = mesh->IndexBufferView();
	cmd.drawArgs.BaseVertexLocation = 0;
	cmd.drawArgs.IndexCountPerInstance = mesh->GetIndexCount();
	cmd.drawArgs.InstanceCount = 1;
	cmd.drawArgs.StartIndexLocation = 0;
	cmd.drawArgs.StartInstanceLocation = 0;
}
void GetObjectCullData(ObjectCullData& objData, const ObjWeakPtr<Transform>& transPtr, const ObjWeakPtr<Mesh>& meshPtr, uint cullingMask)
{
	if (!transPtr || !meshPtr) return;
	Transform* trans = transPtr;
	Mesh* mesh = meshPtr;
	objData.boundingCenter = mesh->boundingCenter;
	objData.boundingExtent = mesh->boundingExtent;
	objData.maskID = cullingMask;
	float4x4 localToWorld = trans->GetLocalToWorldMatrixGPU();
	GetFloat4x3(localToWorld, objData.localToWorld);
}
struct Command
{
	enum CommandType
	{
		CommandType_Add,
		CommandType_Remove,
		CommandType_TransformPos,
		CommandType_Renderer
	};
	CommandType type;
	uint index;
	ObjWeakPtr<Transform> trans;
	ObjWeakPtr<Mesh> mesh;
	int2 id;
	int lightmapID;
	Command() {}
	Command(const Command& cmd)
	{
		memcpy(this, &cmd, sizeof(Command));
	}
};
class GpuDrivenRenderer : public IPipelineResource
{
private:
	std::mutex mtx;

	CBufferPool* objectPool;
	uint capacity;
	uint count;
	std::vector<Command> allCommands;
	std::vector<ConstBufferElement> allocatedObjects;
public:
	std::unique_ptr<UploadBuffer> objectPosBuffer;
	std::unique_ptr<UploadBuffer> cmdDrawBuffers;
	bool isWorldMoving = false;
	double3 movingDir;
	void AddCommand(const Command& c)
	{
		std::lock_guard<std::mutex> lck(mtx);
		allCommands.push_back(c);
	}
	GpuDrivenRenderer(
		CBufferPool* pool,
		ID3D12Device* device,
		uint capacity
	) : capacity(capacity), count(0),
		movingDir(0, 0, 0),
		objectPool(pool),
		objectPosBuffer(new UploadBuffer(device, capacity, false, sizeof(ObjectCullData))),
		cmdDrawBuffers(new UploadBuffer(device, capacity, false, sizeof(MultiDrawCommand)))
	{
		allocatedObjects.reserve(capacity);
		allCommands.reserve(50);
	}
	void MoveTheWorld(
		JobBucket* bucket)
	{
		//TODO
		//Move The World!
		//loop allocatedObjects Did
		//loop objectPosBuffer
		if (isWorldMoving)
		{
			isWorldMoving = false;
			auto moveDirection = movingDir;
			movingDir = { 0,0,0 };
			const uint len = 1000;
			uint taskCount = count / len + 1;
			for (uint taskIndex = 0; taskIndex < taskCount; ++taskIndex)
			{
				uint startCount = taskIndex * len;
				uint endCount = (taskIndex + 1) * len;
				bool isLast = (taskIndex == taskCount - 1);
				bucket->GetTask(nullptr, 0, [&, moveDirection, startCount, endCount, isLast]()->void
					{
						uint endCountMin = isLast ? count : endCount;
						for (uint i = startCount; i < endCountMin; ++i)
						{
							ConstBufferElement& ite = allocatedObjects[i];
							ObjectConstants* mappedPtr = (ObjectConstants*)ite.buffer->GetMappedDataPtr(ite.element);
							float4x4 twoMatrix[2];
							memcpy(twoMatrix, &mappedPtr->lastObjectToWorld, sizeof(float4x4) * 2);
							twoMatrix[0].m[3][0] += moveDirection.x;
							twoMatrix[0].m[3][1] += moveDirection.y;
							twoMatrix[0].m[3][2] += moveDirection.z;
							twoMatrix[1].m[3][0] += moveDirection.x;
							twoMatrix[1].m[3][1] += moveDirection.y;
							twoMatrix[1].m[3][2] += moveDirection.z;
							memcpy(&mappedPtr->lastObjectToWorld, twoMatrix, sizeof(float4x4) * 2);
						}
					});
				bucket->GetTask(nullptr, 0, [&, moveDirection, startCount, endCount, isLast]()->void
					{
						uint endCountMin = isLast ? count : endCount;
						for (uint i = startCount; i < endCountMin; ++i)
						{
							ObjectCullData* mappedPtr = (ObjectCullData*)objectPosBuffer->GetMappedDataPtr(i);
							float4x3 mat;
							memcpy(&mat, &mappedPtr->localToWorld, sizeof(float4x3));
							mat.m[3][0] += moveDirection.x;
							mat.m[3][1] += moveDirection.y;
							mat.m[3][2] += moveDirection.z;
							memcpy(&mappedPtr->localToWorld, &mat, sizeof(float4x3));
						}
					});
			}

		}
	}

	void UpdateFrame(
		ID3D12Device* device,
		FrameResource* resource
	)
	{
		std::lock_guard<std::mutex> lck(mtx);
		for (uint i = 0; i < allCommands.size(); ++i)
		{
			Command& c = allCommands[i];
			std::vector<ConstBufferElement>::iterator ite;// = allocatedObjects.end() - 1;
			uint last = count - 1;
			switch (c.type)
			{
			case Command::CommandType_Add:
			{
				ConstBufferElement obj = objectPool->Get(device);
				ObjectCullData objData;
				GetObjectCullData(objData, c.trans, c.mesh, c.index);
				MultiDrawCommand cmd;
				GetMultiDrawCommand(cmd, obj.buffer->GetAddress(obj.element), c.mesh);
				allocatedObjects.push_back(obj);
				ObjectConstants mappedPtr;
				mappedPtr.id = c.id;
				mappedPtr.lightmapID = c.lightmapID;
				GetFloat4x4(objData.localToWorld, mappedPtr.lastObjectToWorld);
				mappedPtr.objectToWorld = mappedPtr.lastObjectToWorld;
				obj.buffer->CopyData(obj.element, &mappedPtr);
				objectPosBuffer->CopyData(count, &objData);
				cmdDrawBuffers->CopyData(count, &cmd);
				count++;
			}
			break;
			case Command::CommandType_Remove:
			{
				if (c.index != last)
				{
					objectPosBuffer->CopyDataInside(last, c.index);
					cmdDrawBuffers->CopyDataInside(last, c.index);
				}
				ConstBufferElement& cEle = allocatedObjects[c.index];
				objectPool->Return(cEle);
				ite = allocatedObjects.end() - 1;
				cEle = *ite;
				allocatedObjects.erase(ite);
				count--;
			}
			break;
			case Command::CommandType::CommandType_TransformPos:
			{
				ConstBufferElement& objEle = allocatedObjects[c.index];
				ObjectConstants* mappedPtr = (ObjectConstants*)objEle.buffer->GetMappedDataPtr(objEle.element);
				ObjectConstants data;
				memcpy(&data, mappedPtr, sizeof(ObjectConstants));
				data.lastObjectToWorld = data.objectToWorld;
				ObjectCullData objData;
				GetObjectCullData(objData, c.trans, c.mesh, c.index);
				GetFloat4x4(objData.localToWorld, data.objectToWorld);
				data.id = c.id;
				data.lightmapID = c.lightmapID;
				memcpy(mappedPtr, &data, sizeof(ObjectConstants));
				objectPosBuffer->CopyData(c.index, &objData);
			}
			break;
			case Command::CommandType::CommandType_Renderer:
			{
				ConstBufferElement& cEle = allocatedObjects[c.index];
				MultiDrawCommand cmd;
				GetMultiDrawCommand(cmd, cEle.buffer->GetAddress(cEle.element), c.mesh);
				cmdDrawBuffers->CopyData(c.index, &cmd);
			}
			break;
			}
		}
		allCommands.clear();
	}

	~GpuDrivenRenderer()
	{
	}
};

GRPRenderManager::GRPRenderManager(
	uint meshLayoutIndex,
	uint initCapacity,
	const Shader* shader,
	ID3D12Device* device
) :
	capacity(initCapacity),
	shader(shader),
	cmdSig(shader, device),
	meshLayoutIndex(meshLayoutIndex),
	objectPool(sizeof(ObjectConstants), initCapacity)
{
	cullShader = ShaderCompiler::GetComputeShader("Cull");
	StructuredBufferElement ele[3];
	ele[0].elementCount = capacity;
	ele[0].stride = sizeof(MultiDrawCommand);
	ele[1].elementCount = 1;
	ele[1].stride = sizeof(uint);
	ele[2].elementCount = capacity;
	ele[2].stride = sizeof(uint);
	cullResultBuffer = std::unique_ptr<StructuredBuffer>(new StructuredBuffer(
		device,
		ele,
		_countof(ele),
		true
	));
	ele[0].elementCount = 4;
	ele[0].stride = sizeof(uint);
	dispatchIndirectBuffer = std::unique_ptr<StructuredBuffer>(new StructuredBuffer(
		device,
		ele,
		1,
		false
	));


	_InputBuffer = ShaderID::PropertyToID("_InputBuffer");
	_InputDataBuffer = ShaderID::PropertyToID("_InputDataBuffer");
	_OutputBuffer = ShaderID::PropertyToID("_OutputBuffer");
	_CountBuffer = ShaderID::PropertyToID("_CountBuffer");
	CullBuffer = ShaderID::PropertyToID("CullBuffer");
	_InputIndexBuffer = ShaderID::PropertyToID("_InputIndexBuffer");
	_DispatchIndirectBuffer = ShaderID::PropertyToID("_DispatchIndirectBuffer");
	_HizDepthTex = ShaderID::PropertyToID("_HizDepthTex");
}

void GRPRenderManager::SetWorldMoving(
	ID3D12Device* device,
	const double3& movingDir)
{
	for (auto ite = FrameResource::mFrameResources.begin(); ite != FrameResource::mFrameResources.end(); ++ite)
	{
		if (*ite)
		{
			GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)(*ite)->GetResource(this, [=]()->GpuDrivenRenderer*
				{
					return new GpuDrivenRenderer(&objectPool, device, capacity);
				});
			perFrameData->isWorldMoving = true;
			perFrameData->movingDir += movingDir;
		}
	}
}

void GRPRenderManager::MoveTheWorld(
	ID3D12Device* device,
	FrameResource* resource,
	JobBucket* bucket)
{

	GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)resource->GetResource(this, [=]()->GpuDrivenRenderer*
		{
			return new GpuDrivenRenderer(&objectPool, device, capacity);
		});
	perFrameData->MoveTheWorld(bucket);
}

GRPRenderManager::RenderElement::RenderElement(
	const ObjectPtr<Transform>& anotherTrans,
	float3 boundingCenter,
	float3 boundingExtent
) : transform(anotherTrans), boundingCenter(boundingCenter), boundingExtent(boundingExtent) {}
void GRPRenderManager::AddRenderElement(
	ID3D12Device* device,
	const ObjectPtr<Transform>& targetTrans,
	const ObjectPtr<Mesh>& mesh,
	int shaderID,
	int materialID,
	int lightmapID,
	uint cullingMask
)
{
	if (mesh->GetLayoutIndex() != meshLayoutIndex)
#if defined(DEBUG) || defined(_DEBUG)
		throw "Mesh Bad Layout!";
#else
		return;
#endif
	if (elements.size() + 1 >= capacity)
	{
#if defined(DEBUG) || defined(_DEBUG)
		throw "Out of Range!";
#else
		return;
#endif
	}
	auto ite = dicts.find(targetTrans);
	if (ite != dicts.end())
	{
		return;
	}
	dicts.insert(std::pair<Transform*, uint>(targetTrans, elements.size()));

	elements.emplace_back(
		targetTrans,
		mesh->boundingCenter,
		mesh->boundingExtent
	);

	Command cmd;
	cmd.type = Command::CommandType_Add;
	cmd.trans = targetTrans;
	cmd.mesh = mesh;
	cmd.id = { shaderID,materialID };
	cmd.lightmapID = lightmapID;
	cmd.index = cullingMask;
	for (uint i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)FrameResource::mFrameResources[i]->GetResource(this, [=]()->GpuDrivenRenderer*
			{
				return new GpuDrivenRenderer(&objectPool, device, capacity);
			});
		perFrameData->AddCommand(cmd);
	}
}

void GRPRenderManager::UpdateRenderer(Transform* targetTrans, const ObjectPtr<Mesh>& mesh, ID3D12Device* device)
{
	auto ite = dicts.find(targetTrans);
	if (ite == dicts.end()) return;
	RenderElement& rem = elements[ite->second];

	Command cmd;
	cmd.type = Command::CommandType_Renderer;
	cmd.index = ite->second;
	cmd.mesh = mesh;
	for (uint i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)FrameResource::mFrameResources[i]->GetResource(this, [=]()->GpuDrivenRenderer*
			{
				return new GpuDrivenRenderer(&objectPool, device, capacity);
			});
		perFrameData->AddCommand(cmd);
	}
}

void GRPRenderManager::RemoveElement(Transform* trans, ID3D12Device* device)
{
	auto ite = dicts.find(trans);
	if (ite == dicts.end()) return;
	auto arrEnd = elements.end() - 1;
	uint index = ite->second;
	if (index != elements.size() - 1)
	{
		RenderElement& ele = elements[index];
		ele = *arrEnd;
	}
	dicts.insert_or_assign(arrEnd->transform.operator->(), ite->second);
	elements.erase(arrEnd);
	dicts.erase(ite);

	Command cmd;
	cmd.type = Command::CommandType_Remove;
	cmd.index = index;
	for (uint i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)FrameResource::mFrameResources[i]->GetResource(this, [=]()->GpuDrivenRenderer*
			{
				return new GpuDrivenRenderer(&objectPool, device, capacity);
			});
		perFrameData->AddCommand(cmd);
	}

}

void GRPRenderManager::UpdateFrame(FrameResource* resource, ID3D12Device* device)
{
	GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)resource->GetResource(this, [=]()->GpuDrivenRenderer*
		{
			return new GpuDrivenRenderer(&objectPool, device, capacity);
		});
	perFrameData->UpdateFrame(device, resource);
}

void GRPRenderManager::UpdateTransform(
	const ObjectPtr<Transform>& targetTrans,
	ID3D12Device* device,
	int shaderID,
	int materialID,
	int lightmapID,
	uint cullingMask)
{
	auto ite = dicts.find(targetTrans);
	if (ite == dicts.end()) return;
	RenderElement& rem = elements[ite->second];
	Command cmd;
	cmd.index = ite->second;
	cmd.type = Command::CommandType_TransformPos;
	cmd.trans = targetTrans;
	cmd.id = { shaderID, materialID };
	cmd.lightmapID = lightmapID;
	for (uint i = 0, size = FrameResource::mFrameResources.size(); i < size; ++i)
	{
		GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)FrameResource::mFrameResources[i]->GetResource(this, [=]()->GpuDrivenRenderer*
			{
				return new GpuDrivenRenderer(&objectPool, device, capacity);
			});
		perFrameData->AddCommand(cmd);
	}
}

void GRPRenderManager::OcclusionRecheck(
	ID3D12GraphicsCommandList* commandList,
	TransitionBarrierBuffer& barrierBuffer,
	ID3D12Device* device,
	FrameResource* targetResource,
	const ConstBufferElement& cullDataBuffer,
	RenderTexture* hizDepth)
{
	if (elements.empty()) return;
	GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)targetResource->GetResource(this, [=]()->GpuDrivenRenderer*
		{
			return new GpuDrivenRenderer(&objectPool, device, capacity);
		});
	const DescriptorHeap* heap = Graphics::GetGlobalDescHeap();
	cullShader->BindRootSignature(commandList, heap);
	cullShader->SetResource(commandList, _InputBuffer, perFrameData->cmdDrawBuffers.get(), 0);
	cullShader->SetResource(commandList, _InputDataBuffer, perFrameData->objectPosBuffer.get(), 0);
	cullShader->SetStructuredBufferByAddress(commandList, _OutputBuffer, cullResultBuffer->GetAddress(0, 0));
	cullShader->SetStructuredBufferByAddress(commandList, _CountBuffer, cullResultBuffer->GetAddress(1, 0));
	cullShader->SetResource(commandList, CullBuffer, cullDataBuffer.buffer, cullDataBuffer.element);
	cullShader->SetStructuredBufferByAddress(commandList, _InputIndexBuffer, cullResultBuffer->GetAddress(2, 0));
	cullShader->SetStructuredBufferByAddress(commandList, _DispatchIndirectBuffer, dispatchIndirectBuffer->GetAddress(0, 0));
	cullShader->SetResource(commandList, _HizDepthTex, heap, hizDepth->GetGlobalDescIndex());
	cullShader->Dispatch(commandList, 4, 1, 1, 1);
	barrierBuffer.AddCommand(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, dispatchIndirectBuffer->GetResource());
	barrierBuffer.AddCommand(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, cullResultBuffer->GetResource());
	barrierBuffer.ExecuteCommand(commandList);

	cullShader->DispatchIndirect(commandList, 5, dispatchIndirectBuffer.get(), 0, 0);
	barrierBuffer.AddCommand(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, dispatchIndirectBuffer->GetResource());
	barrierBuffer.AddCommand(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, cullResultBuffer->GetResource());
}
void GRPRenderManager::Culling(
	ID3D12GraphicsCommandList* commandList,
	TransitionBarrierBuffer& barrierBuffer,
	ID3D12Device* device,
	FrameResource* targetResource,
	const ConstBufferElement& cullDataBuffer,
	uint cullingMask,
	float4* frustumPlanes,
	float3 frustumMinPoint,
	float3 frustumMaxPoint,
	const float4x4* vpMatrix,
	const float4x4* lastVPMatrix,
	RenderTexture* hizDepth,
	bool occlusion)
{
	if (elements.empty()) return;
	uint dispatchCount = (63 + elements.size()) / 64;
	uint capacity = this->capacity;
	GpuDrivenRenderer* perFrameData = (GpuDrivenRenderer*)targetResource->GetResource(this, [=]()->GpuDrivenRenderer*
		{
			return new GpuDrivenRenderer(&objectPool, device, capacity);
		});
	CullData& cullD = *(CullData*)cullDataBuffer.buffer->GetMappedDataPtr(cullDataBuffer.element);
	memcpy(cullD.planes, frustumPlanes, sizeof(float4) * 6);
	memcpy(&cullD._FrustumMaxPoint, &frustumMaxPoint, sizeof(float3));
	memcpy(&cullD._FrustumMinPoint, &frustumMinPoint, sizeof(float3));
	cullD._Count = elements.size();
	cullD._CullingMask = cullingMask;
	const DescriptorHeap* heap = Graphics::GetGlobalDescHeap();
	cullShader->BindRootSignature(commandList, heap);
	cullShader->SetResource(commandList, _InputBuffer, perFrameData->cmdDrawBuffers.get(), 0);
	cullShader->SetResource(commandList, _InputDataBuffer, perFrameData->objectPosBuffer.get(), 0);
	cullShader->SetStructuredBufferByAddress(commandList, _OutputBuffer, cullResultBuffer->GetAddress(0, 0));
	cullShader->SetStructuredBufferByAddress(commandList, _CountBuffer, cullResultBuffer->GetAddress(1, 0));
	cullShader->SetResource(commandList, CullBuffer, cullDataBuffer.buffer, cullDataBuffer.element);
	if (occlusion)
	{
		memcpy(&cullD._LastVPMatrix, lastVPMatrix, sizeof(float4x4));
		memcpy(&cullD._VPMatrix, vpMatrix, sizeof(float4x4));
		cullShader->SetStructuredBufferByAddress(commandList, _InputIndexBuffer, cullResultBuffer->GetAddress(2, 0));
		cullShader->SetStructuredBufferByAddress(commandList, _DispatchIndirectBuffer, dispatchIndirectBuffer->GetAddress(0, 0));
		cullShader->SetResource(commandList, _HizDepthTex, heap, hizDepth->GetGlobalDescIndex());
		barrierBuffer.AddCommand(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, cullResultBuffer->GetResource());
		barrierBuffer.ExecuteCommand(commandList);
		cullShader->Dispatch(commandList, 3, 1, 1, 1);
		barrierBuffer.UAVBarriers(
			{
			cullResultBuffer->GetResource(),
			dispatchIndirectBuffer->GetResource() }
		);
		barrierBuffer.ExecuteCommand(commandList);
		cullShader->Dispatch(commandList, 2, dispatchCount, 1, 1);
		barrierBuffer.UAVBarrier(dispatchIndirectBuffer->GetResource());
		barrierBuffer.AddCommand(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, cullResultBuffer->GetResource());
	}
	else
	{
		barrierBuffer.AddCommand(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, cullResultBuffer->GetResource());
		barrierBuffer.ExecuteCommand(commandList);
		cullShader->Dispatch(commandList, 1, 1, 1, 1);
		barrierBuffer.UAVBarrier(cullResultBuffer->GetResource());
		barrierBuffer.ExecuteCommand(commandList);
		cullShader->Dispatch(commandList, 0, dispatchCount, 1, 1);
		barrierBuffer.AddCommand(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, cullResultBuffer->GetResource());
	}
}
void  GRPRenderManager::DrawCommand(
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	uint targetShaderPass,
	PSOContainer* container, uint containerIndex
)
{
	if (elements.empty()) return;
	//if (!Input::IsKeyPressed(KeyCode::Space))

	PSODescriptor desc;
	desc.meshLayoutIndex = meshLayoutIndex;
	desc.shaderPass = targetShaderPass;
	desc.shaderPtr = shader;
	//desc.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	ID3D12PipelineState* pso = container->GetState(desc, device, containerIndex);
	commandList->SetPipelineState(pso);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->ExecuteIndirect(
		cmdSig.GetSignature(),
		elements.size(),
		cullResultBuffer->GetResource(),
		cullResultBuffer->GetAddressOffset(0, 0),
		cullResultBuffer->GetResource(),
		cullResultBuffer->GetAddressOffset(1, 0)
	);

}


GRPRenderManager::~GRPRenderManager()
{
	FrameResource::DisposeResources(this);
}

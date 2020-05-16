#pragma once
//#include "../RenderComponent/MeshRenderer.h"
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/BitArray.h"
#include "../Common/RandomVector.h"
#include <mutex>
class PBRMaterialManager;

class FrameResource;
class GameTimer;
class Transform;
class DescriptorHeap;
class Mesh;
class GRPRenderManager;
class MeshRenderer;
class JobBucket;
class Texture;
class Camera;
class Light;
class JobHandle;
class Skybox;

//Only For Test!
class World final
{
	friend class Transform;
private:
	World(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device);
	static World* current;
	std::mutex mtx;
	GRPRenderManager* grpRenderer;
	PBRMaterialManager* grpMaterialManager;
	RandomVector<ObjectPtr<Transform>> allTransformsPtr;
	std::vector<ObjectPtr<Camera>> allCameras;
	int3 blockIndex = { 0,0,0 };
public:
	int3 GetBlockIndex() const { return blockIndex; }
	inline static const double BLOCK_SIZE = 512;
	//ObjectPtr<MeshRenderer> testMeshRenderer;
	std::wstring windowInfo;
	ObjectPtr<Light> testLight;
	ObjectPtr<Skybox> currentSkybox;
	GRPRenderManager* GetGRPRenderManager() const
	{
		return grpRenderer;
	}
	PBRMaterialManager* GetPBRMaterialManager() const
	{
		return grpMaterialManager;
	}
	std::vector<ObjectPtr<Camera>>& GetCameras()
	{
		return allCameras;
	}
	~World();
	UINT windowWidth;
	UINT windowHeight;
	inline static constexpr World* GetInstance() { return current; }
	inline static constexpr World* CreateInstance(ID3D12GraphicsCommandList* cmdList, ID3D12Device* device)
	{
		if (current)
			return current;
		new World(cmdList, device);
		return current;
	}
	inline static constexpr void DestroyInstance()
	{
		auto a = current;
		current = nullptr;
		if (a) delete a;
	}
	void Update(FrameResource* resource, ID3D12Device* device, GameTimer& timer, int2 screenSize, const Math::Vector3& moveDir, bool isMovingWorld);
	void PrepareUpdateJob(
		JobBucket* bucket,
		FrameResource* resource,
		ID3D12Device* device,
		GameTimer& timer,
		int2 screenSize,
		Math::Vector3& moveDir,
		bool& isMovingWorld,
		std::vector<JobHandle>& mainDependedTasks);
};
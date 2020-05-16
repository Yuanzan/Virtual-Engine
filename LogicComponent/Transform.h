#pragma once
#include <DirectXMath.h>
#include "../Common/MObject.h"
#include "../LogicComponent/Component.h"
#include "../Common/RandomVector.h"
class World;
class AssetReference;
class JobHandle;
class JobBucket;
class TransformMoveStruct;

namespace neb
{
	class CJsonObject;
}
struct TransformData
{
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 forward;
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 localScale;
	DirectX::XMFLOAT3 position;
	bool isWorldMovable;
};
class Transform : public MObject
{
	friend class Scene;
	friend class Component;
	friend class AssetReference;
	friend class TransformMoveStruct;
	friend class World;
private:
	static std::mutex transMutex;
	static RandomVector<TransformData> randVec;
	static std::vector<JobHandle> moveWorldHandles;
	RandomVector<ObjectPtr<Component>> allComponents;
	int worldIndex;
	uint vectorPos;
	Transform(const neb::CJsonObject& path, ObjectPtr<Transform>& targetPtr, bool isWorldMovable = true);
	Transform(ObjectPtr<Transform>& targetPtr, bool isWorldMovable = true);
	void Dispose();
	static void MoveTheWorld(int3* worldBlockIndexPtr, const int3& moveBlock, const double3& moveDirection, JobBucket* bucket, std::vector<JobHandle>& jobHandles);
public:
	uint GetComponentCount() const
	{
		return allComponents.Length();
	}
	static ObjectPtr<Transform> GetTransform();
	static ObjectPtr<Transform> GetTransform(const neb::CJsonObject& path);
	DirectX::XMFLOAT3 GetPosition() const { return randVec[vectorPos].position; }
	DirectX::XMFLOAT3 GetForward() const { return randVec[vectorPos].forward; }
	DirectX::XMFLOAT3 GetRight() const { return randVec[vectorPos].right; }
	DirectX::XMFLOAT3 GetUp() const { return randVec[vectorPos].up; }
	DirectX::XMFLOAT3 GetLocalScale() const { return randVec[vectorPos].localScale; }
	void SetRight(const Math::Vector3& right);
	void SetUp(const Math::Vector3& up);
	void SetForward(const Math::Vector3& forward);
	void SetRotation(const Math::Vector4& quaternion);
	void SetPosition(const DirectX::XMFLOAT3& position);
	void SetLocalScale(const DirectX::XMFLOAT3& localScale);
	Math::Matrix4 GetLocalToWorldMatrixCPU() const;
	Math::Matrix4 GetLocalToWorldMatrixGPU() const;
	Math::Matrix4 GetWorldToLocalMatrixCPU() const;
	Math::Matrix4 GetWorldToLocalMatrixGPU() const;
	~Transform();
	static void DisposeTransform(ObjectPtr<Transform>& trans);
	
};

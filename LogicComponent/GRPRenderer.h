#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "Component.h"
#include "../RenderComponent/Utility/CullingMask.h"
class PBRMaterial;
class Mesh;
class Scene;
class Transform;
namespace neb
{
	class CJsonObject;
}
class GRPRenderer : public Component
{
	friend class Scene;
	ObjectPtr<Mesh> mesh;
	ObjectPtr<PBRMaterial> material;
	ID3D12Device* device;
	bool loaded = false;
	GRPRenderer(
		const ObjectPtr<Transform>& trans, ObjectPtr<Component>& ptr);
	~GRPRenderer();
public:
	CullingMask cullingMask;
	static ObjectPtr<GRPRenderer> GetRenderer(ID3D12Device* device,
		const ObjectPtr<Transform>& trans,
		const ObjectPtr<Mesh>& mesh,
		const ObjectPtr<PBRMaterial>& mat, CullingMask cullingMask);
};
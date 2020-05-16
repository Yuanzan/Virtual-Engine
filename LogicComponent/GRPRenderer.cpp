#include "GRPRenderer.h"
#include "../RenderComponent/Mesh.h"
#include "Transform.h"
#include "../RenderComponent/PBRMaterial.h"
#include "../CJsonObject/CJsonObject.hpp"
#include "../RenderComponent/GRPRenderManager.h"
#include "../ResourceManagement/AssetDatabase.h"
#include "World.h"
GRPRenderer::GRPRenderer(
	const ObjectPtr<Transform>& trans, ObjectPtr<Component>& ptr) :
	Component(trans, ptr)
{

}
GRPRenderer::~GRPRenderer()
{
	World* world = World::GetInstance();
	if (world && loaded && GetTransformPtr())
	{
		GRPRenderManager* rManager = world->GetGRPRenderManager();
		if (rManager)
			rManager->RemoveElement(GetTransform(), device);
	}
}
ObjectPtr<GRPRenderer> GRPRenderer::GetRenderer(ID3D12Device* device, const ObjectPtr<Transform>& trans, const ObjectPtr<Mesh>& mesh, const ObjectPtr<PBRMaterial>& mat,
	CullingMask cullingMask)
{
	ObjectPtr<Component> comp;
	auto ptr = new GRPRenderer(trans, comp);
	ptr->mesh = mesh;
	ptr->cullingMask = cullingMask;
	ptr->device = device;
	ptr->material = mat;
	World* world = World::GetInstance();
	if (ptr->mesh && ptr->material && world)
	{
		ptr->loaded = true;
		GRPRenderManager* rManager = world->GetGRPRenderManager();
		rManager->AddRenderElement(device, ptr->GetTransformPtr(), ptr->mesh, 0, ptr->material->GetMaterialIndex(), 0, (uint)cullingMask);
	}
	return comp.CastTo<GRPRenderer>();
}
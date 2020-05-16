#include "Scene.h"
#include "../CJsonObject/CJsonObject.hpp"
#include "../LogicComponent/World.h"
#include "../LogicComponent/Transform.h"
#include "../LogicComponent/GRPRenderer.h"
#include "../RenderComponent/Mesh.h"
#include "../RenderComponent/PBRMaterial.h"
#include "../RenderComponent/Light.h"
#include "../RenderComponent/ReflectionProbe.h"
#include "AssetReference.h"
#include "AssetDatabase.h"
using namespace neb;
Scene::Scene(const std::string& guid, CJsonObject& jsonObj, ID3D12Device* device) : 
	guid(guid)
{
	loadedTransforms.reserve(50);
	std::string key;
	CJsonObject gameObjectJson;
	CJsonObject transformObjJson;
	CJsonObject rendererObjJson;
	std::string str;
	std::string meshGuid, pbrMatGuid;
	while (jsonObj.GetKey(key))
	{
		if (jsonObj.Get(key, gameObjectJson))//Get Per GameObject
		{
			if (gameObjectJson.Get("transform", transformObjJson)) // Get Transform
			{
				ObjectPtr<Transform> tr = Transform::GetTransform(transformObjJson);
				loadedTransforms.push_back(tr);
				if (gameObjectJson.Get("renderer", rendererObjJson))
				{
					rendererObjJson.Get("mesh", meshGuid);
					rendererObjJson.Get("material", pbrMatGuid);
					CullingMask cullingMask = CullingMask::ALL;
					rendererObjJson.Get("mask", (uint&)cullingMask);
					auto mesh = AssetReference::SyncLoad(device, meshGuid, AssetLoadType::Mesh).CastTo<Mesh>();
					auto mat = AssetReference::SyncLoad(device, pbrMatGuid, AssetLoadType::GPURPMaterial).CastTo<PBRMaterial>();
					if (mesh && mat)
					{
						GRPRenderer::GetRenderer(device, tr, mesh, mat, cullingMask);
					}
				}
				if (gameObjectJson.Get("light", rendererObjJson))
				{
					ObjectPtr<Light> lgt = Light::CreateLight(tr);
					lgt->GetDataFromJson(device, rendererObjJson);
				}
				if (gameObjectJson.Get("rp", rendererObjJson))
				{
					ReflectionProbe::GetNewProbe(device, tr, rendererObjJson);
				}
			}
		}
	}
}
Scene::~Scene()
{
	for (auto ite = loadedTransforms.begin(); ite != loadedTransforms.end(); ++ite)
	{
		(*ite)->Dispose();
		ite->Destroy();
	}
}

void Scene::DestroyScene()
{
	AssetDatabase::GetInstance()->AsyncLoad(AssetReference(AssetLoadType::Scene, guid, nullptr, false));
}
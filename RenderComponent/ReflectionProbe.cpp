#include "ReflectionProbe.h"
#include "../CJsonObject/CJsonObject.hpp"
#include "../LogicComponent/Transform.h"
#include "../Singleton/MathLib.h"
#include "../ResourceManagement/AssetDatabase.h"
#include "../ResourceManagement/AssetReference.h"
using namespace Math;
using namespace neb;
RandomVector<ReflectionProbe*> ReflectionProbe::allProbes(50);
std::mutex ReflectionProbe::mtx;
ReflectionProbe::ReflectionProbe(const ObjectPtr<Transform>& trans, ObjectPtr<Component>& ptr, const ObjectPtr<ITexture>& cubemap) :
	Component(trans, ptr),
	cubemap(cubemap)
{
	std::lock_guard<std::mutex> lck(mtx);
	allProbes.Add(this, &idx);
}

void ReflectionProbe::GetAllReflectionProbes(Math::Vector4* frustumPlanes, const Math::Vector3& frustumMinPoint, const Math::Vector3& frustumMaxPoint, std::vector<ReflectionProbe*>& probes)
{
	probes.clear();
	for (uint i = 0; i < allProbes.Length(); ++i)
	{
		auto p = allProbes[i];
		if (p->Avaliable())
		{
			Vector3 pos = (Vector3)mul(p->GetTransform()->GetLocalToWorldMatrixCPU(), Vector4(p->localPosition, 1));
			Vector3 extent = p->localExtent;
			extent += p->blendDistance;
			extent *= 0.5;
			bool3 inRange = (frustumMinPoint < (pos + extent)) && (frustumMaxPoint > (pos - extent));
			if (inRange.x && inRange.y && inRange.z && MathLib::BoxIntersect(pos, extent, frustumPlanes, 6))
			{
				probes.push_back(p);
			}
		}
	}
	if (probes.size() > 1)
	{
		auto compFunc = [](ReflectionProbe* a, ReflectionProbe* b)->int
		{
			return a->importance - b->importance;
		};
		QuicksortStackCustomCompare<ReflectionProbe*, decltype(compFunc)>(probes.data(), compFunc, 0, probes.size() - 1);
	}
}

ReflectionProbe::ReflectionProbe(ID3D12Device* device, const ObjectPtr<Transform>& trans, ObjectPtr<Component>& ptr, CJsonObject& cjson) :
	Component(trans, ptr)
{
	{
		std::lock_guard<std::mutex> lck(mtx);
		allProbes.Add(this, &idx);
	}
#define DEFINE_STR(x) static std::string x##_str = #x
	DEFINE_STR(blendDistance);
	DEFINE_STR(color);
	DEFINE_STR(localPosition);
	DEFINE_STR(boxProjection);
	DEFINE_STR(localExtent);
	DEFINE_STR(cubemap);
	cjson.Get(blendDistance_str, blendDistance);
	std::string s;
	if (cjson.Get(color_str, s))
	{
		ReadStringToVector<float4>(s.data(), s.length(), &color);
	}
	if (cjson.Get(localPosition_str, s))
	{
		ReadStringToVector<float3>(s.data(), s.length(), &localPosition);
	}
	if (cjson.Get(localExtent_str, s))
	{
		ReadStringToVector<float3>(s.data(), s.length(), &localExtent);
	}
	int i = 0;
	cjson.Get(boxProjection_str, i);
	boxProjection = i;
	if (cjson.Get(cubemap_str, s))
	{
		cubemap = AssetReference::SyncLoad(device, s, AssetLoadType::Texture2D).CastTo<ITexture>();
	}
#undef DEFINE_STR
}

ObjectPtr<ReflectionProbe> ReflectionProbe::GetNewProbe(ID3D12Device* device, const ObjectPtr<Transform>& trans, neb::CJsonObject& cjson)
{
	ObjectPtr<Component> ptr;
	new ReflectionProbe(device, trans, ptr, cjson);
	return ptr.CastTo<ReflectionProbe>();
}

ObjectPtr<ReflectionProbe> ReflectionProbe::GetNewProbe(const ObjectPtr<Transform>& trans, const ObjectPtr<ITexture>& cubemap)
{
	ObjectPtr<Component> ptr;
	new ReflectionProbe(trans, ptr, cubemap);
	return ptr.CastTo<ReflectionProbe>();
}

void ReflectionProbe::GetReflectionData(ReflectionData& result) const
{
	result.position = (Vector3)mul(GetTransform()->GetLocalToWorldMatrixCPU(), Vector4(localPosition, 1));
	result.blendDistance = blendDistance;
	Vector3 extent = localExtent;
	result.minExtent = (extent - blendDistance) * 0.5;
	result.maxExtent = (extent + blendDistance) * 0.5;
	result.hdr = color;
	result.boxProjection = boxProjection ? 1 : 0;
	result.texIndex = cubemap->GetGlobalDescIndex();
}

ReflectionProbe::~ReflectionProbe()
{
	{
		std::lock_guard<std::mutex> lck(mtx);
		allProbes.Remove(idx);
	}
}
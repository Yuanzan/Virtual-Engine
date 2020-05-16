#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/MetaLib.h"
#include "../LogicComponent/Component.h"
#include "ITexture.h"
#include "../Common/RandomVector.h"
#include "ReflectionData.h"
namespace neb { class CJsonObject; }

class ReflectionProbe final : public Component
{
private:
	ReflectionProbe(const ObjectPtr<Transform>& trans, ObjectPtr<Component>& ptr, const ObjectPtr<ITexture>& cubemap);
	ReflectionProbe(ID3D12Device* device, const ObjectPtr<Transform>& trans, ObjectPtr<Component>& ptr, neb::CJsonObject& cjson);
	ObjectPtr<ITexture> cubemap;
	static RandomVector<ReflectionProbe*> allProbes;
	static std::mutex mtx;
	uint idx;
	
public:
	static ObjectPtr<ReflectionProbe> GetNewProbe(const ObjectPtr<Transform>& trans, const ObjectPtr<ITexture>& cubemap);
	static ObjectPtr<ReflectionProbe> GetNewProbe(ID3D12Device* device, const ObjectPtr<Transform>& trans, neb::CJsonObject& cubemap);
	static void GetAllReflectionProbes(Math::Vector4* frustumPlanes, const Math::Vector3& frustumMinPoint, const Math::Vector3& frustumMaxPoint, std::vector<ReflectionProbe*>& probes);
	float blendDistance = 0.5;
	void GetReflectionData(ReflectionData& result) const;
	bool Avaliable() const { return cubemap; }
	float4 color = { 1,1,1,1 };
	float3 localPosition = { 0,0,0 };
	float3 localExtent = { 5,5,5 };
	int32_t importance = 0;
	bool boxProjection = false;
	ITexture* GetCubemap() const { return cubemap; }
	void SetCubemap(const ObjectPtr<ITexture>& rt)
	{
		if (rt->GetDimension() != TextureDimension::Cubemap)
			cubemap = nullptr;
		else
			cubemap = rt;
	}

	~ReflectionProbe();
};
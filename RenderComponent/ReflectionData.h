#pragma once
#include "../Common/d3dUtil.h"
struct ReflectionData
{
	float3 position;
	float blendDistance;
	float3 minExtent;
	uint boxProjection;
	float4 hdr;
	float3 maxExtent;
	uint texIndex;
};
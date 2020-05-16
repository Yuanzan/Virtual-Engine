#include "Include/Sampler.cginc"
#define GBUFFER_SHADER
#define VT_COUNT 2
struct VTIndices
{
    int indices[VT_COUNT];
};
struct VTResult
{
    float4 results[VT_COUNT];
};
Texture2D<float4> _MainTex[2] : register(t0, space0);
Texture2D<float> _GreyTex[2] : register(t0, space1);
Texture2D<uint> _IntegerTex[2] : register(t0, space2);
TextureCube<float4> _Cubemap[2] : register(t0, space3);
TextureCube<float> _GreyCubemap[2] : register(t0, space4);
Texture2D<float4> _VirtualTex[2] : register(t0, space5);
Texture2D<uint4> _IndirectTex : register(t0, space6);
StructuredBuffer<VTIndices> _IndirectBuffer : register(t1, space6);
#include "include/VirtualSampler.cginc"
cbuffer Per_Object_Buffer : register(b0)
{
    float4x4 _LocalToWorld;
    int2 _CurrentChunkOffset;
    float _VirtualTextureChunkSize;
};

cbuffer Per_Camera_Buffer : register(b1)
{
    float4x4 _WorldToCamera;
    float4x4 _InverseWorldToCamera;
    float4x4 _Proj;
    float4x4 _InvProj;
    float4x4 _VP;
    float4x4 _InvVP;
    float4x4 _NonJitterVP;
    float4x4 _NonJitterInverseVP;
    float4x4 _LastVP;
    float4x4 _InverseLastVP;

	float4x4 _FlipProj;
	float4x4 _FlipInvProj;
	float4x4 _FlipVP;
	float4x4 _FlipInvVP;
	float4x4 _FlipNonJitterVP;
	float4x4 _FlipNonJitterInverseVP;
	float4x4 _FlipLastVP;
	float4x4 _FlipInverseLastVP;

	float4 _ZBufferParams;
	float4 _RandomSeed;
    float3 worldSpaceCameraPos;
    float _NearZ;
    float _FarZ;
};

cbuffer LightCullCBuffer : register(b2)
{
	float4 _CameraNearPos;
	float4 _CameraFarPos;
	float3 _CameraForward;
	uint _LightCount;
	float3 _SunColor;
	uint _SunEnabled;
	float3 _SunDir;
	uint _SunShadowEnabled;
	uint4 _ShadowmapIndices;
	float4 _CascadeDistance;
	float4x4 _ShadowMatrix[4];
	float4 _ShadowSoftValue;
	float4 _ShadowOffset;
	uint _ReflectionProbeCount;
};

cbuffer TextureIndices : register(b3)
{
	uint _SkyboxTex;
	uint _PreintTexture;
};

cbuffer ProjectionShadowParams : register(b4)
{
	float4x4 _ShadowmapVP;
	float4 _LightPos;
};
cbuffer VirtualTextureParams : register(b5)
{
    float2 _ChunkTexelSize;
    uint2 _IndirectTexelSize;
    uint _MaxMipLevel;
};

cbuffer HeightMapParams : register(b6)
{
    float2 _HeightmapUVScale;
    float2 _HeightmapUVOffset;
	float2 _HeightScaleOffset;
	uint _HeightmapChunkSize;
};

#include "Include/Lighting.cginc"
#include "Include/Random.cginc"

struct SurfaceOutput
{
	float3 albedo;
	float3 specular;
	float smoothness;
	float3 emission;
	float3 normal;
	float occlusion;
};
struct ChunkData
{
	float4 texelSize;
	int texIndex;
	int normalIndex;
};

StructuredBuffer<LightCommand> _AllLight : register(t2, space6);
StructuredBuffer<uint> _LightIndexBuffer : register(t3, space6);
StructuredBuffer<ChunkData> _HeightMapDefaultBuffer : register(t4, space6);

#include "Include/TerrainHeightMap.cginc"
#include "Include/SunShadow.cginc"

struct VertexIn
{
	float3 PosL    : POSITION;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : TEXCOORD0;
    float4 lastProjPos : TEXCOORD1;
    float4 currentProjPos : TEXCOORD2;
    float3 PosL    : TEXCOORD3;
};

inline float LinearEyeDepth( float z )
{
    return 1.0 / (_ZBufferParams.z * z + _ZBufferParams.w);
}

inline float Linear01Depth( float z)
{
    return 1.0 / (_ZBufferParams.x * z + _ZBufferParams.y);
}

SurfaceOutput Surface(float2 uv)
{
	SurfaceOutput o;
	o.albedo = 0.8;//
	o.specular = 0.2;
	o.occlusion = 1;
	o.smoothness = 0.7;
	o.emission = 0;
	o.normal = float3(0,0,1);
    uv /= _VirtualTextureChunkSize;
	float2 chunkFloat =  _CurrentChunkOffset + uv;
    int2 chunk = chunkFloat >= 0 ? (int2)chunkFloat : (int2)chunkFloat - 1;
    float2 localUV = frac(uv);
	VTResult result = SampleTrilinear(chunk, localUV, _ChunkTexelSize, _MaxMipLevel, _IndirectTexelSize);
//	o.emission = result.results[0];//float3( GetVirtualTextureDebugUV(chunk, localUV, _IndirectTexelSize), 0);

/*	uv = uv * uvScale + uvOffset;
	o.albedo = albedo * ((albedoTexIndex >= 0) ? _MainTex[albedoTexIndex].Sample(anisotropicWrapSampler, uv).xyz : 1);
	float3 smo = (specularTexIndex >= 0) ? _MainTex[specularTexIndex].Sample(anisotropicWrapSampler, uv).xyz : 1;
	float metallic = smo.y * metallic;
	o.specular = lerp(0.04, o.albedo, metallic);
	o.albedo = lerp(o.albedo, 0, metallic);
	o.occlusion = occlusion * smo.z;
	o.smoothness = smoothness * smo.x;
	o.emission = emission * ((emissionTexIndex >= 0) ? _MainTex[emissionTexIndex].Sample(anisotropicWrapSampler, uv).xyz : 1);
	o.normal = float3(0, 1, 0);//TODO : Read Normal*/
	return o;
}

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    float4 posW = float4(mul(_LocalToWorld, float4(vin.PosL, 1.0f)).xyz, 1);
	posW.y += GetHeightMap(vin.PosL.xz);
    vout.PosW = posW.xyz;
    // Transform to homogeneous clip space.
    vout.PosL = vin.PosL;
    vout.PosH = mul(_VP, posW);
    vout.currentProjPos = mul(_NonJitterVP, posW);
    vout.lastProjPos = mul(_LastVP, posW);
    return vout;
}

void PS(VertexOut i, 
        out float4 albedo : SV_TARGET0,
        out float4 specular : SV_TARGET1,
        out float4 normalTex : SV_TARGET2,
        out float2 motionVectorTex : SV_TARGET3,
        out float4 emissionRT : SV_TARGET4)
{
	SurfaceOutput surf = Surface(i.PosL.xz);
	float roughness = 1 - surf.smoothness;
   float2 lastScreenUV = (i.lastProjPos.xy / i.lastProjPos.w) * float2(0.5, 0.5) + 0.5;
   float2 screenUV = (i.currentProjPos.xy / i.currentProjPos.w) * float2(0.5, 0.5) + 0.5;
   motionVectorTex = screenUV - lastScreenUV;
   motionVectorTex.y = -motionVectorTex.y;
   float3 viewDir = normalize(worldSpaceCameraPos - i.PosW.xyz);
   float3 normal = SampleTerrainNormal(i.PosL.xz);
   normal.z = -normal.z;
   float3 tangent = float3(1, 0, 0);
   float3 binormal = normalize(cross(normal, tangent));
   tangent = normalize(cross(binormal, normal));
    float linearEyeDepth = LinearEyeDepth(i.PosH.z);
    float3 refl = reflect(-viewDir, normal);
			BSDFContext context = (BSDFContext)0;
			float3 sunColor = clamp(CalculateSunLight_NoShadow(
				normal,
				viewDir,
				-_SunDir,
				_SunColor,
				surf.albedo,
				surf.specular,
				roughness,
				context
			), 0, 32768);
			float2 preintAB = _MainTex[_PreintTexture].SampleLevel(bilinearClampSampler, float2(roughness, context.NoV), 0).rg;
			float3 EnergyCompensation;
			float3 preint = PreintegratedDGF_LUT(preintAB, EnergyCompensation, surf.specular);
			preint *= EnergyCompensation;
			float sunShadow = GetShadow(i.PosW, linearEyeDepth, dot(normal, -_SunDir), _ShadowmapIndices);
   			
   			
			   
			emissionRT = float4(sunColor * sunShadow + surf.emission, 1);
			emissionRT.xyz += clamp(CalculateLocalLight(
				screenUV,
				i.PosW.xyz,
				linearEyeDepth,
				normal,
				viewDir,
				_CameraNearPos.w,
				_CameraFarPos.w,
				surf.albedo,
				surf.specular,
				roughness,
				_LightIndexBuffer,
				_AllLight
			), 0, 32768);
			albedo = float4(surf.albedo, surf.occlusion);
			normalTex = float4(normal * 0.5 + 0.5, 1);
   			specular = float4(preint, surf.smoothness);
			emissionRT =  KillNaN(emissionRT);
		}

float4 VS_Depth(float3 position : POSITION) : SV_POSITION
{
    float4 posW = mul(_LocalToWorld, float4(position, 1));
	posW.y = GetHeightMap(position.xz);
    return mul(_VP, posW);
}

void PS_Depth(){}

float4 VS_Shadowmap(float3 position : POSITION) : SV_POSITION
{
	float4 posW = mul(_LocalToWorld, float4(position, 1));
	posW.y = GetHeightMap(position.xz);
	return mul(_ShadowmapVP, posW);
}

void PS_Shadowmap(){}
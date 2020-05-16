#include "Include/Sampler.cginc"
#define GBUFFER_SHADER
Texture2D<float4> _MainTex[2] : register(t0, space0);
Texture2D<float> _GreyTex[2] : register(t0, space1);
Texture2D<uint> _IntegerTex[2] : register(t0, space2);
TextureCube<float4> _Cubemap[2] : register(t0, space3);
TextureCube<float> _GreyCubemap[2] : register(t0, space5);
cbuffer Per_Object_Buffer : register(b0)
{
    float4x4 _LastLocalToWorld;
    float4x4 _LocalToWorld;
    int2 _ID;   //shaderid, materialid
	int _LightmapID;
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

StructuredBuffer<LightCommand> _AllLight : register(t0, space4);
StructuredBuffer<uint> _LightIndexBuffer : register(t1, space4);

struct StandardPBRMaterial
{
	float2 uvScale;
	float2 uvOffset;
	//align
	float3 albedo;
	float metallic;
	float3 emission;
	float smoothness;
	//align
	float occlusion;
	int albedoTexIndex;
	int specularTexIndex;
	int normalTexIndex;
	//align
	int emissionTexIndex;
};

StructuredBuffer<StandardPBRMaterial> _DefaultMaterials : register(t2, space4);
inline float LinearEyeDepth( float z )
{
    return 1.0 / (_ZBufferParams.z * z + _ZBufferParams.w);
}
#include "Include/SunShadow.cginc"
struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 uv    : TEXCOORD;
    float4 tangent : TANGENT;
    float2 uv2     : TEXCOORD1;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
    float4 tangent : TANGENT;
	float4 uv      : TEXCOORD;
    float4 lastProjPos : TEXCOORD1;
    float4 currentProjPos : TEXCOORD2;
};

inline float Linear01Depth( float z)
{
    return 1.0 / (_ZBufferParams.x * z + _ZBufferParams.y);
}

SurfaceOutput Surface(float2 uv)
{
	SurfaceOutput o;
	StandardPBRMaterial mat = _DefaultMaterials[_ID.y];
	uv = uv * mat.uvScale + mat.uvOffset;
	o.albedo = mat.albedo * ((mat.albedoTexIndex >= 0) ? _MainTex[mat.albedoTexIndex].Sample(anisotropicWrapSampler, uv).xyz : 1);
	float3 smo = (mat.specularTexIndex >= 0) ? _MainTex[mat.specularTexIndex].Sample(anisotropicWrapSampler, uv).xyz : 1;
	float metallic = smo.y * mat.metallic;
	o.specular = lerp(0.04, o.albedo, metallic);
	o.albedo = lerp(o.albedo, 0, metallic);
	o.occlusion = mat.occlusion * smo.z;
	o.smoothness = mat.smoothness * smo.x;
	o.emission = mat.emission * ((mat.emissionTexIndex >= 0) ? _MainTex[mat.emissionTexIndex].Sample(anisotropicWrapSampler, uv).xyz : 1);
	o.normal = float3(0, 1, 0);//TODO : Read Normal
	return o;
}

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    float4 posW = mul(_LocalToWorld, float4(vin.PosL, 1.0f));
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul((float3x3)_LocalToWorld, vin.NormalL);

    // Transform to homogeneous clip space.
    vout.PosH = mul(_VP, posW);
    vout.currentProjPos = mul(_NonJitterVP, posW);
    vout.lastProjPos = mul(_LastVP, mul(_LastLocalToWorld, float4(vin.PosL, 1.0f)));
    vout.uv.xy = float4(vin.uv.xy, vin.uv2.xy);

    vout.tangent.xyz = mul((float3x3)_LocalToWorld, vin.tangent.xyz);
    vout.tangent.w = vin.tangent.w;
    return vout;
}

void PS(VertexOut i, 
        out float4 albedo : SV_TARGET0,
        out float4 specular : SV_TARGET1,
        out float4 normalTex : SV_TARGET2,
        out float2 motionVectorTex : SV_TARGET3,
        out float4 emissionRT : SV_TARGET4)
{

	SurfaceOutput surf = Surface(i.uv.xy);
	float roughness = 1 - surf.smoothness;
   float2 lastScreenUV = (i.lastProjPos.xy / i.lastProjPos.w) * float2(0.5, 0.5) + 0.5;
   float2 screenUV = (i.currentProjPos.xy / i.currentProjPos.w) * float2(0.5, 0.5) + 0.5;
   motionVectorTex = screenUV - lastScreenUV;
   motionVectorTex.y = -motionVectorTex.y;
   float3 viewDir = normalize(worldSpaceCameraPos - i.PosW.xyz);
   i.NormalW = normalize(i.NormalW);
    float linearEyeDepth = LinearEyeDepth(i.PosH.z);
			BSDFContext context = (BSDFContext)0;
			float3 sunColor = clamp(CalculateSunLight_NoShadow(
				i.NormalW,
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
			float3 preint = (PreintegratedDGF_LUT(preintAB, EnergyCompensation,surf.specular));
			preint *= EnergyCompensation;
			float sunShadow = (GetShadow(i.PosW, linearEyeDepth, dot(i.NormalW, -_SunDir), _ShadowmapIndices));
			
			emissionRT = float4(sunColor * sunShadow + surf.emission, 1);
			emissionRT.xyz += (clamp(CalculateLocalLight(
				screenUV,
				i.PosW.xyz,
				linearEyeDepth,
				i.NormalW,
				viewDir,
				_CameraNearPos.w,
				_CameraFarPos.w,
				surf.albedo,
				surf.specular,
				roughness,
				_LightIndexBuffer,
				_AllLight
			), 0, 32768));
			emissionRT = KillNaN(emissionRT);
			albedo = float4(surf.albedo,surf.occlusion);
			specular = float4(preint,surf.smoothness);
   			normalTex = float4(i.NormalW * 0.5 + 0.5, 1);
		}

float4 VS_Depth(float3 position : POSITION) : SV_POSITION
{
    float4 posW = mul(_LocalToWorld, float4(position, 1));
    return mul(_VP, posW);
}

void PS_Depth(){}

float4 VS_Shadowmap(float3 position : POSITION) : SV_POSITION
{
	float4 posW = mul(_LocalToWorld, float4(position, 1));
	return mul(_ShadowmapVP, posW);
}

void PS_Shadowmap(){}

struct v2f_pointLight
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
};

v2f_pointLight VS_PointLightShdowmap(float3 position : POSITION)
{
	v2f_pointLight o;
	float4 posW = mul(_LocalToWorld, float4(position, 1));
	o.position = mul(_ShadowmapVP, posW);
	o.worldPos = posW.xyz;
	return o;
}

float PS_PointLightShadowmap(v2f_pointLight i) : SV_TARGET
{
	float dist = distance(_LightPos.xyz, i.worldPos);
	return dist / _LightPos.w;
}
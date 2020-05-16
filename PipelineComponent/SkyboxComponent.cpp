#include "SkyboxComponent.h"
#include "../Singleton/ShaderID.h"
#include "../RenderComponent/Skybox.h"
#include "../Singleton/PSOContainer.h"
#include "RenderPipeline.h"
#include "../RenderComponent/Texture.h"
#include "../RenderComponent/MeshRenderer.h"
#include "LightingComponent.h";
#include "../Singleton/ShaderCompiler.h"
#include "../LogicComponent/World.h"
#include "../RenderComponent/GRPRenderManager.h"
#include "DepthComponent.h"
#include "../RenderComponent/PBRMaterial.h"
#include "PrepareComponent.h"
#include "CameraData/CameraTransformData.h"
//#include "BaseColorComponent.h"
uint _DefaultMaterials;
StackObject<PSOContainer> gbufferContainer;
DepthComponent* depthComp;
const Shader* terrainTestShader;
using namespace Math;
PrepareComponent* prepareComp_Skybox;
struct TextureIndices
{
	uint _SkyboxCubemap;
	uint _PreintTexture;
};
const uint TEX_COUNT = sizeof(TextureIndices) / 4;

LightingComponent* lightComp_GBufferGlobal;
uint SkyboxComponent::_AllLight(0);
uint SkyboxComponent::_LightIndexBuffer(0);
uint SkyboxComponent::LightCullCBuffer(0);
uint SkyboxComponent::TextureIndices(0);
uint SkyboxComponent::_GreyTex;
uint SkyboxComponent::_IntegerTex;
uint SkyboxComponent::_Cubemap;
uint SkyboxComponent::_GreyCubemap;

struct SkyboxBuffer
{
	XMFLOAT4X4 invvp;
	XMFLOAT4X4 nonJitterVP;
	XMFLOAT4X4 lastVP;
};


GBufferCameraData::GBufferCameraData(ID3D12Device* device) :
	texIndicesBuffer(
		device, 1, true, sizeof(TextureIndices)
	),
	posBuffer(device,
		3, true,
		sizeof(SkyboxBuffer))
{
}

GBufferCameraData::~GBufferCameraData()
{

}
XMFLOAT4X4 CalculateViewMatrix(Camera* cam)
{
	Vector3 R = cam->GetRight();
	Vector3 U = cam->GetUp();
	Vector3 L = cam->GetLook();
	// Keep camera's axes orthogonal to each other and of unit length.
	L = normalize(L);
	U = normalize(cross(L, R));

	// U, L already ortho-normal, so no need to normalize cross product.
	R = cross(U, L);

	XMFLOAT3* mRight = (XMFLOAT3*)&R;
	XMFLOAT3* mUp = (XMFLOAT3*)&U;
	XMFLOAT3* mLook = (XMFLOAT3*)&L;
	XMFLOAT4X4 mView;
	mView(0, 0) = mRight->x;
	mView(1, 0) = mRight->y;
	mView(2, 0) = mRight->z;
	mView(3, 0) = 0;

	mView(0, 1) = mUp->x;
	mView(1, 1) = mUp->y;
	mView(2, 1) = mUp->z;
	mView(3, 1) = 0;

	mView(0, 2) = mLook->x;
	mView(1, 2) = mLook->y;
	mView(2, 2) = mLook->z;
	mView(3, 2) = 0;

	mView(0, 3) = 0.0f;
	mView(1, 3) = 0.0f;
	mView(2, 3) = 0.0f;
	mView(3, 3) = 1.0f;
	return mView;
}

class SkyboxRunnable
{
public:
	RenderTexture* emissionTex;
	RenderTexture* mvTex;
	RenderTexture* depthTex;
	RenderTexture* gbuffer0Tex;
	RenderTexture* gbuffer1Tex;
	RenderTexture* gbuffer2Tex;
	SkyboxComponent* selfPtr;
	ThreadCommand* commandList;
	FrameResource* resource;
	ID3D12Device* device;
	Camera* cam;
	uint frameIndex;
	void RenderGBuffer(GBufferCameraData* camData)
	{
		auto commandList = this->commandList->GetCmdList();
		auto world = World::GetInstance();
		DepthCameraResource* depthCameraRes = (DepthCameraResource*)cam->GetResource(depthComp, [=]()->DepthCameraResource*
			{
				return new DepthCameraResource(device);
			});



		TextureIndices* indices = (TextureIndices*)camData->texIndicesBuffer.GetMappedDataPtr(0);
		if (world->currentSkybox)
		{
			indices->_SkyboxCubemap = world->currentSkybox->GetTexture()->GetGlobalDescIndex();
		}
		indices->_PreintTexture = selfPtr->preintTexture->GetGlobalDescIndex();

		LightFrameData* lightFrameData = (LightFrameData*)resource->GetPerCameraResource(lightComp_GBufferGlobal, cam, []()->LightFrameData*
			{
#ifndef NDEBUG
				throw "No Light Data Exception!";
#endif
				return nullptr;	//Get Error if there is no light coponent in pipeline
			});
		LightCameraData* lightCameraData = (LightCameraData*)cam->GetResource(lightComp_GBufferGlobal, []()->LightCameraData*
			{
#ifndef NDEBUG
				throw "No Light Data Exception!";
#endif
				return nullptr;	//Get Error if there is no light coponent in pipeline
			});
		const DescriptorHeap* worldHeap = Graphics::GetGlobalDescHeap();

		const Shader* gbufferShader = world->GetGRPRenderManager()->GetShader();

		auto setGBufferShaderFunc = [&]()->void
		{
			gbufferShader->BindRootSignature(commandList, worldHeap);
			gbufferShader->SetResource(commandList, ShaderID::GetMainTex(), worldHeap, 0);
			gbufferShader->SetResource(commandList, SkyboxComponent::_GreyTex, worldHeap, 0);
			gbufferShader->SetResource(commandList, SkyboxComponent::_IntegerTex, worldHeap, 0);
			gbufferShader->SetResource(commandList, SkyboxComponent::_Cubemap, worldHeap, 0);
			gbufferShader->SetResource(commandList, SkyboxComponent::_GreyCubemap, worldHeap, 0);
			gbufferShader->SetResource(commandList, _DefaultMaterials, world->GetPBRMaterialManager()->GetMaterialBuffer(), 0);
			gbufferShader->SetStructuredBufferByAddress(commandList, SkyboxComponent::_AllLight, lightFrameData->lightsInFrustum.GetAddress(0));
			gbufferShader->SetStructuredBufferByAddress(commandList, SkyboxComponent::_LightIndexBuffer, lightComp_GBufferGlobal->lightIndexBuffer->GetAddress(0, 0));
			gbufferShader->SetResource(commandList, SkyboxComponent::LightCullCBuffer, &lightCameraData->lightCBuffer, frameIndex);
			gbufferShader->SetResource(commandList, SkyboxComponent::TextureIndices, &camData->texIndicesBuffer, 0);
			auto cb = resource->cameraCBs[cam];
			gbufferShader->SetResource(commandList, ShaderID::GetPerCameraBufferID(),
				cb.buffer, cb.element);
		};

		setGBufferShaderFunc();

		Graphics::SetRenderTarget(
			commandList,
			{ gbuffer0Tex , gbuffer1Tex, gbuffer2Tex, mvTex, emissionTex },
			depthTex);
		gbuffer0Tex->ClearRenderTarget(commandList, 0, 0);
		gbuffer1Tex->ClearRenderTarget(commandList, 0, 0);
		gbuffer2Tex->ClearRenderTarget(commandList, 0, 0);
		mvTex->ClearRenderTarget(commandList, 0, 0);
		emissionTex->ClearRenderTarget(commandList, 0, 0);
		
		uint gbufferPSOIndex = gbufferContainer->GetIndex(
			{ gbuffer0Tex->GetFormat() , gbuffer1Tex->GetFormat(), gbuffer2Tex->GetFormat(), mvTex->GetFormat(), emissionTex->GetFormat() },
			depthTex->GetFormat()
		);
		world->GetGRPRenderManager()->DrawCommand(
			commandList,
			device,
			0,
			gbufferContainer, gbufferPSOIndex
		);
		//Recheck Culling
		ConstBufferElement cullEle;
		cullEle.buffer = &depthCameraRes->cullBuffer;
		cullEle.element = frameIndex;
		//Culling
		world->GetGRPRenderManager()->OcclusionRecheck(
			commandList,
			*this->commandList->GetBarrierBuffer(),
			device,
			resource,
			cullEle,
			depthCameraRes->hizTexture);
		Graphics::SetRenderTarget(commandList, {}, depthTex);
		setGBufferShaderFunc();
		this->commandList->GetBarrierBuffer()->ExecuteCommand(commandList);
		Graphics::SetRenderTarget(
			commandList,
			{ gbuffer0Tex , gbuffer1Tex, gbuffer2Tex, mvTex, emissionTex },
			depthTex);
		world->GetGRPRenderManager()->DrawCommand(
			commandList,
			device,
			3,
			gbufferContainer, gbufferPSOIndex
		);
	}

	void operator()()
	{
		commandList->ResetCommand();
		TransitionBarrierBuffer* barrierBuffer = commandList->GetBarrierBuffer();
		auto world = World::GetInstance();
		ID3D12GraphicsCommandList* cmdList = commandList->GetCmdList();
		if (!selfPtr->preintTexture)
		{
			RenderTextureFormat format;
			format.colorFormat = DXGI_FORMAT_R16G16_UNORM;
			selfPtr->preintContainer.New(
				DXGI_FORMAT_UNKNOWN,
				1,
				&format.colorFormat
			);

			format.usage = RenderTextureUsage::ColorBuffer;
			selfPtr->preintTexture = new RenderTexture(
				device,
				256,
				256,
				format,
				TextureDimension::Tex2D,
				1,
				0);
			const Shader* preintShader = ShaderCompiler::GetShader("PreInt");
			preintShader->BindRootSignature(cmdList);
			Graphics::Blit(
				cmdList,
				device,
				&selfPtr->preintTexture->GetColorDescriptor(0, 0),
				1,
				nullptr,
				selfPtr->preintContainer, 0,
				selfPtr->preintTexture->GetWidth(),
				selfPtr->preintTexture->GetHeight(),
				preintShader,
				0
			);
			barrierBuffer->AddCommand(selfPtr->preintTexture->GetWriteState(), selfPtr->preintTexture->GetReadState(), selfPtr->preintTexture->GetResource());
		}
		GBufferCameraData* camData = (GBufferCameraData*)cam->GetResource(selfPtr, [&]()->GBufferCameraData*
			{
				return new GBufferCameraData(device);
			});
		RenderGBuffer(camData);
		if (world->currentSkybox)
		{
			CameraTransformData* transData = (CameraTransformData*)cam->GetResource(prepareComp_Skybox, []()->CameraTransformData*
				{
					return new CameraTransformData;
				});
			Matrix4 view = CalculateViewMatrix(cam);
			Matrix4 viewProj = mul(view, cam->GetProj());
			SkyboxBuffer bf;
			bf.invvp = inverse(viewProj);
			bf.nonJitterVP = mul(view, transData->nonJitteredProjMatrix);
			bf.lastVP = camData->vpMatrix;
			camData->vpMatrix = bf.nonJitterVP;
			camData->posBuffer.CopyData(frameIndex, &bf);
			Graphics::SetRenderTarget(cmdList, { emissionTex, mvTex }, depthTex);
			ConstBufferElement skyboxData;
			skyboxData.buffer = &camData->posBuffer;
			skyboxData.element = frameIndex;
			world->currentSkybox->Draw(
				0,
				cmdList,
				device,
				&skyboxData,
				resource,
				gbufferContainer, 
				gbufferContainer->GetIndex(
					{emissionTex->GetFormat(), mvTex->GetFormat()}, depthTex->GetFormat()
				)
			);
		}

		commandList->CloseCommand();
	}
};

std::vector<TemporalResourceCommand>& SkyboxComponent::SendRenderTextureRequire(const EventData& evt)
{
	for (int i = 0; i < tempRT.size(); ++i)
	{
		tempRT[i].descriptor.rtDesc.width = evt.width;
		tempRT[i].descriptor.rtDesc.height = evt.height;
	}
	return tempRT;
}
void SkyboxComponent::RenderEvent(const EventData& data, ThreadCommand* commandList)
{
	ScheduleJob<SkyboxRunnable>(
		{
			 (RenderTexture*)allTempResource[0],
			  (RenderTexture*)allTempResource[1],
			  (RenderTexture*)allTempResource[2],
			  (RenderTexture*)allTempResource[3],
			  (RenderTexture*)allTempResource[4],
			  (RenderTexture*)allTempResource[5],
			 this,
			 commandList,
			 data.resource,
			 data.device,
			 data.camera,
			 data.ringFrameIndex
		});
}


void SkyboxComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SetCPUDepending<LightingComponent, PrepareComponent>();
	SetGPUDepending<LightingComponent>();
	tempRT.resize(6);
	prepareComp_Skybox = RenderPipeline::GetComponent<PrepareComponent>();
	terrainTestShader = ShaderCompiler::GetShader("Terrain");
	lightComp_GBufferGlobal = RenderPipeline::GetComponent<LightingComponent>();
	depthComp = RenderPipeline::GetComponent<DepthComponent>();
	TemporalResourceCommand& emissionBuffer = tempRT[0];
	emissionBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	emissionBuffer.uID = ShaderID::PropertyToID("_CameraRenderTarget");
	emissionBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	emissionBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::ColorBuffer;
	emissionBuffer.descriptor.rtDesc.depthSlice = 1;
	emissionBuffer.descriptor.rtDesc.type = TextureDimension::Tex2D;
	tempRT[1].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[1].uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	tempRT[2].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[2].uID = ShaderID::PropertyToID("_CameraDepthTexture");

	TemporalResourceCommand& specularBuffer = tempRT[3];
	specularBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	specularBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture0");
	specularBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	specularBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::ColorBuffer;
	specularBuffer.descriptor.rtDesc.depthSlice = 1;
	specularBuffer.descriptor.rtDesc.type = TextureDimension::Tex2D;
	TemporalResourceCommand& albedoBuffer = tempRT[4];
	albedoBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	albedoBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture1");
	albedoBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	albedoBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::ColorBuffer;
	albedoBuffer.descriptor.rtDesc.depthSlice = 1;
	albedoBuffer.descriptor.rtDesc.type = TextureDimension::Tex2D;
	TemporalResourceCommand& normalBuffer = tempRT[5];
	normalBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	normalBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture2");
	normalBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	normalBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::ColorBuffer;
	normalBuffer.descriptor.rtDesc.depthSlice = 1;
	normalBuffer.descriptor.rtDesc.type = TextureDimension::Tex2D;
	/*
	PSORTSetting settings[3];
	settings[0].rtCount = 5;
	settings[0].rtFormat[0] = albedoBuffer.descriptor.rtDesc.rtFormat.colorFormat;
	settings[0].rtFormat[1] = specularBuffer.descriptor.rtDesc.rtFormat.colorFormat;
	settings[0].rtFormat[2] = normalBuffer.descriptor.rtDesc.rtFormat.colorFormat;
	settings[0].rtFormat[3] = DXGI_FORMAT_R16G16_SNORM;
	settings[0].rtFormat[4] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	settings[0].depthFormat = DXGI_FORMAT_D32_FLOAT;
	settings[1].rtCount = 0;
	settings[1].depthFormat = DXGI_FORMAT_D32_FLOAT;
	settings[2].depthFormat = DXGI_FORMAT_D32_FLOAT;
	settings[2].rtCount = 2;
	settings[2].rtFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	settings[2].rtFormat[1] = DXGI_FORMAT_R16G16_SNORM;

	gbufferContainer.New(settings, 3);*/
	gbufferContainer.New();

	_AllLight = ShaderID::PropertyToID("_AllLight");
	_LightIndexBuffer = ShaderID::PropertyToID("_LightIndexBuffer");
	LightCullCBuffer = ShaderID::PropertyToID("LightCullCBuffer");
	TextureIndices = ShaderID::PropertyToID("TextureIndices");
	_DefaultMaterials = ShaderID::PropertyToID("_DefaultMaterials");
	_GreyTex = ShaderID::PropertyToID("_GreyTex");
	_IntegerTex = ShaderID::PropertyToID("_IntegerTex");
	_Cubemap = ShaderID::PropertyToID("_Cubemap");
	_GreyCubemap = ShaderID::PropertyToID("_GreyCubemap");
}
void SkyboxComponent::Dispose()
{
	preintContainer.Delete();
	gbufferContainer.Delete();
}
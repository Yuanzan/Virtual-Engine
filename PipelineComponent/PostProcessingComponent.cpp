#include "PostProcessingComponent.h"
#include "../RenderComponent/TransitionBarrierBuffer.h"
#include "../Singleton/PSOContainer.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../LogicComponent/World.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "PrepareComponent.h"
//#include "TemporalAA.h"
#include "../RenderComponent/Texture.h"
#include "PostProcess/ColorGradingLut.h"
#include "PostProcess//LensDistortion.h"
#include "PostProcess/MotionBlur.h"
#include "RenderPipeline.h"
#include "../Common/MetaLib.h"
#include "AutoExposure.h"
#include "Bloom.h"
#include "../Common/d3dApp.h"
#include "../CJsonObject/CJsonObject.hpp"
//#include "DepthComponent.h"

//#include "SkyboxComponent.h"
namespace PostGlobal
{
	const Shader* postShader;
	StackObject<PSOContainer> backBufferContainer;

	int _Lut3D;
	uint _ExposureTex;
	uint _BloomTex;
	uint _Bloom_DirtTex;
	//std::unique_ptr<TemporalAA> taaComponent;
	StackObject<Bloom> blooms;
	StackObject<ColorGrading> lutComponent;
	StackObject<MotionBlur> motionBlurComponent;
	StackObject<AutoExposure> autoExposureComponent;
	StackObject<LensDistortion> lensComponent;
	StackObject<ChromaticAberration> chromaticComponent;
	//DepthComponent* depthComp_Post;
}
using namespace PostGlobal;
//ObjectPtr<Texture> testTex;
//PrepareComponent* prepareComp = nullptr;
struct PostParams
{
	BloomCBuffers bloomData;
	float4 _Lut3DParam;
	float4 _Distortion_Amount;
	float4 _Distortion_CenterScale;
	float4 _MainTex_TexelSize;
	float _ChromaticAberration_Amount;
	uint _AutoExposureEnabled;
	uint _ChromaticEnabled;
	uint _BloomEnabled;
	uint _ColorGradingEnabled;
};
class PostCameraData : public IPipelineResource
{
public:
	DescriptorHeap postSRVHeap;
	UploadBuffer postUBuffer;
	inline static const uint POST_HEAP_SIZE = 5;
	PostCameraData(ID3D12Device* device)
		: postSRVHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, POST_HEAP_SIZE * 3, true),
		postUBuffer(device, 3, true, sizeof(PostParams))
	{

	}
};
class PostFrameData : public IPipelineResource
{
public:

	RenderTexture* texs[20];
	PostFrameData()
	{
	}
};
uint Params = 0;
class PostRunnable
{
public:
	RenderTexture* renderTarget;
	RenderTexture* depthTarget;
	RenderTexture* motionVector;
	RenderTexture* destMap;
	ThreadCommand* threadCmd;
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle;
	ID3D12Resource* backBuffer;
	ID3D12Device* device;
	UINT width;
	UINT height;
	void* selfPtr;
	FrameResource* resource;
	bool isForPresent;
	Camera* cam;
	PrepareComponent* prepareComp;
	float deltaTime;
	uint frameIndex;
	void operator()()
	{
		threadCmd->ResetCommand();
		TransitionBarrierBuffer* transitionBarrier = threadCmd->GetBarrierBuffer();
		ID3D12GraphicsCommandList* commandList = threadCmd->GetCmdList();
		PostFrameData* frameRes = (PostFrameData*)resource->GetPerCameraResource(selfPtr, cam,
			[=]()->PostFrameData*
			{
				return nullptr;
			});
		PostCameraData* camRes = (PostCameraData*)cam->GetResource(selfPtr, [&]()->PostCameraData*
			{
				return new PostCameraData(device);
			});

		RenderTexture* blitSource = destMap;
		RenderTexture* blitDest = renderTarget;
		auto SwitchRenderTarget = [&]()->void
		{
			RenderTexture* swaper = blitSource;
			blitSource = blitDest;
			blitDest = swaper;

			transitionBarrier->AddCommand(blitSource->GetWriteState(), blitSource->GetReadState(), blitSource->GetResource());
			transitionBarrier->AddCommand(blitDest->GetReadState(), blitDest->GetWriteState(), blitDest->GetResource());

		};

		transitionBarrier->AddCommand(blitSource->GetWriteState(), blitSource->GetReadState(), blitSource->GetResource());
		transitionBarrier->AddCommand(frameRes->texs[1]->GetWriteState(), frameRes->texs[1]->GetReadState(), frameRes->texs[1]->GetResource());
		transitionBarrier->AddCommand(frameRes->texs[3]->GetWriteState(), frameRes->texs[3]->GetReadState(), frameRes->texs[3]->GetResource());
		PostParams params;
		if (motionBlurComponent->Execute(
			device,
			threadCmd,
			cam,
			resource,
			blitSource,
			blitDest,
			frameRes->texs,
			prepareComp->_ZBufferParams,
			blitDest->GetWidth(),
			blitDest->GetHeight(),
			3, 1, frameIndex))
		{
			SwitchRenderTarget();
		}

		bool lutEnabled = (*lutComponent)(
			device,
			commandList,
			transitionBarrier);
		blitSource->BindSRVToHeap(&camRes->postSRVHeap, 0 + PostCameraData::POST_HEAP_SIZE * frameIndex, device);
		if (lutEnabled)
			lutComponent->lut->BindSRVToHeap(&camRes->postSRVHeap, 1 + PostCameraData::POST_HEAP_SIZE * frameIndex, device);

		RenderTexture* autoExposureRT = autoExposureComponent->Render(
			device,
			commandList,
			width, height,
			blitSource,
			cam,
			resource,
			deltaTime, transitionBarrier, frameIndex);

		if (autoExposureRT)
			autoExposureRT->BindSRVToHeap(&camRes->postSRVHeap, 2 + PostCameraData::POST_HEAP_SIZE * frameIndex, device);
		Texture* dirtTex = nullptr;
		RenderTexture* bloomRT = nullptr;
		bool isBloomEnabled = blooms->Render(
			commandList, device,
			width, height,
			blitSource,
			cam,
			resource,
			autoExposureRT,
			params.bloomData,
			dirtTex,
			bloomRT, transitionBarrier);

		if (isBloomEnabled)
		{
			bloomRT->BindSRVToHeap(&camRes->postSRVHeap, 3 + PostCameraData::POST_HEAP_SIZE * frameIndex, device);
			ITexture* iDirtTex = (dirtTex != nullptr) ? (ITexture*)dirtTex : (ITexture*)bloomRT;
			iDirtTex->BindSRVToHeap(&camRes->postSRVHeap, 4 + PostCameraData::POST_HEAP_SIZE * frameIndex, device);
		}


		if (isForPresent)
		{
			transitionBarrier->AddCommand(
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				backBuffer);
		}

		postShader->BindRootSignature(commandList, &camRes->postSRVHeap);

		params._Lut3DParam = { 1.0f / lutComponent->k_Lut3DSize, lutComponent->k_Lut3DSize - 1.0f, 1, 1 };
		params._MainTex_TexelSize = {
			1.0f / width,
			1.0f / height,
			(float)width,
			(float)height
		};
		lensComponent->GetLensDistortion(
			params._Distortion_CenterScale,
			params._Distortion_Amount);
		bool isChromaticEnabled = chromaticComponent->Run(params._ChromaticAberration_Amount);
		//Set Dynamic Branch
		params._ColorGradingEnabled = lutEnabled;
		params._AutoExposureEnabled = autoExposureRT != nullptr;
		params._BloomEnabled = isBloomEnabled;
		params._ChromaticEnabled = isChromaticEnabled;

		postShader->SetResource(commandList, ShaderID::GetMainTex(), &camRes->postSRVHeap, 0 + PostCameraData::POST_HEAP_SIZE * frameIndex);
		postShader->SetResource(commandList, _ExposureTex, &camRes->postSRVHeap, 2 + PostCameraData::POST_HEAP_SIZE * frameIndex);
		postShader->SetResource(commandList, _Lut3D, &camRes->postSRVHeap, 1 + PostCameraData::POST_HEAP_SIZE * frameIndex);
		camRes->postUBuffer.CopyData(frameIndex, &params);
		postShader->SetResource(commandList, Params, &camRes->postUBuffer, frameIndex);
		postShader->SetResource(commandList, _BloomTex, &camRes->postSRVHeap, 3 + PostCameraData::POST_HEAP_SIZE * frameIndex);
		postShader->SetResource(commandList, _Bloom_DirtTex, &camRes->postSRVHeap, 4 + PostCameraData::POST_HEAP_SIZE * frameIndex);
		transitionBarrier->ExecuteCommand(commandList);
		Graphics::Blit(
			commandList,
			device,
			&backBufferHandle,
			1,
			nullptr,
			backBufferContainer, 0,
			width, height,
			postShader,
			0
		);
		if (isForPresent) {
			transitionBarrier->AddCommand(
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT,
				backBuffer);
		}
		transitionBarrier->AddCommand(frameRes->texs[1]->GetReadState(), frameRes->texs[1]->GetWriteState(), frameRes->texs[1]->GetResource());
		transitionBarrier->AddCommand(frameRes->texs[3]->GetReadState(), frameRes->texs[3]->GetWriteState(), frameRes->texs[3]->GetResource());
		transitionBarrier->AddCommand(blitSource->GetReadState(), blitSource->GetWriteState(), blitSource->GetResource());

		threadCmd->CloseCommand();
	}
};

void PostProcessingComponent::RenderEvent(const EventData& data, ThreadCommand* commandList)
{
	PostFrameData* frameRes = (PostFrameData*)data.resource->GetPerCameraResource(this, data.camera,
		[]()->PostFrameData*
		{
			return new PostFrameData();
		});
	memcpy(frameRes->texs, allTempResource.data(), sizeof(RenderTexture*) * allTempResource.size());
	JobHandle handle = ScheduleJob<PostRunnable>({
		(RenderTexture*)allTempResource[0],
		(RenderTexture*)allTempResource[3],
		(RenderTexture*)allTempResource[1],
		(RenderTexture*)allTempResource[2],
		commandList,
		data.backBufferHandle,
		data.backBuffer,
		data.device,
		data.width,
		data.height,
		this,
		data.resource,
		data.isBackBufferForPresent,
		data.camera,
		prepareComp ,
		data.deltaTime,
		data.ringFrameIndex });
}
void PostProcessingComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	jsonData.New();
	uint bloomJsonTypeIndex;
	uint lutJsonTypeIndex;
	uint motionBlurJsonTypeIndex;
	uint autoExposureJsonTypeIndex;
	uint lensJsonTypeIndex;
	uint chromaticJsonTypeIndex;
	bloomJsonTypeIndex = jsonData->TypeToIndex<Bloom>();
	lutJsonTypeIndex = jsonData->TypeToIndex<ColorGrading>();
	motionBlurJsonTypeIndex = jsonData->TypeToIndex<MotionBlur>();
	autoExposureJsonTypeIndex = jsonData->TypeToIndex<AutoExposure>();
	lensJsonTypeIndex = jsonData->TypeToIndex<LensDistortion>();
	chromaticJsonTypeIndex = jsonData->TypeToIndex<ChromaticAberration>();
	_Lut3D = ShaderID::PropertyToID("_Lut3D");
	Params = ShaderID::PropertyToID("Params");
	_ExposureTex = ShaderID::PropertyToID("_ExposureTex");
	_BloomTex = ShaderID::PropertyToID("_BloomTex");
	_Bloom_DirtTex = ShaderID::PropertyToID("_Bloom_DirtTex");
	//depthComp_Post = RenderPipeline::GetComponent<DepthComponent>();
	SetCPUDepending<PrepareComponent>();
	//	SetGPUDepending<SkyboxComponent>();
	tempRT.reserve(20);
	tempRT.resize(4);
	tempRT[0].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[0].uID = ShaderID::PropertyToID("_CameraRenderTarget");
	tempRT[1].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[1].uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	tempRT[2].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[2].uID = ShaderID::PropertyToID("_PostProcessBlitTarget");
	tempRT[3].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[3].uID = ShaderID::PropertyToID("_CameraDepthTexture");

	motionBlurComponent.New();
	motionBlurComponent->Init(tempRT, jsonData->GetJsonObject(motionBlurJsonTypeIndex));
	autoExposureComponent.New(device, jsonData->GetJsonObject(autoExposureJsonTypeIndex));

	postShader = ShaderCompiler::GetShader("PostProcess");
	DXGI_FORMAT backBufferFormat = D3DApp::BACK_BUFFER_FORMAT();
	backBufferContainer.New(DXGI_FORMAT_UNKNOWN, 1, &backBufferFormat);
	blooms.New(device, jsonData->GetJsonObject(bloomJsonTypeIndex));
	prepareComp = RenderPipeline::GetComponent<PrepareComponent>();
	lensComponent.New(jsonData->GetJsonObject(lensJsonTypeIndex));
	chromaticComponent.New(jsonData->GetJsonObject(chromaticJsonTypeIndex));
	/*
	taaComponent = std::unique_ptr<TemporalAA>(new TemporalAA());
	taaComponent->prePareComp = prepareComp;
	taaComponent->device = device;
	taaComponent->toRTContainer = renderTextureContainer.get();*/
	lutComponent.New(device, jsonData->GetJsonObject(lutJsonTypeIndex));
	/*	testTex = new Texture(
			device,
			"Test",
			L"Resource/Test.vtex"
		);*/
}

std::vector<TemporalResourceCommand>& PostProcessingComponent::SendRenderTextureRequire(const EventData& evt)
{
	auto& desc = tempRT[2].descriptor;
	desc.rtDesc.width = evt.width;
	desc.rtDesc.height = evt.height;
	motionBlurComponent->UpdateTempRT(tempRT, evt);
	return tempRT;
}

void PostProcessingComponent::Dispose()
{
	backBufferContainer.Delete();

	blooms.Delete();
	autoExposureComponent.Delete();
	lutComponent.Delete();
	motionBlurComponent.Delete();
	lensComponent.Delete();
	chromaticComponent.Delete();

	jsonData.Delete();
}
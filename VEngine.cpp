//***************************************************************************************
// VEngine.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************
#include "RenderComponent/Shader.h"
#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "RenderComponent/UploadBuffer.h"
#include "Singleton/FrameResource.h"
#include "Singleton/ShaderID.h"
#include "Common/Input.h"
#include "RenderComponent/Texture.h"
#include "Singleton/MeshLayout.h"
#include "Singleton/PSOContainer.h"
#include "Common/Camera.h"
#include "RenderComponent/Mesh.h"
#include "RenderComponent/MeshRenderer.h"
#include "Singleton/ShaderCompiler.h"
#include "RenderComponent/Skybox.h"
#include "RenderComponent/ComputeShader.h"
#include "RenderComponent/RenderTexture.h"
#include "PipelineComponent/RenderPipeline.h"
#include "Singleton/Graphics.h"
#include "Singleton/MathLib.h"
#include "JobSystem/JobInclude.h"
#include "ResourceManagement/AssetDatabase.h"
#include "Common/RuntimeStatic.h"
using Microsoft::WRL::ComPtr;
using namespace Math;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

#include "LogicComponent/World.h"
#include "IRendererBase.h"

D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView_VEngine(const D3DAppDataPack& dataPack);
ID3D12Resource* CurrentBackBuffer_VEngine(const D3DAppDataPack& dataPack);
class VEngine
{
public:
	std::vector<JobBucket*> buckets[2];
	std::vector<JobHandle> dependingHandles;
	bool bucketsFlag = false;
	ComPtr<ID3D12CommandQueue> mComputeCommandQueue;
	ComPtr<ID3D12CommandQueue> mCopyCommandQueue;
	INLINE VEngine() {
		RuntimeStaticBase::ConstructAll();
	}
	INLINE ~VEngine()
	{
		RuntimeStaticBase::DestructAll();
	}
	INLINE VEngine(const VEngine& rhs) = delete;
	INLINE VEngine& operator=(const VEngine& rhs) = delete;
	INLINE void InitRenderer(D3DAppDataPack& pack, const D3DAppFunctionPack& funcPack)
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

		ThrowIfFailed(pack.md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mComputeCommandQueue)));
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		ThrowIfFailed(pack.md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCopyCommandQueue)));
		directThreadCommand.New(pack.md3dDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		directThreadCommand->ResetCommand();
		// Reset the command list to prep for initialization commands.
		ShaderCompiler::Init(pack.md3dDevice.Get(), pipelineJobSys.GetPtr());
		BuildFrameResources(pack);

		//BuildPSOs();
		// Execute the initialization commands.
		// Wait until initialization is complete.
		Graphics::Initialize(pack.md3dDevice.Get(), directThreadCommand->GetCmdList());
		World::CreateInstance(directThreadCommand->GetCmdList(), pack.md3dDevice.Get());

		rp = RenderPipeline::GetInstance(pack.md3dDevice.Get(),
			directThreadCommand->GetCmdList());
		directThreadCommand->CloseCommand();
		ID3D12CommandList* lst = directThreadCommand->GetCmdList();
		pack.mCommandQueue->ExecuteCommandLists(1, &lst);
		funcPack.flushCommandQueue(pack);
		directThreadCommand.Delete();
	}
	INLINE void DisposeRenderer()
	{
		FrameResource::mFrameResources.clear();

		RenderPipeline::DestroyInstance();
		ShaderCompiler::Dispose();
	}
	INLINE bool Initialize(D3DAppDataPack& pack, const D3DAppFunctionPack& funcPack)
	{
		PtrLink::globalEnabled = true;
		//pack.mSwapChain->SetFullscreenState(true, nullptr);
		ShaderID::Init();
		buckets[0].reserve(20);
		buckets[1].reserve(20);
		UINT cpuCoreCount = std::thread::hardware_concurrency() - 2;	//One for main thread & one for loading
		pipelineJobSys.New(Max<uint>(1, cpuCoreCount));
		AssetDatabase::CreateInstance(pack.md3dDevice.Get());
		InitRenderer(pack, funcPack);
		return true;
	}
	INLINE void OnResize(D3DAppDataPack& pack, const D3DAppFunctionPack& funcPack)
	{
		auto& vec = FrameResource::mFrameResources;
		for (auto ite = vec.begin(); ite != vec.end(); ++ite)
		{
			(*ite)->commandBuffer->Submit();
			(*ite)->commandBuffer->Clear();
		}
		funcPack.flushCommandQueue(pack);
	}
	INLINE bool Draw(D3DAppDataPack& pack, GameTimer& timer, const D3DAppFunctionPack& funcPack)
	{
		if (pack.settings.mClientHeight < 1 || pack.settings.mClientWidth < 1) return true;
		mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
		lastResource = FrameResource::mCurrFrameResource;
		FrameResource::mCurrFrameResource = FrameResource::mFrameResources[mCurrFrameResourceIndex].get();
		auto currentFrameResource = FrameResource::mCurrFrameResource;
		std::vector <JobBucket*>& bucketArray = buckets[bucketsFlag];
		bucketsFlag = !bucketsFlag;
		JobBucket* mainLogicBucket = pipelineJobSys->GetJobBucket();
		bucketArray.push_back(mainLogicBucket);

		//Rendering
		pack.settings.windowInfo = const_cast<wchar_t*>(World::GetInstance()->windowInfo.empty() ? nullptr : World::GetInstance()->windowInfo.c_str());
		pack.settings.mCurrBackBuffer = (pack.settings.mCurrBackBuffer + 1) % pack.SwapChainBufferCount;
		RenderPipelineData data;
		data.device = pack.md3dDevice.Get();
		data.commandQueue = pack.mCommandQueue.Get();
		data.backBufferHandle = CurrentBackBufferView_VEngine(pack);
		data.backBufferResource = CurrentBackBuffer_VEngine(pack);
		data.lastResource = lastResource;
		data.resource = currentFrameResource;
		data.allCameras = &World::GetInstance()->GetCameras();
		data.fence = pack.mFence.Get();
		data.fenceIndex = &pack.settings.mCurrentFence;
		data.ringFrameIndex = mCurrFrameResourceIndex;
		data.world = World::GetInstance();
		data.world->windowWidth = pack.settings.mClientWidth;
		data.world->windowHeight = pack.settings.mClientHeight;
		data.deltaTime = timer.DeltaTime();
		data.time = timer.TotalTime();
		data.isMovingTheWorld = false;
		data.worldMovingDir = { 0,0,0 };
		/*if (Input::IsKeyDown(KeyCode::M))
		{
			data.isMovingTheWorld = true;
			data.worldMovingDir = { 10, 10, 10 };
		}*/
		Vector3 worldMoveDir;
		bool isMovingTheWorld = false;
		dependingHandles.clear();
		World::GetInstance()->PrepareUpdateJob(mainLogicBucket, currentFrameResource, pack.md3dDevice.Get(), timer, int2(pack.settings.mClientWidth, pack.settings.mClientHeight),
			worldMoveDir, isMovingTheWorld, dependingHandles);
		if (isMovingTheWorld)
		{
			data.isMovingTheWorld = true;
			data.worldMovingDir = worldMoveDir;
		}
		JobHandle cameraUpdateJob = mainLogicBucket->GetTask(dependingHandles.data(), dependingHandles.size(), [&, currentFrameResource, worldMoveDir, isMovingTheWorld]()->void
			{
				AssetDatabase::GetInstance()->MainThreadUpdate();
				World::GetInstance()->Update(currentFrameResource, pack.md3dDevice.Get(), timer, int2(pack.settings.mClientWidth, pack.settings.mClientHeight), worldMoveDir, isMovingTheWorld);

			});

		rp->PrepareRendering(data, pipelineJobSys.GetPtr(), bucketArray);
		
		HRESULT crashResult = pack.md3dDevice->GetDeviceRemovedReason();
		if (crashResult != S_OK)
		{
			return false;
		}
		Input::UpdateFrame(int2(-1, -1));//Update Input Buffer
	//	SetCursorPos(mClientWidth / 2, mClientHeight / 2);
		data.resource->UpdateBeforeFrame(data.fence);//Flush CommandQueue
		pipelineJobSys->Wait();//Last Frame's Logic Stop Here
		pipelineJobSys->ExecuteBucket(bucketArray.data(), bucketArray.size());					//Execute Tasks
		HRESULT presentResult = rp->ExecuteRendering(data, pack.mSwapChain.Get());
#if defined(DEBUG) | defined(_DEBUG)
		ThrowHResult(presentResult, PresentFunction);
#endif
		if (presentResult != S_OK)
		{
			return false;
		}

		std::vector <JobBucket*>& lastBucketArray = buckets[bucketsFlag];
		for (auto ite = lastBucketArray.begin(); ite != lastBucketArray.end(); ++ite)
		{
			pipelineJobSys->ReleaseJobBucket(*ite);
		}

		lastBucketArray.clear();
		return true;
	}
	INLINE void Dispose(D3DAppDataPack& pack, const D3DAppFunctionPack& funcPack)
	{
		pipelineJobSys->Wait();
		if (pack.md3dDevice != nullptr)
			funcPack.flushCommandQueue(pack);
		DisposeRenderer();
		AssetDatabase::DestroyInstance();
		pipelineJobSys.Delete();
		World::DestroyInstance();
		PtrLink::globalEnabled = false;
		mComputeCommandQueue = nullptr;
		mCopyCommandQueue = nullptr;
	}
	//void AnimateMaterials(const GameTimer& gt);
	//void UpdateObjectCBs(const GameTimer& gt);

	StackObject<JobSystem> pipelineJobSys;
	INLINE void BuildFrameResources(D3DAppDataPack& pack)
	{
		FrameResource::mFrameResources.resize(gNumFrameResources);
		for (int i = 0; i < gNumFrameResources; ++i)
		{
			FrameResource::mFrameResources[i] = std::unique_ptr<FrameResource>(new FrameResource(pack.md3dDevice.Get(), 1, 1));
			FrameResource::mFrameResources[i]->commandBuffer.New(pack.mCommandQueue.Get(), mComputeCommandQueue.Get(), mCopyCommandQueue.Get());
		}
	}
	int mCurrFrameResourceIndex = 0;
	RenderPipeline* rp;

	float mTheta = 1.3f * XM_PI;
	float mPhi = 0.4f * XM_PI;
	float mRadius = 2.5f;
	POINT mLastMousePos;
	FrameResource* lastResource = nullptr;
	StackObject<ThreadCommand> directThreadCommand;
};
void __cdecl GetRendererFunction(IRendererBase& renderBase)
{
	renderBase.rendererSize = sizeof(VEngine);
	renderBase.rendererAlign = alignof(VEngine);
	renderBase.Dispose = [](void* ptr, D3DAppDataPack& dataPack, const D3DAppFunctionPack& funcPack)->void
	{
		VEngine* ve = (VEngine*)ptr;
		ve->Dispose(dataPack, funcPack);
		ve->~VEngine();
	};
	renderBase.Initialize = [](void* ptr, D3DAppDataPack& dataPack, const D3DAppFunctionPack& funcPack)->bool
	{
		VEngine* ve = (VEngine*)ptr;
		new (ve)VEngine();
		if (!ve->Initialize(dataPack, funcPack))
		{
			ve->~VEngine();
			return false;
		}
		return true;
	};
	renderBase.Draw = [](void* ptr, D3DAppDataPack& pack, GameTimer& ge, const D3DAppFunctionPack& funcPack)->bool
	{
		VEngine* ve = (VEngine*)ptr;
		return ve->Draw(pack, ge, funcPack);
	};
	renderBase.OnResize = [](void* ptr, D3DAppDataPack& pack, const D3DAppFunctionPack& funcPack)->void
	{
		VEngine* ve = (VEngine*)ptr;
		ve->OnResize(pack, funcPack);
	};
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	StackObject<D3DApp> d3dApp;
	IRendererBase renderBase;
	GetRendererFunction(renderBase);

	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	try
	{
#endif
		d3dApp.New(hInstance);
		if (!d3dApp->Initialize(renderBase))
			return 0;
		int value = d3dApp->Run();
		if (value == -1)
		{
			return 0;
		}
		d3dApp.Delete();
#if defined(DEBUG) | defined(_DEBUG)
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
#endif
}



ID3D12Resource* CurrentBackBuffer_VEngine(const D3DAppDataPack& dataPack)
{
	return dataPack.mSwapChainBuffer[dataPack.settings.mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView_VEngine(const D3DAppDataPack& dataPack)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		dataPack.mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		dataPack.settings.mCurrBackBuffer,
		dataPack.settings.mRtvDescriptorSize);
}

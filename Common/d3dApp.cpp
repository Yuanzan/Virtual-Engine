//***************************************************************************************
// d3dApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "d3dApp.h"
#include "Memory.h"
#include "Input.h"
#include <WindowsX.h>
#include "../IRendererBase.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}
D3DApp* D3DApp::mApp = nullptr;
D3DApp* D3DApp::GetApp()
{
	return mApp;
}
D3DApp::D3DApp(HINSTANCE hInstance)
{
	dataPack.Clear();
	dataPack.settings.mClientWidth = 2560;
	dataPack.settings.mClientHeight = 1440;
	funcPack.flushCommandQueue = [](D3DAppDataPack& dataPack)
	{
		// Advance the fence value to mark commands up to this fence point.
		dataPack.settings.mCurrentFence++;

		// Add an instruction to the command queue to set a new fence point.  Because we 
		// are on the GPU timeline, the new fence point won't be set until the GPU finishes
		// processing all the commands prior to this Signal().
		ThrowIfFailed(dataPack.mCommandQueue->Signal(dataPack.mFence.Get(), dataPack.settings.mCurrentFence));
		if (dataPack.mFence->GetCompletedValue() < dataPack.settings.mCurrentFence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

			// Fire event when GPU hits current fence.  
			ThrowIfFailed(dataPack.mFence->SetEventOnCompletion(dataPack.settings.mCurrentFence, eventHandle));

			// Wait until the GPU hits current fence event is fired.
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
		// Wait until the GPU has completed commands up to this fence point.
	};
	dataPack.settings.mhAppInst = hInstance;

	if (mApp) delete mApp;
	mApp = this;
}
#undef RELEASE_PTR

D3DApp::~D3DApp()
{
	renderBaseFuncs.Dispose(rendererPtr, dataPack, funcPack);
	aligned_free(rendererPtr);
	rendererPtr = nullptr;
	dataPack.mSwapChain->SetFullscreenState(false, nullptr);
	mApp = nullptr;
#define RELEASE_PTR(ptr) ptr = nullptr;
	RELEASE_PTR(dataPack.mdxgiFactory);
	RELEASE_PTR(dataPack.mSwapChain);
	RELEASE_PTR(dataPack.md3dDevice);
	RELEASE_PTR(dataPack.mFence);
	RELEASE_PTR(dataPack.mCommandQueue);
	for (uint i = 0; i < dataPack.SwapChainBufferCount; ++i)
		RELEASE_PTR(dataPack.mSwapChainBuffer[i]);
	RELEASE_PTR(dataPack.mRtvHeap);
#undef RELEASE_PTR
}

int D3DApp::Run()
{
	MSG msg = { 0 };

	timer.Reset();
	while (msg.message != WM_QUIT)
	{

		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff.
		else
		{
			timer.Tick();
			if (!dataPack.settings.mAppPaused)
			{
				CalculateFrameStats();
				if (!renderBaseFuncs.Draw(rendererPtr, dataPack, timer, funcPack))
				{
					if (dataPack.mSwapChain)
						dataPack.mSwapChain->SetFullscreenState(false, nullptr);
					return -1;
				}
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}
bool windowsInitialized = false;
bool D3DApp::Initialize(const IRendererBase& renderBase)
{
	renderBaseFuncs = renderBase;
	if (!windowsInitialized)
	{
		if (!InitMainWindow())
			return false;
		windowsInitialized = true;
	}

	if (!InitDirect3D())
		return false;
	OnResize();
	rendererPtr = aligned_malloc(renderBase.rendererSize, renderBase.rendererAlign);
	return renderBase.Initialize(rendererPtr, dataPack, funcPack);
}

void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = dataPack.SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(dataPack.md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(&dataPack.mRtvHeap)));
}

void D3DApp::OnResize()
{
	assert(dataPack.md3dDevice);
	assert(dataPack.mSwapChain);
	if (rendererPtr)
		renderBaseFuncs.OnResize(rendererPtr, dataPack, funcPack);
	// Release the previous resources we will be recreating.
	for (int i = 0; i < dataPack.SwapChainBufferCount; ++i)
	{
		dataPack.mSwapChainBuffer[i] = nullptr;

	}

	// Resize the swap chain.
	ThrowIfFailed(dataPack.mSwapChain->ResizeBuffers(
		dataPack.SwapChainBufferCount,
		dataPack.settings.mClientWidth, dataPack.settings.mClientHeight,
		dataPack.BACK_BUFFER_FORMAT,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	dataPack.settings.mCurrBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(dataPack.mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < dataPack.SwapChainBufferCount; i++)
	{
		ThrowIfFailed(dataPack.mSwapChain->GetBuffer(i, IID_PPV_ARGS(&dataPack.mSwapChainBuffer[i])));
		dataPack.md3dDevice->CreateRenderTargetView(dataPack.mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, dataPack.settings.mRtvDescriptorSize);
	}
	// Update the viewport transform to cover the client area.
	dataPack.settings.mScreenViewport.TopLeftX = 0;
	dataPack.settings.mScreenViewport.TopLeftY = 0;
	dataPack.settings.mScreenViewport.Width = static_cast<float>(dataPack.settings.mClientWidth);
	dataPack.settings.mScreenViewport.Height = static_cast<float>(dataPack.settings.mClientHeight);
	dataPack.settings.mScreenViewport.MinDepth = 0.0f;
	dataPack.settings.mScreenViewport.MaxDepth = 1.0f;

	dataPack.settings.mScissorRect = { 0, 0, dataPack.settings.mClientWidth, dataPack.settings.mClientHeight };
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			OnPressMinimizeKey(true);
			dataPack.settings.mAppPaused = true;
			timer.Stop();
		}
		else
		{
			OnPressMinimizeKey(false);
			dataPack.settings.mAppPaused = false;
			timer.Start();
		}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		dataPack.settings.mClientWidth = LOWORD(lParam);
		dataPack.settings.mClientHeight = HIWORD(lParam);
		if (dataPack.md3dDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				dataPack.settings.mAppPaused = true;
				dataPack.settings.mMinimized = true;
				dataPack.settings.mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				dataPack.settings.mAppPaused = false;
				dataPack.settings.mMinimized = false;
				dataPack.settings.mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// Restoring from minimized state?
				if (dataPack.settings.mMinimized)
				{
					dataPack.settings.mAppPaused = false;
					dataPack.settings.mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if (dataPack.settings.mMaximized)
				{
					dataPack.settings.mAppPaused = false;
					dataPack.settings.mMaximized = false;
					OnResize();
				}
				else if (dataPack.settings.mResizing)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or dataPack.mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		dataPack.settings.mAppPaused = true;
		dataPack.settings.mResizing = true;
		timer.Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		dataPack.settings.mAppPaused = false;
		dataPack.settings.mResizing = false;
		timer.Start();
		OnResize();
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
		Input::Input::inputData[!Input::inputDataSwitcher].lMouseDown = true;
		return 0;
	case WM_MBUTTONDOWN:
		Input::Input::inputData[!Input::inputDataSwitcher].mMouseDown = true;
		return 0;
	case WM_RBUTTONDOWN:
		//OnRMouseDown(wParam);
		Input::Input::inputData[!Input::inputDataSwitcher].rMouseDown = true;
		return 0;
	case WM_LBUTTONUP:
		Input::Input::inputData[!Input::inputDataSwitcher].lMouseUp = true;
		//OnLMouseUp(wParam);
		return 0;
	case WM_MBUTTONUP:
		Input::Input::inputData[!Input::inputDataSwitcher].mMouseUp = true;
		//OnMMouseUp(wParam);
		return 0;
	case WM_RBUTTONUP:
		Input::Input::inputData[!Input::inputDataSwitcher].mMouseUp = true;
		//OnRMouseUp(wParam);
		return 0;
	case WM_MOUSEMOVE:
	{
		Input::inputData[!Input::inputDataSwitcher].mouseState = wParam;
		int2 mouseMove = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		Input::OnMoveMouse({ mouseMove.x, mouseMove.y });
	}
	return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		wParam = Min<uint>(wParam, 105);
		Input::Input::inputData[!Input::inputDataSwitcher].keyUpArray[wParam] = true;
		return 0;
	case WM_KEYDOWN:
		wParam = Min<uint>(wParam, 105);
		Input::Input::inputData[!Input::inputDataSwitcher].keyDownArray[wParam] = true;
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
HWND      mhMainWnd = nullptr; // main window handle
bool D3DApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = dataPack.settings.mhAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, dataPack.settings.mClientWidth, dataPack.settings.mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"MainWnd", L"Virtual Engine",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, dataPack.settings.mhAppInst, 0);
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

bool D3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dataPack.mdxgiFactory)));

	// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&dataPack.md3dDevice));

	// Fallback to WARP device.
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(dataPack.mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&dataPack.md3dDevice)));
	}

	ThrowIfFailed(dataPack.md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&dataPack.mFence)));

	dataPack.settings.mRtvDescriptorSize = dataPack.md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dataPack.settings.mDsvDescriptorSize = dataPack.md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	dataPack.settings.mCbvSrvUavDescriptorSize = dataPack.md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.


#ifdef _DEBUG
	LogAdapters();
#endif

	CreateCommandObjects();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();

	return true;
}

void D3DApp::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(dataPack.md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&dataPack.mCommandQueue)));
}

void D3DApp::CreateSwapChain()
{
	// Release the previous swapchain we will be recreating.
	if (dataPack.mSwapChain)
	{
		dataPack.mSwapChain->Release();
		dataPack.mSwapChain = nullptr;
	}

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = dataPack.settings.mClientWidth;
	sd.BufferDesc.Height = dataPack.settings.mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = dataPack.BACK_BUFFER_FORMAT;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = dataPack.SwapChainBufferCount;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT r = (dataPack.mdxgiFactory->CreateSwapChain(
		dataPack.mCommandQueue.Get(),
		&sd,
		&dataPack.mSwapChain));
	size_t t = 0;
}
#include "../Singleton/FrameResource.h"

void D3DApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((timer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);

		std::wstring windowText = L"Virtual Engine";
		windowText +=
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr + L" ";
		if (dataPack.settings.windowInfo) windowText += dataPack.settings.windowInfo;

		SetWindowText(mhMainWnd, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void D3DApp::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (dataPack.mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, dataPack.BACK_BUFFER_FORMAT);

		ReleaseCom(output);

		++i;
	}
}

void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// Call with nullptr to get list count.
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}
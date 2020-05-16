#pragma once
#include "../Common/d3dUtil.h"
#include <mutex>
#include "../Common/RingQueue.h"
class TransitionBarrierBuffer;
class FrameResource;
class RenderCommand
{
private:
	static std::mutex mtx;
	static RingQueue<RenderCommand*> queue;
	static RingQueue<RenderCommand*> loadingQueue;
public:
	virtual ~RenderCommand() {}
	virtual void operator()(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource,
		TransitionBarrierBuffer*) = 0;
	static void AddCommand(RenderCommand* command);
	static void AddLoadingCommand(RenderCommand* command);
	static bool ExecuteCommand(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource,
		TransitionBarrierBuffer*);
	static bool ExecuteLoadingCommand(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource,
		TransitionBarrierBuffer*);
};
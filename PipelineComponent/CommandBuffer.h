#pragma once
#include "../Common/d3dUtil.h"
class CommandBuffer
{
private:
	ID3D12CommandQueue* graphicsCommandQueue;
	ID3D12CommandQueue* computeCommandQueue;
	ID3D12CommandQueue* copyCommandQueue;
	std::vector<ID3D12GraphicsCommandList*> graphicsCmdLists;
	struct Fence
	{
		ID3D12Fence* fence;
		UINT64 frameIndex;
	};
	struct ExecuteCommand
	{
		UINT start;
		UINT count;
	};
	struct InnerCommand
	{
		enum CommandType
		{
			CommandType_ExecuteGraphics,
			CommandType_ExecuteCompute,
			CommandType_ExecuteCopy,
			CommandType_WaitCompute,
			CommandType_SignalCompute,
			CommandType_WaitGraphics,
			CommandType_SignalGraphics,
			CommandType_WaitCopy,
			CommandType_SignalCopy
		};
		CommandType type;
		union
		{
			UINT executeIndex;
			Fence waitFence;
			Fence signalFence;
		};
	};
	std::vector<InnerCommand> executeCommands;
public:
	ID3D12CommandQueue* GetGraphicsQueue() const {
		return graphicsCommandQueue;
	}
	ID3D12CommandQueue* GetComputeQueue() const {
		return computeCommandQueue;
	}
	ID3D12CommandQueue* GetCopyQueue() const
	{
		return copyCommandQueue;
	}
	void WaitForCompute(ID3D12Fence* computeFence, UINT currentFrame);
	void WaitForGraphics(ID3D12Fence* graphicsFence, UINT currentFrame);
	void SignalToCompute(ID3D12Fence* computeFence, UINT currentFrame);
	void SignalToGraphics(ID3D12Fence* graphicsFence, UINT currentFrame);
	void WaitForCopy(ID3D12Fence* graphicsFence, UINT currentFrame);
	void SignalToCopy(ID3D12Fence* computeFence, UINT currentFrame);
	void ExecuteGraphicsCommandList(ID3D12GraphicsCommandList* commandList);
	void ExecuteComputeCommandList(ID3D12GraphicsCommandList* commandList);
	void ExecuteCopyCommandList(ID3D12GraphicsCommandList* commandList);
	void Submit();
	void SubmitOnlyCopy();
	void Clear();
	CommandBuffer(
		ID3D12CommandQueue* graphicsCommandQueue,
		ID3D12CommandQueue* computeCommandQueue,
		ID3D12CommandQueue* copyCommandQueue
	);
};
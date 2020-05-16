#include "RenderCommand.h"
#include "../Singleton/FrameResource.h"
std::mutex RenderCommand::mtx;
RingQueue<RenderCommand*> RenderCommand::queue(100);
RingQueue<RenderCommand*> RenderCommand::loadingQueue(100);

void RenderCommand::AddCommand(RenderCommand* command)
{
	std::lock_guard<std::mutex> lck(mtx);
	queue.EmplacePush(command);
}

void RenderCommand::AddLoadingCommand(RenderCommand* command)
{
	std::lock_guard<std::mutex> lck(mtx);
	loadingQueue.EmplacePush(command);
}


bool RenderCommand::ExecuteCommand(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	FrameResource* resource,
	TransitionBarrierBuffer* barrierBuffer)
{
	RenderCommand* ptr = nullptr;
	bool v = false;
	{
		std::lock_guard<std::mutex> lck(mtx);
		v = queue.TryPop(&ptr);
	}
	if (!v) return false;
	PointerKeeper<RenderCommand> keeper(ptr);
	(*ptr)(device, commandList, resource, barrierBuffer);
	return true;
}

bool RenderCommand::ExecuteLoadingCommand(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	FrameResource* resource,
	TransitionBarrierBuffer* barrierBuffer)
{
	RenderCommand* ptr = nullptr;
	bool v = false;
	{
		std::lock_guard<std::mutex> lck(mtx);
		v = loadingQueue.TryPop(&ptr);
	}
	if (!v) return false;
	PointerKeeper<RenderCommand> keeper(ptr);
	(*ptr)(device, commandList, resource, barrierBuffer);
	return true;
}
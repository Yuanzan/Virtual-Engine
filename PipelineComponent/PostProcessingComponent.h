#pragma once
#include "PipelineComponent.h"
#include "PipelineJsonData.h"
class PostRunnable;
class PrepareComponent;
class PostProcessingComponent : public PipelineComponent
{
	friend class PostRunnable;
protected:
	StackObject<PipelineJsonData> jsonData;
	std::vector<TemporalResourceCommand> tempRT;
	PrepareComponent* prepareComp;
	virtual CommandListType GetCommandListType() {
		return CommandListType_Graphics;
	}
	virtual std::vector<TemporalResourceCommand>& SendRenderTextureRequire(const EventData& evt);
	virtual void RenderEvent(const EventData& data, ThreadCommand* commandList);
public:
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
};
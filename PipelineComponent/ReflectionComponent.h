#pragma once
#include "PipelineComponent.h"
#include "../RenderComponent/RenderComponentInclude.h"
#include "../RenderComponent/ReflectionData.h"
class ReflectionFrameData;
class ReflectionRunnable;
class ReflectionProbe;
class ReflectionComponent final : public PipelineComponent
{
	friend class ReflectionFrameData;
	friend class ReflectionRunnable;
protected:
	uint ReflectionProbeData;
	uint TextureIndices;
	uint _CameraGBuffer0;
	uint _CameraGBuffer1;
	uint _CameraGBuffer2;
	uint _Cubemap;
	bool enableDiffuseGI = true;
	StackObject<CBufferPool> reflectionDataPool;
	std::vector<TemporalResourceCommand> tempRT;
	std::vector<ReflectionProbe*> culledReflectionProbes;
	virtual CommandListType GetCommandListType() {
		return CommandListType_Graphics;
	}
	virtual std::vector<TemporalResourceCommand>& SendRenderTextureRequire(const EventData& evt);
	virtual void RenderEvent(const EventData& data, ThreadCommand* commandList);
public:
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
};
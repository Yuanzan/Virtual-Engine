#pragma once
#include "../Common/MetaLib.h"
#include <unordered_map>
#include "../Singleton/Graphics.h"
class ThreadCommand;
class TransitionBarrierBuffer
{
private:
	friend class ThreadCommand;
	std::vector<D3D12_RESOURCE_BARRIER> commands;
	struct Command
	{
		D3D12_RESOURCE_STATES targetState;
		int32_t index;
		Command()
		{

		}
		Command(D3D12_RESOURCE_STATES targetState,
			int32_t index) :
			targetState(targetState),
			index(index) {}
	};
	Dictionary<ID3D12Resource*, Command> barrierRecorder;
	std::unordered_map<ID3D12Resource*, bool> uavBarriersDict;
	std::vector<ID3D12Resource*> uavBarriers;
	void KillSame();
	void Clear(ID3D12GraphicsCommandList* commandList);
	
public:
	TransitionBarrierBuffer();
	void RegistInitState(D3D12_RESOURCE_STATES initState, ID3D12Resource* resource);
	void UpdateState(D3D12_RESOURCE_STATES newState, ID3D12Resource* resource);
	void AddCommand(D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState, ID3D12Resource* resource);
	void ExecuteCommand(ID3D12GraphicsCommandList* commandList);
	void UAVBarrier(ID3D12Resource*);
	void UAVBarriers(const std::initializer_list<ID3D12Resource*>&);
};
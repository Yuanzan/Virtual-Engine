#pragma once
#include "../../Common/d3dUtil.h"
class Shader;
struct MultiDrawCommand
{
	D3D12_GPU_VIRTUAL_ADDRESS objectCBufferAddress; // Object Constant Buffer Address
	D3D12_VERTEX_BUFFER_VIEW vertexBuffer;			// Vertex Buffer Address
	D3D12_INDEX_BUFFER_VIEW indexBuffer;			//Index Buffer Address
	D3D12_DRAW_INDEXED_ARGUMENTS drawArgs;			//Draw Arguments
};

class CommandSignature
{
private:
	Microsoft::WRL::ComPtr<ID3D12CommandSignature> mCommandSignature;
	const Shader* mShader;
public:
	CommandSignature(const Shader* shader, ID3D12Device* device);
	ID3D12CommandSignature* GetSignature() const { return mCommandSignature.Get(); }
	const Shader* GetShader() const { return mShader; }
};
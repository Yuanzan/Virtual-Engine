#pragma once
typedef unsigned int UINT;
typedef unsigned int uint;
#include <d3d12.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
namespace SCompile {
	struct ComputeShaderVariable
	{
		enum Type
		{
			ConstantBuffer, StructuredBuffer, RWStructuredBuffer, SRVDescriptorHeap, UAVDescriptorHeap
		};
		std::string name;
		Type type;
		UINT tableSize;
		UINT registerPos;
		UINT space;
		ComputeShaderVariable() {}
		ComputeShaderVariable(
			const std::string& name,
			Type type,
			UINT tableSize,
			UINT registerPos,
			UINT space
		) : name(name),
			type(type),
			tableSize(tableSize),
			registerPos(registerPos),
			space(space)
		{}
	};
	struct Pass
	{
		std::vector<char> vsShaderr;
		std::vector<char> psShader;
		D3D12_RASTERIZER_DESC rasterizeState;
		D3D12_DEPTH_STENCIL_DESC depthStencilState;
		D3D12_BLEND_DESC blendState;
	};

	enum ShaderVariableType
	{
		ShaderVariableType_ConstantBuffer,
		ShaderVariableType_SRVDescriptorHeap,
		ShaderVariableType_UAVDescriptorHeap,
		ShaderVariableType_StructuredBuffer
	};
	struct KernelDescriptor
	{
		std::string name;
		std::shared_ptr<std::vector<std::string>> macros;
	};
	struct ShaderVariable
	{
		std::string name;
		ShaderVariableType type;
		UINT tableSize;
		UINT registerPos;
		UINT space;
		ShaderVariable() {}
		ShaderVariable(
			const std::string& name,
			ShaderVariableType type,
			UINT tableSize,
			UINT registerPos,
			UINT space
		) : name(name),
			type(type),
			tableSize(tableSize),
			registerPos(registerPos),
			space(space) {}
	};
	struct PassDescriptor
	{
		std::string name;
		std::string vertex;
		std::string fragment;
		std::string hull;
		std::string domain;
		std::shared_ptr<std::vector<std::string>> macros;
		D3D12_RASTERIZER_DESC rasterizeState;
		D3D12_DEPTH_STENCIL_DESC depthStencilState;
		D3D12_BLEND_DESC blendState;
	};
	struct ShaderUniform
	{
		static std::unordered_map<std::string, uint> uMap;
		//Change Here to Add New Command
		static void Init();
		static void GetShaderRootSigData(const std::string& path, std::vector<ShaderVariable>& vars, std::vector<PassDescriptor>& GetPasses);
		static void GetComputeShaderRootSigData(const std::string& path, std::vector<ComputeShaderVariable>& vars, std::vector<KernelDescriptor>& GetPasses);
	};
}
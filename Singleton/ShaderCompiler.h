#pragma once
#include "../Common/d3dUtil.h"
class JobSystem;
class Shader;
class ComputeShader;
class ID3D12Device;
class MaterialManager;
class ShaderCompiler
{
private:
	static int shaderIDCount;
	static std::unordered_map<std::string, ComputeShader*> mComputeShaders;
	static std::unordered_map<std::string, Shader*> mShaders;
public:
	static Shader const* GetShader(const std::string& name);
	static ComputeShader const* GetComputeShader(const std::string& name);
	static void AddShader(const std::string& str, Shader* shad);
	static void AddComputeShader(const std::string& str, ComputeShader* shad);
	static void Init(ID3D12Device* device, JobSystem* jobSys);
	static void Dispose();
};
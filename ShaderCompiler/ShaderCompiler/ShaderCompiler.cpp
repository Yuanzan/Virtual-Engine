// ShaderCompiler.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#define _CRT_SECURE_NO_WARNINGS
#include "ICompileIterator.h"
#include "DirectCompile.h"
#include "StringUtility.h"
#include  <stdio.h>
#include  <io.h>
#include <vector>
#include "BatchCompile.h"
#include <fstream>
#include <unordered_map>
#include <string>
#include "ShaderUniforms.h"
#include "JobSystem/JobInclude.h"
#include <atomic>
#include <windows.h> 
#include <tchar.h>
#include <strsafe.h>

using namespace std;
using namespace SCompile;

static bool g_needCommandOutput = true;

void CreateChildProcess(const std::string& cmd)
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{
	if (g_needCommandOutput)
	{
		cout << cmd << endl;
		system(cmd.c_str());
		return;
	}
	HANDLE g_hChildStd_IN_Rd = NULL;
	HANDLE g_hChildStd_IN_Wr = NULL;
	HANDLE g_hChildStd_OUT_Rd = NULL;
	HANDLE g_hChildStd_OUT_Wr = NULL;
	PROCESS_INFORMATION piProcInfo;

	static HANDLE g_hInputFile = NULL;
	std::wstring ws;
	ws.resize(cmd.length());
	for (uint i = 0; i < cmd.length(); ++i)
	{
		ws[i] = cmd[i];
	}

	//PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE;

	// Set up members of the PROCESS_INFORMATION structure. 

	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Create the child process. 

	bSuccess = CreateProcess(NULL,
		ws.data(),     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 

	 // If an error occurs, exit the application. 
	if (bSuccess)
	{
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 
		WaitForSingleObject(piProcInfo.hProcess, INFINITE);
		WaitForSingleObject(piProcInfo.hThread, INFINITE);
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
		// Close handles to the stdin and stdout pipes no longer needed by the child process.
		// If they are not explicitly closed, there is no way to recognize that the child process has ended.

		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(g_hChildStd_IN_Rd);
	}
}


string start = " /nologo /enable_unbounded_descriptor_tables";
string shaderTypeCmd = " /T ";
string fullOptimize = " /O3";
string funcName = " /E ";
string output = " /Fo ";
string macro_compile = " /D ";
string dxcversion;
string dxcpath;
string fxcversion;
string fxcpath;
void InitRegisteData()
{
	ifstream ifs("DXC\\register.txt");
	if (!ifs)
	{
		cout << "Register.txt not found in DXC folder!" << endl;
		system("pause");
		exit(0);
	}
	vector<string> lines;
	ReadLines(ifs, lines);
	vector<string> splitResult;
	for (auto ite = lines.begin(); ite != lines.end(); ++ite)
	{
		Split(*ite, ':', splitResult);
		if (splitResult.size() == 2)
		{
			ToUpper(splitResult[0]);
			if (splitResult[0] == "DXCSM")
			{
				dxcversion = splitResult[1];
			}
			else if (splitResult[0] == "DXCPATH")
			{
				dxcpath = "DXC\\" + splitResult[1];
			}
			else if (splitResult[0] == "FXCSM")
			{
				fxcversion = splitResult[1];
			}
			else if (splitResult[0] == "FXCPATH")
			{
				fxcpath = "DXC\\" + splitResult[1];
			}
		}
	}
}
struct CompileFunctionCommand
{
	std::string name;
	ShaderType type;
	std::shared_ptr<std::vector<std::string>> macros;
};
void GenerateDXCCommand(
	const string& fileName,
	const string& functionName,
	const string& resultFileName,
	const std::shared_ptr<std::vector<string>>& macros,
	ShaderType shaderType,
	string& cmdResult
) {
	string shaderTypeName;
	switch (shaderType)
	{
	case ShaderType::ComputeShader:
		shaderTypeName = "cs_";
		break;
	case ShaderType::VertexShader:
		shaderTypeName = "vs_";
		break;
	case ShaderType::HullShader:
		shaderTypeName = "hs_";
		break;
	case ShaderType::DomainShader:
		shaderTypeName = "ds_";
		break;
	case ShaderType::GeometryShader:
		shaderTypeName = "gs_";
		break;
	case ShaderType::PixelShader:
		shaderTypeName = "ps_";
		break;
	default:
		shaderTypeName = " ";
		break;
	}
	shaderTypeName += dxcversion;
	cmdResult.clear();
	cmdResult.reserve(50);
	cmdResult += dxcpath + start + shaderTypeCmd + shaderTypeName + fullOptimize;
	if (macros && !macros->empty())
	{
		cmdResult += macro_compile;
		for (auto ite = macros->begin(); ite != macros->end(); ++ite)
		{
			cmdResult += *ite + "=1 ";
		}
	}
	cmdResult += funcName + functionName + output + '\"' + resultFileName + '\"' + " " + '\"' + fileName + '\"';
}
void GenerateFXCCommand(
	const string& fileName,
	const string& functionName,
	const string& resultFileName,
	const std::shared_ptr<std::vector<string>>& macros,
	ShaderType shaderType,
	string& cmdResult
) {
	string shaderTypeName;
	switch (shaderType)
	{
	case ShaderType::ComputeShader:
		shaderTypeName = "cs_";
		break;
	case ShaderType::VertexShader:
		shaderTypeName = "vs_";
		break;
	case ShaderType::HullShader:
		shaderTypeName = "hs_";
		break;
	case ShaderType::DomainShader:
		shaderTypeName = "ds_";
		break;
	case ShaderType::GeometryShader:
		shaderTypeName = "gs_";
		break;
	case ShaderType::PixelShader:
		shaderTypeName = "ps_";
		break;
	default:
		shaderTypeName = " ";
		break;
	}
	shaderTypeName += fxcversion;
	cmdResult.clear();
	cmdResult.reserve(50);
	cmdResult += fxcpath + start + shaderTypeCmd + shaderTypeName + fullOptimize;
	if (macros && !macros->empty())
	{
		cmdResult += macro_compile;
		for (auto ite = macros->begin(); ite != macros->end(); ++ite)
		{
			cmdResult += *ite + "=1 ";
		}
	}
	cmdResult += funcName + functionName + output + '\"' + resultFileName + '\"' + " " + '\"' + fileName + '\"';
}

template <typename T>
void PutIn(std::vector<char>& c, const T& data)
{
	T* cc = &((T&)data);
	size_t siz = c.size();
	c.resize(siz + sizeof(T));
	memcpy(c.data() + siz, cc, sizeof(T));
}
template <>
void PutIn<string>(std::vector<char>& c, const string& data)
{
	PutIn<uint>(c, (uint)data.length());
	size_t siz = c.size();
	c.resize(siz + data.length());
	memcpy(c.data() + siz, data.data(), data.length());
}
template <typename T>
void DragData(ifstream& ifs, T& data)
{
	ifs.read((char*)&data, sizeof(T));
}
template <>
void DragData<string>(ifstream& ifs, string& str)
{
	uint32_t length = 0;
	DragData<uint32_t>(ifs, length);
	str.clear();
	str.resize(length);
	ifs.read(str.data(), length);
}
struct PassFunction
{
	std::string name;
	std::shared_ptr<std::vector<std::string>> macros;
	PassFunction(const std::string& name,
		const std::shared_ptr<std::vector<std::string>>& macros) :
		name(name),
		macros(macros) {}
	PassFunction() {}
	bool operator==(const PassFunction& p) const noexcept
	{
		bool cur = macros.operator bool();
		bool pCur = p.macros.operator bool();
		if (name == p.name)
		{
			if ((!pCur && !cur))
			{
				return true;
			}
			else if (cur && pCur && macros->size() == p.macros->size())
			{
				for (uint i = 0; i < macros->size(); ++i)
					if ((*macros)[i] != (*p.macros)[i]) return false;
				return true;
			}
		}
		return false;
	}
	bool operator!=(const PassFunction& p) const noexcept
	{
		return !operator==(p);
	}
};
struct PassFunctionHash
{
	size_t operator()(const PassFunction& pf) const noexcept
	{
		hash<std::string> hashStr;
		size_t h = hashStr(pf.name);
		if (pf.macros)
		{
			for (uint i = 0; i < pf.macros->size(); ++i)
			{
				h <<= 4;
				h ^= hashStr((*pf.macros)[i]);
			}
		}
		return h;
	}
};
void CompileShader(
	const string& fileName,
	const string& propertyPath,
	const string& tempFilePath,
	std::vector<char>& resultData,
	bool isDebug)
{
	vector<ShaderVariable> vars;
	vector<PassDescriptor> passDescs;
	ShaderUniform::GetShaderRootSigData(propertyPath, vars, passDescs);

	resultData.clear();
	resultData.reserve(32768);
	PutIn<uint>(resultData, (uint)vars.size());
	for (auto i = vars.begin(); i != vars.end(); ++i)
	{
		PutIn<string>(resultData, i->name);
		PutIn<ShaderVariableType>(resultData, i->type);
		PutIn<uint>(resultData, i->tableSize);
		PutIn<uint>(resultData, i->registerPos);
		PutIn<uint>(resultData, i->space);
	}
	auto func = [&](const string& command, uint64_t& fileSize)->void
	{
		//TODO
		CreateChildProcess(command);
		//system(command.c_str());
		fileSize = 0;
		ifstream ifs(tempFilePath, ios::binary);
		if (!ifs) return;
		ifs.seekg(0, ios::end);
		fileSize = ifs.tellg();
		ifs.seekg(0, ios::beg);
		PutIn<uint64_t>(resultData, fileSize);
		if (fileSize == 0) return;
		size_t originSize = resultData.size();
		resultData.resize(fileSize + originSize);
		ifs.read(resultData.data() + originSize, fileSize);
	};
	std::unordered_map<PassFunction, std::pair<ShaderType, uint>, PassFunctionHash> passMap;
	passMap.reserve(passDescs.size() * 2);
	for (auto i = passDescs.begin(); i != passDescs.end(); ++i)
	{
		auto findFunc = [&](const std::string& namestr, const std::shared_ptr<std::vector<std::string>>& macros, ShaderType type)->void
		{
			PassFunction name(namestr, macros);
			if (name.name.empty()) return;
			auto ite = passMap.find(name);
			if (ite == passMap.end())
			{
				passMap.insert(std::pair<PassFunction, std::pair<ShaderType, uint>>(name, std::pair<ShaderType, uint>(type, (uint)passMap.size())));
			}
		};
		findFunc(i->vertex, i->macros, ShaderType::VertexShader);
		findFunc(i->hull, i->macros, ShaderType::HullShader);
		findFunc(i->domain, i->macros, ShaderType::DomainShader);
		findFunc(i->fragment, i->macros, ShaderType::PixelShader);
	}
	std::vector<CompileFunctionCommand> functionNames(passMap.size());
	PutIn<uint>(resultData, (uint)passMap.size());
	for (auto ite = passMap.begin(); ite != passMap.end(); ++ite)
	{
		CompileFunctionCommand cmd;
		cmd.macros = ite->first.macros;
		cmd.name = ite->first.name;
		cmd.type = ite->second.first;

		functionNames[ite->second.second] = cmd;
	}
	string commandCache;
	for (uint i = 0; i < functionNames.size(); ++i)
	{
		uint64_t fileSize = 0;
		if (isDebug)
		{
			GenerateFXCCommand(
				fileName, functionNames[i].name, tempFilePath,
				functionNames[i].macros, functionNames[i].type, commandCache);
		}
		else
		{
			GenerateDXCCommand(
				fileName, functionNames[i].name, tempFilePath,
				functionNames[i].macros, functionNames[i].type, commandCache);
		}
		func(commandCache, fileSize);
	}

	PutIn<uint>(resultData, (uint)passDescs.size());
	for (auto i = passDescs.begin(); i != passDescs.end(); ++i)
	{
		PutIn(resultData, i->rasterizeState);
		PutIn(resultData, i->depthStencilState);
		PutIn(resultData, i->blendState);
		auto PutInFunc = [&](const std::string& value, const std::shared_ptr<std::vector<std::string>>& macros)->void
		{
			PassFunction psf(value, macros);
			if (value.empty() || passMap.find(psf) == passMap.end())
				PutIn<int>(resultData, -1);
			else
				PutIn<int>(resultData, (int)passMap[psf].second);
		};
		PutInFunc(i->vertex, i->macros);
		PutInFunc(i->hull, i->macros);
		PutInFunc(i->domain, i->macros);
		PutInFunc(i->fragment, i->macros);
	}

}

void CompileComputeShader(
	const string& fileName,
	const string& propertyPath,
	const string& tempFilePath,
	std::vector<char>& resultData,
	bool isDebug)
{
	vector<ComputeShaderVariable> vars;
	vector<KernelDescriptor> passDescs;
	ShaderUniform::GetComputeShaderRootSigData(propertyPath, vars, passDescs);
	resultData.clear();
	resultData.reserve(32768);
	PutIn<uint>(resultData, (uint)vars.size());
	for (auto i = vars.begin(); i != vars.end(); ++i)
	{
		PutIn<string>(resultData, i->name);
		PutIn<ComputeShaderVariable::Type>(resultData, i->type);
		PutIn<uint>(resultData, i->tableSize);
		PutIn<uint>(resultData, i->registerPos);
		PutIn<uint>(resultData, i->space);
	}
	auto func = [&](const string& command, uint64_t& fileSize)->void
	{
		CreateChildProcess(command);
		//system(command.c_str());
		fileSize = 0;
		ifstream ifs(tempFilePath, ios::binary);
		if (!ifs) return;
		ifs.seekg(0, ios::end);
		fileSize = ifs.tellg();
		ifs.seekg(0, ios::beg);
		PutIn<uint64_t>(resultData, fileSize);
		if (fileSize == 0) return;
		size_t originSize = resultData.size();
		resultData.resize(fileSize + originSize);
		ifs.read(resultData.data() + originSize, fileSize);
	};
	PutIn<uint>(resultData, (uint)passDescs.size());
	for (auto i = passDescs.begin(); i != passDescs.end(); ++i)
	{
		uint64_t fileSize = 0;
		string kernelCommand;
		if (isDebug)
		{
			GenerateFXCCommand(
				fileName, i->name, tempFilePath, i->macros, ShaderType::ComputeShader, kernelCommand);
		}
		else
		{
			GenerateDXCCommand(
				fileName, i->name, tempFilePath, i->macros, ShaderType::ComputeShader, kernelCommand);
		}
		func(kernelCommand, fileSize);
	}
}
void getFiles(string path, vector<string>& files)
{
	uint64_t   hFile = 0;
	struct _finddata_t fileinfo;
	string p;
	if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			if ((fileinfo.attrib & _A_SUBDIR))
			{
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					getFiles(p.assign(path).append("\\").append(fileinfo.name), files);
			}
			else
			{
				files.push_back(p.assign(path).append("\\").append(fileinfo.name));
			}
		} while (_findnext(hFile, &fileinfo) == 0);

		_findclose(hFile);
	}
}

typedef unsigned int uint;
void TryCreateDirectory(string& path)
{
	vector<uint> slashIndex;
	slashIndex.reserve(20);
	for (uint i = 0; i < path.length(); ++i)
	{
		if (path[i] == '/' || path[i] == '\\')
		{
			slashIndex.push_back(i);
			path[i] = '\\';
		}
	}
	if (slashIndex.empty()) return;
	string command;
	command.reserve(slashIndex[slashIndex.size() - 1] + 3);
	uint startIndex = 0;
	for (uint i = 0; i < slashIndex.size(); ++i)
	{
		uint value = slashIndex[i];
		for (uint x = startIndex; x < value; ++x)
		{
			command += path[x];
		}
		if (_access(command.data(), 0) == -1)
		{
			std::system(("md " + command).data());
		}
		startIndex = slashIndex[i];
	}
}

int main()
{
	ShaderUniform::Init();
	system("@echo off");
	string cmd;
	string sonCmd;
	string results;
	unique_ptr<ICompileIterator> dc;
	vector<Command>* cmds = nullptr;
	while (true)
	{
		cout << "Choose Compiling Mode: \n";
		cout << "  0: Compile Single File\n";
		cout << "  1: Compile Batched File\n";
		std::cin >> cmd;
		if (StringEqual(cmd, "exit"))
		{

			return 0;
		}
		else if (cmd.size() == 1) {

			if (cmd[0] == '0')
			{
				dc = std::unique_ptr<ICompileIterator>(new DirectCompile());
			}
			else if (cmd[0] == '1')
			{
				dc = std::unique_ptr<ICompileIterator>(new BatchCompile());
			}
			else
				dc = nullptr;

			if (dc)
			{
				dc->UpdateCommand();
				cout << "\n\n\n";
				cmds = &dc->GetCommand();
				if (cmds->empty()) continue;
			}
		EXECUTE:
			InitRegisteData();
			static string pathFolder = "CompileResult\\";

			atomic<uint64_t> counter = 0;
			if (cmds->size() > 1)
			{
				g_needCommandOutput = false;
				JobSystem* jobSys_MainGlobal;
				JobBucket* jobBucket_MainGlobal;
				uint threadCount = min(std::thread::hardware_concurrency() * 4, cmds->size());
				jobSys_MainGlobal = new JobSystem(threadCount);
				jobBucket_MainGlobal = jobSys_MainGlobal->GetJobBucket();

				for (size_t a = 0; a < cmds->size(); ++a)
				{
					Command* i = &(*cmds)[a];
					jobBucket_MainGlobal->GetTask([i, &counter]()->void
						{
							std::vector<char> outputData;
							std::string temp;
							temp.reserve(20);
							temp += ".temp";
							uint64_t tempNameIndex = ++counter;
							temp += std::to_string((uint64_t)tempNameIndex);
							temp += ".tempCso";

							uint32_t maxSize = 0;
							if (i->isCompute)
							{
								CompileComputeShader(i->fileName, i->propertyFileName, temp, outputData, i->isDebug);
							}
							else
							{
								CompileShader(i->fileName, i->propertyFileName, temp, outputData, i->isDebug);
							}
							string filePath = pathFolder + i->fileName + ".cso";
							TryCreateDirectory(filePath);
							ofstream ofs(filePath, ios::binary);
							ofs.write(outputData.data(), outputData.size());
							remove(temp.c_str());
						}, nullptr, 0);
				}
				cout << endl;

				jobSys_MainGlobal->ExecuteBucket(jobBucket_MainGlobal, 1);
				jobSys_MainGlobal->Wait();
				jobSys_MainGlobal->ReleaseJobBucket(jobBucket_MainGlobal);
				delete jobSys_MainGlobal;
			}
			else
			{
				g_needCommandOutput = true;
				std::vector<char> outputData;
				std::string temp = ".temp.cso";
				uint32_t maxSize = 0;
				Command* i = &(*cmds)[0];
				if (i->isCompute)
				{
					CompileComputeShader(i->fileName, i->propertyFileName, temp, outputData, i->isDebug);
				}
				else
				{
					CompileShader(i->fileName, i->propertyFileName, temp, outputData, i->isDebug);
				}
				string filePath = pathFolder + i->fileName + ".cso";
				TryCreateDirectory(filePath);
				ofstream ofs(filePath, ios::binary);
				ofs.write(outputData.data(), outputData.size());
				remove(temp.c_str());
			}
			cout << "\n\n\n";
			cout << "Want to repeat the command again? Y for true\n";
			std::cin >> sonCmd;
			cout << "\n\n\n";
			if (sonCmd.length() == 1 && (sonCmd[0] == 'y' || sonCmd[0] == 'Y'))
			{
				goto EXECUTE;
			}
		}
	}
}

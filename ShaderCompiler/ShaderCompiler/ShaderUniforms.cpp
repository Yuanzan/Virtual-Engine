#include "ShaderUniforms.h"
#include <fstream>
#include <d3d12.h>
#include "d3dx12.h"
#include "StringUtility.h"
using namespace std;
namespace SCompile
{
	enum class DefineType : uint
	{
		RWTexture,
		RWStructuredBuffer,
		CBuffer,
		Texture,
		StructuredBuffer,
		Pragma,
		EndPragma,
		Vertex,
		Fragment,
		Hull,
		Domain,
		Cull,
		ZWrite,
		ZTest,
		Conservative,
		Blend,
		Stencil_Fail,
		Stencil_ZFail,
		Stencil_Pass,
		Stencil_Comp,
		Stencil_Readmask,
		Stencil_Writemask,
		Less,
		LEqual,
		Equal,
		GEqual,
		Greater,
		NEqual,
		Never,
		Macro
	};
	string rwtex = "RWT";
	string rwstr = "RWS";
	string cbuffer = "cbuffer";
	string tex = "Tex";
	string str = "Str";
	string pragma = "#pragma";
	string endPragma = "#end";
	string vertex = "vertex";
	string fragment = "fragment";
	string hull = "hull";
	string domain = "domain";
	string Cull = "cull";
	string zWrite = "zwrite";
	string zTest = "ztest";
	string conservative = "conservative";
	string blend = "blend";
	string stencilFail = "stencil_fail";
	string stencilZFail = "stencil_zfail";
	string stencilpass = "stencil_pass";
	string stencilcomp = "stencil_comp";
	string stencilreadmask = "stencil_readmask";
	string stencilwritemask = "stencil_writemask";
	void ShaderUniform::Init()
	{
		uMap.reserve(101);
#define ADD(str, cmd) uMap.insert(std::pair<std::string, uint>((str), (uint)(cmd)))

		ADD(rwtex, DefineType::RWTexture);
		ADD(rwstr, DefineType::RWStructuredBuffer);
		ADD(cbuffer, DefineType::CBuffer);
		ADD(tex, DefineType::Texture);
		ADD(str, DefineType::StructuredBuffer);
		ADD(pragma, DefineType::Pragma);
		ADD(endPragma, DefineType::EndPragma);
		ADD(vertex, DefineType::Vertex);
		ADD(fragment, DefineType::Fragment);
		ADD(hull, DefineType::Hull);
		ADD(domain, DefineType::Domain);
		ADD(Cull, DefineType::Cull);
		ADD(zWrite, DefineType::ZWrite);
		ADD(zTest, DefineType::ZTest);
		ADD(conservative, DefineType::Conservative);
		ADD(blend, DefineType::Blend);
		ADD(stencilFail, DefineType::Stencil_Fail);
		ADD(stencilZFail, DefineType::Stencil_ZFail);
		ADD(stencilpass, DefineType::Stencil_Pass);
		ADD(stencilcomp, DefineType::Stencil_Comp);
		ADD(stencilreadmask, DefineType::Stencil_Readmask);
		ADD(stencilwritemask, DefineType::Stencil_Writemask);
		ADD("less", DefineType::Less);
		ADD("lequal", DefineType::LEqual);
		ADD("equal", DefineType::Equal);
		ADD("gequal", DefineType::GEqual);
		ADD("greater", DefineType::Greater);
		ADD("nequal", DefineType::NEqual);
		ADD("never", DefineType::Never);
		ADD("macro", DefineType::Macro);
	}

	std::unordered_map<std::string, uint> ShaderUniform::uMap;
	enum class BlendState : uint
	{
		One = 0,
		Alpha = 1,
		Off = 2
	};
	D3D12_BLEND_DESC GetBlendState(BlendState alphaBlend)
	{
		switch (alphaBlend)
		{
		case BlendState::One:
		{
			D3D12_BLEND_DESC blendDesc;
			blendDesc.IndependentBlendEnable = FALSE;
			blendDesc.AlphaToCoverageEnable = FALSE;


			D3D12_RENDER_TARGET_BLEND_DESC desc;
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.SrcBlend = D3D12_BLEND_ONE;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlend = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ONE;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE;

			for (uint i = 0; i < 8; ++i)
			{
				blendDesc.RenderTarget[i] = desc;
			}
			return blendDesc;
		}
		break;
		case BlendState::Alpha:
		{
			D3D12_BLEND_DESC blendDesc;
			blendDesc.IndependentBlendEnable = FALSE;
			blendDesc.AlphaToCoverageEnable = FALSE;


			D3D12_RENDER_TARGET_BLEND_DESC desc;
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.SrcBlendAlpha = D3D12_BLEND_ZERO;
			desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			desc.DestBlendAlpha = D3D12_BLEND_ONE;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE;

			for (uint i = 0; i < 8; ++i)
			{
				blendDesc.RenderTarget[i] = desc;
			}
			return blendDesc;
		}
		break;
		default:
		{
			return CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		}
		break;
		}
	}

	D3D12_STENCIL_OP GetStencilOP(const std::string& str)
	{
		D3D12_STENCIL_OP op = D3D12_STENCIL_OP_KEEP;
		if (StringEqual(str, "zero")) op = D3D12_STENCIL_OP_ZERO;
		else if (StringEqual(str, "replace")) op = D3D12_STENCIL_OP_REPLACE;
		return op;
	}

	D3D12_DEPTH_STENCIL_DESC GetDepthState(bool zwrite, D3D12_COMPARISON_FUNC compareFunc, uint8_t readmask, uint8_t writemask,
		D3D12_STENCIL_OP sFail, D3D12_STENCIL_OP zFail, D3D12_STENCIL_OP pass, D3D12_COMPARISON_FUNC sComp)
	{
		D3D12_DEPTH_STENCIL_DESC dsDesc;
		if (!zwrite && compareFunc == D3D12_COMPARISON_FUNC_ALWAYS)
		{
			dsDesc.DepthEnable = FALSE;
			dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		}
		else
		{
			dsDesc.DepthEnable = TRUE;
			dsDesc.DepthWriteMask = zwrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		}

		dsDesc.DepthFunc = compareFunc;
		if (
			sFail == D3D12_STENCIL_OP_KEEP &&
			zFail == D3D12_STENCIL_OP_KEEP &&
			pass == D3D12_STENCIL_OP_KEEP &&
			sComp == D3D12_COMPARISON_FUNC_ALWAYS)
		{
			dsDesc.StencilEnable = FALSE;
		}
		else
		{
			dsDesc.StencilEnable = TRUE;
		}
		dsDesc.StencilReadMask = readmask;
		dsDesc.StencilWriteMask = writemask;
		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
		{ sFail, zFail, pass, sComp };
		dsDesc.FrontFace = defaultStencilOp;
		dsDesc.BackFace = defaultStencilOp;
		return dsDesc;
	}

	D3D12_RASTERIZER_DESC GetCullState(D3D12_CULL_MODE cullMode, bool enableConservativeRaster = false)
	{
		D3D12_RASTERIZER_DESC desc;
		desc.FillMode = D3D12_FILL_MODE_SOLID;
		desc.CullMode = cullMode;
		desc.FrontCounterClockwise = FALSE;
		desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		desc.DepthClipEnable = TRUE;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;
		desc.ForcedSampleCount = 0;
		desc.ConservativeRaster = enableConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		return desc;
	}

	void GetVarData(const string& s, bool useCount, char registerType, uint& count, uint& regis, uint& space, string& name)
	{
		string data;
		space = 0;
		regis = 0;
		count = 1;
		if (useCount)
		{
			GetDataFromAttribute(s, data);
			if (!data.empty())
				count = StringToInt(data);
		}
		GetDataFromBrackets(s, data);
		int tIndices = GetFirstIndexOf(data, registerType);
		string regisStr;
		regisStr.reserve(3);
		if (tIndices >= 0)
		{
			for (int i = tIndices + 1; i < data.length() && data[i] != ',' && data[i] != ' '; ++i)
			{
				regisStr.push_back(data[i]);
			}
			if (!regisStr.empty())
				regis = StringToInt(regisStr);
			else regis = 0;
		}
		regisStr.clear();
		tIndices = GetFirstIndexOf(data, "space");
		if (tIndices >= 0)
		{
			for (int i = tIndices + 5; i < data.length() && data[i] != ',' && data[i] != ' '; ++i)
			{
				regisStr.push_back(data[i]);
			}
			if (!regisStr.empty())
				space = StringToInt(regisStr);
		}
		tIndices = GetFirstIndexOf(s, ' ');
		name.clear();
		if (tIndices >= 0)
		{
			name.reserve(10);
			for (uint i = tIndices + 1; i < s.length(); ++i)
			{
				if (s[i] == ' ' || s[i] == '[') break;
				name.push_back(s[i]);
			}
		}
	}

	D3D12_COMPARISON_FUNC GetComparison(const std::string& str)
	{
		D3D12_COMPARISON_FUNC ztest = D3D12_COMPARISON_FUNC_ALWAYS;
		auto ite = ShaderUniform::uMap.find(str);
		if (ite != ShaderUniform::uMap.end())
		{
			DefineType dt = (DefineType)ite->second;
			switch (dt)
			{
			case DefineType::Less:
				ztest = D3D12_COMPARISON_FUNC_LESS;
				break;
			case DefineType::LEqual:
				ztest = D3D12_COMPARISON_FUNC_LESS_EQUAL;
				break;
			case DefineType::Greater:
				ztest = D3D12_COMPARISON_FUNC_GREATER;
				break;
			case DefineType::GEqual:
				ztest = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
				break;
			case DefineType::Equal:
				ztest = D3D12_COMPARISON_FUNC_EQUAL;
				break;
			case DefineType::NEqual:
				ztest = D3D12_COMPARISON_FUNC_NOT_EQUAL;
				break;
			case DefineType::Never:
				ztest = D3D12_COMPARISON_FUNC_NEVER;
				break;
			}
		}
		return ztest;
	}

	void ShaderUniform::GetShaderRootSigData(const string& path, vector<ShaderVariable>& vars, vector<PassDescriptor>& passes)
	{
		vars.clear();
		vars.reserve(20);
		passes.clear();
		passes.reserve(20);
		ifstream ifs(path);
		if (!ifs) return;



		std::vector<string> commands;
		std::vector<string> lines;
		ReadLines(ifs, lines);
		for (auto i = lines.begin(); i != lines.end(); ++i)
		{
			string& s = *i;
			ShaderVariable sv;
			sv.tableSize = 1;
			if (GetFirstIndexOf(s, rwtex) == 0)	//RWTexture
			{
				GetVarData(s, true, 'u', sv.tableSize, sv.registerPos, sv.space, sv.name);
				sv.type = ShaderVariableType_UAVDescriptorHeap;
				vars.push_back(sv);
			}
			/*	else if (GetFirstIndexOf(s, rwstr) == 0)//RWStructured
				{
					GetVarData(s, false, 'u', sv.tableSize, sv.registerPos, sv.space, sv.name);
					sv.type = ComputeShaderVariable::RWStructuredBuffer;
				}*/ //VS Shader do not support rwstructured
			else if (GetFirstIndexOf(s, cbuffer) == 0) //cbuffer
			{
				GetVarData(s, false, 'b', sv.tableSize, sv.registerPos, sv.space, sv.name);
				sv.type = ShaderVariableType_ConstantBuffer;
				vars.push_back(sv);
			}
			else if (GetFirstIndexOf(s, tex) == 0) // texture
			{
				GetVarData(s, true, 't', sv.tableSize, sv.registerPos, sv.space, sv.name);
				sv.type = ShaderVariableType_SRVDescriptorHeap;
				vars.push_back(sv);
			}
			else if (GetFirstIndexOf(s, str) == 0)//structured
			{
				GetVarData(s, false, 't', sv.tableSize, sv.registerPos, sv.space, sv.name);
				sv.type = ShaderVariableType_StructuredBuffer;
				vars.push_back(sv);
			}
			else if (GetFirstIndexOf(s, pragma) == 0)
			{
				int start = GetFirstIndexOf(s, ' ');
				if (start >= 0)
				{
					PassDescriptor p;
					p.name.reserve(20);
					for (uint i = start + 1; i < s.length(); ++i)
						p.name += s[i];
					BlendState blendState = BlendState::Off;
					bool zwrite = true;
					auto ztest = D3D12_COMPARISON_FUNC_LESS_EQUAL;
					auto cullmode = D3D12_CULL_MODE_BACK;
					bool conservativeMode = false;
					uint8_t readmask = 255;
					uint8_t writemask = 255;
					D3D12_STENCIL_OP sFail = D3D12_STENCIL_OP_KEEP;
					D3D12_STENCIL_OP zFail = D3D12_STENCIL_OP_KEEP;
					D3D12_STENCIL_OP pass = D3D12_STENCIL_OP_KEEP;
					D3D12_COMPARISON_FUNC sComp = D3D12_COMPARISON_FUNC_ALWAYS;
					for (; i != lines.end(); ++i)
					{
						string& s = *i;
						if (GetFirstIndexOf(s, endPragma) == 0)
						{
							p.blendState = GetBlendState(blendState);
							p.depthStencilState = GetDepthState(zwrite, ztest, readmask, writemask, sFail, zFail, pass, sComp);
							p.rasterizeState = GetCullState(cullmode, conservativeMode);
							passes.push_back(p);
							break;
						}
						Split(s, ' ', commands);
						if (commands.size() >= 2)
						{
							ToLower(commands[0]);
							auto ite = uMap.find(commands[0]);
							if (ite != uMap.end())
							{
								DefineType dt = (DefineType)ite->second;
								switch (dt)
								{
								case DefineType::ZTest:
									ToLower(commands[1]);
									ztest = GetComparison(commands[1]);
									break;
								case DefineType::ZWrite:

									ToLower(commands[1]);
									if (StringEqual(commands[1], "on") || StringEqual(commands[1], "always"))
									{
										zwrite = true;
									}
									else if (StringEqual(commands[1], "off") || StringEqual(commands[1], "never"))
									{
										zwrite = false;
									}
									break;
								case DefineType::Cull:

									ToLower(commands[1]);
									if (StringEqual(commands[1], "back"))
									{
										cullmode = D3D12_CULL_MODE_BACK;
									}
									else if (StringEqual(commands[1], "front"))
									{
										cullmode = D3D12_CULL_MODE_FRONT;
									}
									else if (StringEqual(commands[1], "off") || StringEqual(commands[1], "never"))
									{
										cullmode = D3D12_CULL_MODE_NONE;
									}
									break;
								case DefineType::Vertex:
									p.vertex = commands[1];
									break;
								case DefineType::Fragment:
									p.fragment = commands[1];
									break;
								case DefineType::Hull:
									p.hull = commands[1];
									break;
								case DefineType::Domain:
									p.domain = commands[1];
									break;
								case DefineType::Conservative:
									ToLower(commands[1]);
									if (StringEqual(commands[1], "on") || StringEqual(commands[1], "always"))
										conservativeMode = true;
									else if (StringEqual(commands[1], "off") || StringEqual(commands[1], "never"))
										conservativeMode = false;
									break;
								case DefineType::Blend:
									ToLower(commands[1]);
									if (StringEqual(commands[1], "alpha"))
										blendState = BlendState::Alpha;
									else if (StringEqual(commands[1], "one"))
										blendState = BlendState::One;
									break;
								case DefineType::Stencil_Readmask:
									ToLower(commands[1]);
									readmask = (uint8_t)StringToInt(commands[1]);
									break;
								case DefineType::Stencil_Writemask:
									ToLower(commands[1]);
									writemask = (uint8_t)StringToInt(commands[1]);
									break;
								case DefineType::Stencil_Comp:
									ToLower(commands[1]);
									sComp = GetComparison(commands[1]);
									break;
								case DefineType::Stencil_ZFail:
									ToLower(commands[1]);
									zFail = GetStencilOP(commands[1]);
									break;
								case DefineType::Stencil_Fail:
									ToLower(commands[1]);
									sFail = GetStencilOP(commands[1]);
									break;
								case DefineType::Stencil_Pass:
									ToLower(commands[1]);
									pass = GetStencilOP(commands[1]);
									break;
								case DefineType::Macro:
									if (commands.size() > 1)
									{
										p.macros = std::shared_ptr<std::vector<std::string>>(new std::vector<std::string>());
										p.macros->reserve(commands.size());
										for (uint i = 1; i < commands.size(); ++i)
										{
											p.macros->push_back(commands[i]);
										}
									}
									break;
								}
							}
						}
					}
				}
			}
		}

	}
	void ShaderUniform::GetComputeShaderRootSigData(const string& path, vector<ComputeShaderVariable>& vars, vector<KernelDescriptor>& passes)
	{
		vars.clear();
		vars.reserve(20);
		passes.clear();
		passes.reserve(20);
		ifstream ifs(path);
		if (!ifs) return;


		string kernelName;
		vector<string> lines;
		ReadLines(ifs, lines);
		for (auto i = lines.begin(); i != lines.end(); ++i)
		{
			string& s = *i;
			ComputeShaderVariable sv;
			sv.tableSize = 1;
			if (GetFirstIndexOf(s, rwtex) == 0)	//RWTexture
			{
				GetVarData(s, true, 'u', sv.tableSize, sv.registerPos, sv.space, sv.name);
				sv.type = ComputeShaderVariable::UAVDescriptorHeap;
				vars.push_back(sv);
			}
			else if (GetFirstIndexOf(s, rwstr) == 0)//RWStructured
			{
				GetVarData(s, false, 'u', sv.tableSize, sv.registerPos, sv.space, sv.name);
				sv.type = ComputeShaderVariable::RWStructuredBuffer;
				vars.push_back(sv);
			}
			else if (GetFirstIndexOf(s, cbuffer) == 0) //cbuffer
			{
				GetVarData(s, false, 'b', sv.tableSize, sv.registerPos, sv.space, sv.name);
				sv.type = ComputeShaderVariable::ConstantBuffer;
				vars.push_back(sv);
			}
			else if (GetFirstIndexOf(s, tex) == 0) // texture
			{
				GetVarData(s, true, 't', sv.tableSize, sv.registerPos, sv.space, sv.name);
				sv.type = ComputeShaderVariable::SRVDescriptorHeap;
				vars.push_back(sv);
			}
			else if (GetFirstIndexOf(s, str) == 0)//structured
			{
				GetVarData(s, false, 't', sv.tableSize, sv.registerPos, sv.space, sv.name);
				sv.type = ComputeShaderVariable::StructuredBuffer;
				vars.push_back(sv);
			}
			else if (GetFirstIndexOf(s, pragma) == 0)//Pre
			{
				int start = GetFirstIndexOf(s, ' ');
				if (start >= 0)
				{
					kernelName.clear();
					kernelName.reserve(20);
					for (uint i = start + 1; i < s.length(); ++i)
					{
						kernelName += s[i];
					}
				}
				std::vector<std::string> commands;
				Split(s, ' ', commands);
				KernelDescriptor kd;

				kd.name = kernelName;
				if (commands.size() > 2)
				{
					kd.macros = std::shared_ptr<std::vector<std::string>>(new std::vector<std::string>());
					for (uint i = 2; i < commands.size(); ++i)
					{
						kd.macros->push_back(commands[i]);
					}
				}
				passes.push_back(kd);
			}
		}
	}
}
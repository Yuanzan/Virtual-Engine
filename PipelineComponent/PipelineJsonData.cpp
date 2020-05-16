#include "PipelineJsonData.h"
#include "../CJsonObject/CJsonObject.hpp"
#include "../ResourceManagement/AssetDatabase.h"
using namespace neb;
void PipelineJsonData::GetPipelineIndex(uint* indices, uint offset, const std::type_info& typeData)
{
	std::string na = typeData.name();
	auto ite = typeToIndex.find(na);
	if (ite != typeToIndex.end())
	{
		indices[offset] = ite->second;
	}
	else
	{
		CJsonObject* jsonPtr = jsonPool.New();
		AssetDatabase* instance = AssetDatabase::GetInstance();
		if (instance->TryGetJson(na, jsonPtr))
		{
			jsonObjs.insert_or_assign(
				componentIndex, ObjectPtr<CJsonObject>::MakePtrNoMemoryFree(jsonPtr));
		}
		else
		{
			jsonPool.Delete(jsonPtr);
		}
		typeToIndex.insert_or_assign(na, componentIndex);
		indices[offset] = componentIndex;
		componentIndex++;
	}
}

CJsonObject* PipelineJsonData::GetJsonObject(uint index) const
{
	auto ite = jsonObjs.find(index);
	if (ite == jsonObjs.end()) return nullptr;
	return ite->second;
}

void PipelineJsonData::ForceUpdateAllJson()
{
	AssetDatabase* instance = AssetDatabase::GetInstance();
	for (auto ite = typeToIndex.begin(); ite != typeToIndex.end(); ++ite)
	{
		auto jsonIte = jsonObjs.find(ite->second);
		if (jsonIte == jsonObjs.end())
		{
			CJsonObject* jsonPtr = jsonPool.New();
			
			if (instance->TryGetJson(ite->first, jsonPtr))
			{
				jsonObjs.insert_or_assign(
					componentIndex, ObjectPtr<CJsonObject>::MakePtrNoMemoryFree(jsonPtr));
			}
			else
			{
				jsonPool.Delete(jsonPtr);
			}
		}
		else
		{
			instance->TryGetJson(ite->first, jsonIte->second);
		}
	}
}

PipelineJsonData::PipelineJsonData() :
	jsonPool(64)
{
	typeToIndex.reserve(23);
	jsonObjs.reserve(23);
}

PipelineJsonData::~PipelineJsonData()
{
	jsonObjs.clear();
}

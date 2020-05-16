/*******************************************************************************
 * Project:  neb
 * @file     CJsonObject.hpp
 * @brief    Json
 * @author   bwarliao
 * @date:    2014-7-16
 * @note
 * Modify history:
 ******************************************************************************/

#ifndef CJSONOBJECT_HPP_
#define CJSONOBJECT_HPP_

#include <stdio.h>
#include <stddef.h>
#include <malloc.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <string>
#include <fstream>
#include <unordered_map>
#include <list>
#include "cJSON.h"
#include "../Common/MetaLib.h"
template <typename T, bool b>
class StackObject;
#ifdef _DEBUG
#define DLL
#else
#define DLL __declspec(dllimport)
#endif
namespace neb
{

	class DLL CJsonObject
	{
	public:     // method of ordinary json object or json array
		CJsonObject();
		CJsonObject(const std::string& strJson);
		CJsonObject(const CJsonObject* pJsonObject);
		CJsonObject(const CJsonObject& oJsonObject);
		virtual ~CJsonObject();

		CJsonObject& operator=(const CJsonObject& oJsonObject);
		bool operator==(const CJsonObject& oJsonObject) const;
		bool Parse(const std::string& strJson);
		void Clear();
		bool IsEmpty() const;
		bool IsArray() const;
		std::string ToString() const;
		std::string ToFormattedString() const;
		const std::string& GetErrMsg() const
		{
			return(m_strErrMsg);
		}

	public:     // method of ordinary json object
		bool AddEmptySubObject(const std::string& strKey);
		bool AddEmptySubArray(const std::string& strKey);
		bool GetKey(std::string& strKey);
		void ResetTraversing();
		CJsonObject& operator[](const std::string& strKey);
		std::string operator()(const std::string& strKey) const;
		bool Get(const std::string& strKey, CJsonObject& oJsonObject) const;
		bool Get(const std::string& strKey, std::string& strValue) const;
		bool Get(const std::string& strKey, int32& iValue) const;
		bool Get(const std::string& strKey, uint32& uiValue) const;
		bool Get(const std::string& strKey, int64& llValue) const;
		bool Get(const std::string& strKey, uint64& ullValue) const;
		bool Get(const std::string& strKey, bool& bValue) const;
		bool Get(const std::string& strKey, float& fValue) const;
		bool Get(const std::string& strKey, double& dValue) const;
		bool IsNull(const std::string& strKey) const;
		bool Add(const std::string& strKey, const CJsonObject& oJsonObject);
		bool Add(const std::string& strKey, const std::string& strValue);
		bool Add(const std::string& strKey, int32 iValue);
		bool Add(const std::string& strKey, uint32 uiValue);
		bool Add(const std::string& strKey, int64 llValue);
		bool Add(const std::string& strKey, uint64 ullValue);
		bool Add(const std::string& strKey, bool bValue, bool bValueAgain);
		bool Add(const std::string& strKey, float fValue);
		bool Add(const std::string& strKey, double dValue);
		bool AddNull(const std::string& strKey);    // add null like this:   "key":null
		bool Delete(const std::string& strKey);
		bool Replace(const std::string& strKey, const CJsonObject& oJsonObject);
		bool Replace(const std::string& strKey, const std::string& strValue);
		bool Replace(const std::string& strKey, int32 iValue);
		bool Replace(const std::string& strKey, uint32 uiValue);
		bool Replace(const std::string& strKey, int64 llValue);
		bool Replace(const std::string& strKey, uint64 ullValue);
		bool Replace(const std::string& strKey, bool bValue, bool bValueAgain);
		bool Replace(const std::string& strKey, float fValue);
		bool Replace(const std::string& strKey, double dValue);
		bool ReplaceWithNull(const std::string& strKey);    // replace value with null

	public:     // method of json array
		int GetArraySize();
		CJsonObject& operator[](unsigned int uiWhich);
		std::string operator()(unsigned int uiWhich) const;
		bool Get(int iWhich, CJsonObject& oJsonObject) const;
		bool Get(int iWhich, std::string& strValue) const;
		bool Get(int iWhich, int32& iValue) const;
		bool Get(int iWhich, uint32& uiValue) const;
		bool Get(int iWhich, int64& llValue) const;
		bool Get(int iWhich, uint64& ullValue) const;
		bool Get(int iWhich, bool& bValue) const;
		bool Get(int iWhich, float& fValue) const;
		bool Get(int iWhich, double& dValue) const;
		bool IsNull(int iWhich) const;
		bool Add(const CJsonObject& oJsonObject);
		bool Add(const std::string& strValue);
		bool Add(int32 iValue);
		bool Add(uint32 uiValue);
		bool Add(int64 llValue);
		bool Add(uint64 ullValue);
		bool Add(int iAnywhere, bool bValue);
		bool Add(float fValue);
		bool Add(double dValue);
		bool AddNull();   // add a null value
		bool AddAsFirst(const CJsonObject& oJsonObject);
		bool AddAsFirst(const std::string& strValue);
		bool AddAsFirst(int32 iValue);
		bool AddAsFirst(uint32 uiValue);
		bool AddAsFirst(int64 llValue);
		bool AddAsFirst(uint64 ullValue);
		bool AddAsFirst(int iAnywhere, bool bValue);
		bool AddAsFirst(float fValue);
		bool AddAsFirst(double dValue);
		bool AddNullAsFirst();     // add a null value
		bool Delete(int iWhich);
		bool Replace(int iWhich, const CJsonObject& oJsonObject);
		bool Replace(int iWhich, const std::string& strValue);
		bool Replace(int iWhich, int32 iValue);
		bool Replace(int iWhich, uint32 uiValue);
		bool Replace(int iWhich, int64 llValue);
		bool Replace(int iWhich, uint64 ullValue);
		bool Replace(int iWhich, bool bValue, bool bValueAgain);
		bool Replace(int iWhich, float fValue);
		bool Replace(int iWhich, double dValue);
		bool ReplaceWithNull(int iWhich);      // replace with a null value

	private:
		CJsonObject(cJSON* pJsonData);

	private:
		cJSON* m_pJsonData;
		cJSON* m_pExternJsonDataRef;
		cJSON* m_pKeyTravers;
		std::string m_strErrMsg;
		std::unordered_map<unsigned int, CJsonObject*> m_mapJsonArrayRef;
		std::unordered_map<std::string, CJsonObject*> m_mapJsonObjectRef;
	};

}

template <typename T>
struct JsonKeyValuePair
{
	const std::string& key;
	PureType_t<T>* value;
	JsonKeyValuePair(const std::string& str, PureType_t<T>* value) : value(value), key(str) {}
	JsonKeyValuePair(const std::string& str, PureType_t<T>& value) : value(&value), key(str) {}
};

template <typename ... Args>
constexpr void GetValuesFromJson(neb::CJsonObject* cjson, Args&& ... args)
{
	char c[] = { (cjson->Get(args.key, *args.value), 0)... };
}

bool DLL ReadJson(const std::string& filePath, neb::CJsonObject* placementMemory);
double GetDoubleFromChar(char* c, size_t t);
float DLL GetFloatFromChar(char* c, size_t t);
int DLL GetIntFromChar(char* c, size_t t);
void ReadJson(const std::string& filePath, StackObject<neb::CJsonObject, true>& placementPtr);
template <typename T>
void ReadStringToVector(char* cPtr, size_t t, T* vec)
{
	const size_t floatNum = sizeof(T) / sizeof(float);
	float* vecPointer = (float*)vec;
	size_t count = 0;
	size_t floatOffset = 0;
	char* start = cPtr;
	for (size_t i = 0; i < t; ++i)
	{
		char c = cPtr[i];
		if (c == ',')
		{
			if (floatOffset >= floatNum) return;
			vecPointer[floatOffset] = GetFloatFromChar(start, count);
			start = cPtr + i + 1;
			count = 0;
			floatOffset++;
		}
		else
		{
			count++;
		}
	}
	if (floatOffset >= floatNum) return;
	vecPointer[floatOffset] = GetFloatFromChar(start, count);
}

template <typename T>
void ReadStringToDoubleVector(char* cPtr, size_t t, T* vec)
{
	const size_t floatNum = sizeof(T) / sizeof(double);
	double* vecPointer = (double*)vec;
	size_t count = 0;
	size_t floatOffset = 0;
	char* start = cPtr;
	for (size_t i = 0; i < t; ++i)
	{
		char c = cPtr[i];
		if (c == ',')
		{
			if (floatOffset >= floatNum) return;
			vecPointer[floatOffset] = GetDoubleFromChar(start, count);
			start = cPtr + i + 1;
			count = 0;
			floatOffset++;
		}
		else
		{
			count++;
		}
	}
	if (floatOffset >= floatNum) return;
	vecPointer[floatOffset] = GetDoubleFromChar(start, count);
}

template <typename T>
void ReadStringToIntVector(char* cPtr, size_t t, T* vec)
{
	const size_t intNum = sizeof(T) / sizeof(int);
	int* vecPointer = (int*)vec;
	size_t count = 0;
	size_t floatOffset = 0;
	char* start = cPtr;
	for (size_t i = 0; i < t; ++i)
	{
		char c = cPtr[i];
		if (c == ',')
		{
			if (floatOffset >= intNum) return;
			vecPointer[floatOffset] = GetIntFromChar(start, count);
			start = cPtr + i + 1;
			count = 0;
			floatOffset++;
		}
		else
		{
			count++;
		}
	}
	if (floatOffset >= intNum) return;
	vecPointer[floatOffset] = GetIntFromChar(start, count);
}
#undef DLL
#endif

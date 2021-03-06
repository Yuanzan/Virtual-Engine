#include "AssetDatabase.h"
#include "AssetReference.h"
#include "../Common/d3dApp.h"
AssetDatabase* AssetDatabase::current(nullptr);
using namespace neb;
ID3D12Device* AssetDatabase::device = nullptr;
namespace LoadingThreadFlags
{
	bool threadEnabled = true;
}
using namespace LoadingThreadFlags;

void AssetDatabase::LoadingThreadMainLogic()
{
	std::vector<AssetReference> loadListCache;
	loadListCache.reserve(100);
	while (threadEnabled)
	{
		{
			uint v = (uint)&current->loadingThreadMtx;
			std::unique_lock<std::mutex> lck(current->loadingThreadMtx);
			while (current->shouldWaiting)
				current->cv.wait(lck);
			current->shouldWaiting = true;
		}
		{
			std::lock_guard<std::mutex> lck(current->loadingThreadMtx);
			for (uint i = 0, size = current->loadList.size(); i < size; ++i)
			{
				loadListCache.push_back(current->loadList[i]);
			}
			current->loadList.clear();
		}
		for (uint i = 0; i < loadListCache.size(); ++i)
		{
			AssetReference& ref = loadListCache[i];
			if (ref.IsLoad())
			{
				ref.Load(
					device,
					current->mainThreadRunnable);
			}
			else
			{
				ref.Unload();
			}
		}
		loadListCache.clear();
		current->shouldWaiting = current->shouldWaiting && current->loadList.empty();
	}
}

void AssetDatabase::AsyncLoads(AssetReference* refs, uint refCount)
{
	for (uint i = 0; i < refCount; ++i)
	{
		loadList.push_back(refs[i]);
	}
	shouldWaiting = false;
	{
		std::lock_guard<std::mutex> lck(current->loadingThreadMtx);
		current->cv.notify_all();
	}
}

void AssetDatabase::AsyncLoad(const AssetReference& ref)
{
	loadList.push_back(ref);
	shouldWaiting = false;
	{
		std::lock_guard<std::mutex> lck(current->loadingThreadMtx);
		current->cv.notify_all();
	}
}

void AssetDatabase::MainThreadUpdate()
{
	for (auto ite = mainThreadRunnable.begin(); ite != mainThreadRunnable.end(); ++ite)
	{
		ite->first(ite->second);
	}
	mainThreadRunnable.clear();
}

AssetDatabase::AssetDatabase(ID3D12Device* device)
{
	splitCache.reserve(5);
	this->device = device;
	current = this;
	mainThreadRunnable.reserve(20);
	loadList.reserve(20);
	loadingThread = (std::thread*) & threadStorage;
	new (loadingThread) std::thread(LoadingThreadMainLogic);
	loadedObjects.reserve(500);
	resourceReferenceCount.reserve(500);
	rootJsonObj = (CJsonObject*)&rootJsonObjStorage;
	if (!ReadJson("Resource/AssetDatabase.json", rootJsonObj))
	{
		
		rootJsonObj = nullptr;
	}
	std::string key, value;
	while (rootJsonObj->GetKey(key))
	{
		rootJsonObj->Get(key, value);
		std::string& v = value;
	}
	ReadJson("Data/JsonDirectory.json", jsonDirectory);
	if (!jsonDirectory)
	{
		throw "Json Not Found Exception!";
	}
}

ObjectPtr<MObject> AssetDatabase::GetLoadedObject(const std::string& str)
{
	std::lock_guard<std::mutex> lck(mtx);
	auto ite = loadedObjects.find(str);
	if (ite == loadedObjects.end())
	{
		return nullptr;
	}
	if (!ite->second)
	{
		loadedObjects.erase(ite);
		return nullptr;
	}
	return ite->second;
}

AssetDatabase::~AssetDatabase()
{

	threadEnabled = false;
	shouldWaiting = false;
	{
		std::lock_guard<std::mutex> lck(current->loadingThreadMtx);
		current->cv.notify_all();
	}
	loadingThread->join();
	loadingThread->~thread();
	if (rootJsonObj)
		rootJsonObj->~CJsonObject();
}
#include "../Common/StringUtility.h"
bool AssetDatabase::TryGetJson(const std::string& name, neb::CJsonObject* targetJsonObj)
{
	std::string path;
	StringUtil::Split(name, ' ', splitCache);
	if (!jsonDirectory->Get(*(splitCache.end() - 1), path)) return false;
	path = "Data/" + path;
	std::ifstream ifs(path);
	if (!ifs) return false;
	ifs.seekg(0, std::ios::end);
	size_t len = ifs.tellg();
	ifs.seekg(0, 0);
	std::string s;
	s.resize(len);
	ifs.read((char*)s.data(), len);
	return targetJsonObj->Parse(s);
}
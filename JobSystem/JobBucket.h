#pragma once
#include <vector>
#include "JobHandle.h"
#include "../Common/Pool.h"
#include "../Common/TypeWiper.h"
class JobSystem;
class JobThreadRunnable;
class JobNode;
class DLL_CLASS JobBucket
{
	friend class JobSystem;
	friend class JobNode;
	friend class JobHandle;
	friend class JobThreadRunnable;
private:
	std::vector<JobNode*> jobNodesVec;
	unsigned int allJobNodeCount = 0;
	JobSystem* sys = nullptr;
	JobBucket(JobSystem* sys) noexcept;
	~JobBucket() noexcept{}
	JobHandle GetTask(JobHandle* dependedJobs, unsigned int dependCount, const FunctorData& funcData, void* funcPtr);
public:
	template <typename T>
	constexpr JobHandle GetTask(JobHandle* dependedJobs, unsigned int dependCount, const T& func)
	{
		using Func = typename std::remove_cvref_t<T>;
		static_assert(sizeof(Func) <= JobNode::STORAGE_SIZE);
		static_assert(alignof(Func) <= alignof(__m128));
		FunctorData funcData = GetFunctor<Func>();
		return GetTask(dependedJobs, dependCount, funcData, (Func*)(&func));
	}
};
#ifdef DLL_EXPORT
#include "JobBucket.h"
#include "JobNode.h"
#include "JobSystem.h"
JobBucket::JobBucket(JobSystem* sys) noexcept : 
	sys(sys)
{
	jobNodesVec.reserve(20);
}

JobHandle JobBucket::GetTask(JobHandle* dependedJobs, unsigned int dependCount, const FunctorData& funcData, void* funcPtr)
{
	JobNode* node = sys->jobNodePool.New();
	allJobNodeCount++;
	node->Create(funcData, funcPtr, &sys->threadMtx, dependedJobs, dependCount);
	if (dependCount == 0) jobNodesVec.push_back(node);
	return JobHandle(node);
}
#endif
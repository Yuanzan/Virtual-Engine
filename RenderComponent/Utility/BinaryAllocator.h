#pragma once
#include <stdint.h>
#include <vector>
#include <unordered_map>
typedef uint32_t uint;
typedef uint64_t uint64;


class BinaryAllocator
{
public:
	struct BinaryTreeNode
	{
		int64_t vectorIndex;
		uint layer;
		bool isLeft;
	};
	struct AllocatedChunkHandle
	{
		friend class BinaryAllocator;
	private:
		BinaryTreeNode* node;
	public:
	};
private:
	std::vector<std::vector<BinaryTreeNode*>> usefulNodes;
	void ReturnChunk(BinaryTreeNode* node);
public:
	size_t GetLayerCount() const { return usefulNodes.size(); }
	bool TryAllocate(uint targetLevel, AllocatedChunkHandle& result);
	void Return(AllocatedChunkHandle target);
	BinaryAllocator(
		uint32_t maximumLayer);
	~BinaryAllocator();
};
using BuddyAllocatorHandle = BinaryAllocator::AllocatedChunkHandle;
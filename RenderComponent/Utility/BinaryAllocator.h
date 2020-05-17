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
		uint64_t size;
		uint64_t offset;
		BinaryTreeNode* parentNode;
		BinaryTreeNode* leftNode;
		BinaryTreeNode* rightNode;
		uint layer;
	};
	struct AllocatedChunkHandle
	{
		friend class BinaryAllocator;
	private:
		BinaryTreeNode* node;
	public:
		uint64_t GetSize() const
		{
			return node->size;
		}
		uint64_t GetOffset() const
		{
			return node->offset;
		}
	};
private:
	BinaryTreeNode* nodes;
	uint allNodesCount;
	std::vector<std::vector<BinaryTreeNode*>> usefulNodes;
	BinaryTreeNode* SetTreeNode(
		BinaryTreeNode* parentNode,
		uint64_t size,
		uint64_t offset,
		uint layer,
		uint layerCount,
		uint& indexOffset);
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
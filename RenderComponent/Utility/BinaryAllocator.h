#pragma once
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <iostream>
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
	///////////////////// Debug
	void Test()
	{
		using namespace std;
		cout << "All Node Count: " << usefulNodes.size() << endl;
		for (uint i = 0; i < usefulNodes.size(); ++i)
		{
			cout << "Layer " << i << " count: " << usefulNodes[i].size() << endl;
		}
	}
	///////////////////
	bool TryAllocate(uint targetLevel, AllocatedChunkHandle& result);
	void Return(AllocatedChunkHandle target);
	BinaryAllocator(
		uint32_t maximumLayer);
	~BinaryAllocator();
};
using BuddyAllocHandle = BinaryAllocator::AllocatedChunkHandle;
#pragma once
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <iostream>
typedef uint32_t uint;
typedef uint64_t uint64;


class BuddyAllocator
{
public:
	struct BinaryTreeNode
	{
		int64_t vectorIndex;
		BinaryTreeNode* parentNode;
		BinaryTreeNode* leftNode;
		BinaryTreeNode* rightNode;
		uint layer;
	};
	struct AllocatedChunkHandle
	{
		friend class BuddyAllocator;
	private:
		BinaryTreeNode* node;
	};
private:
	BinaryTreeNode* nodes;
	uint allNodesCount;
	std::vector<std::vector<BinaryTreeNode*>> usefulNodes;
	BinaryTreeNode* SetTreeNode(
		BinaryTreeNode* parentNode,
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
	BuddyAllocator(
		uint32_t maximumLayer);
	~BuddyAllocator();
};
using BuddyAllocHandle = BuddyAllocator::AllocatedChunkHandle;
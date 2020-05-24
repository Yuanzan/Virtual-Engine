#include "BinaryAllocator.h"
#include <algorithm>
#include <stdint.h>
BinaryAllocator::BinaryAllocator(
	uint32_t maximumLayer) :
	usefulNodes(maximumLayer)
{
	maximumLayer = std::max<uint32_t>({ maximumLayer, 1 });
	allNodesCount = (1 << maximumLayer) - 1;
	nodes = new BinaryTreeNode[allNodesCount];
	uint indexOffset = 0;
	uint64_t fullSize = 1 << (maximumLayer - 1);
	SetTreeNode(nullptr, fullSize, 0, 0, maximumLayer, indexOffset);
	for (size_t i = 1; i < usefulNodes.size(); ++i)
	{
		usefulNodes[i].reserve(std::max({ (1 << i) / 2 , 1 }));
	}
	usefulNodes[0].push_back(nodes);
}

bool BinaryAllocator::TryAllocate(uint targetLevel, AllocatedChunkHandle& result)
{

}

BinaryAllocator::~BinaryAllocator()
{
	
}

void BinaryAllocator::ReturnChunk(BinaryTreeNode* node)
{
	
}

void BinaryAllocator::Return(AllocatedChunkHandle target)
{
	ReturnChunk(target.node);
}
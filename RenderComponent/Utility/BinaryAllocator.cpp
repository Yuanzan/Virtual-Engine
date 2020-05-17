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
		usefulNodes[i].reserve(std::max({ (1 << i) / 2 , 1}));
	}
	usefulNodes[0].push_back(nodes);
}

BinaryAllocator::BinaryTreeNode* BinaryAllocator::SetTreeNode(
	BinaryTreeNode* parentNode,
	uint64_t size,
	uint64_t offset,
	uint layer,
	uint layerCount,
	uint& indexOffset)
{
	BinaryTreeNode* currentNode = nodes + indexOffset;
	indexOffset++;
	currentNode->vectorIndex = -1;
	currentNode->parentNode = parentNode;
	currentNode->layer = layer;
	currentNode->size = size;
	currentNode->offset = offset;
	uint64_t subSize = size / 2;
	//Is last level
	if (layer == layerCount - 1)
	{
		currentNode->leftNode = nullptr;
		currentNode->rightNode = nullptr;
	}
	else
	{
		layer++;
		currentNode->leftNode = SetTreeNode(
			currentNode,
			subSize,
			offset,
			layer,
			layerCount,
			indexOffset);
		currentNode->rightNode = SetTreeNode(
			currentNode,
			subSize,
			offset + subSize,
			layer,
			layerCount,
			indexOffset);
	}
	return currentNode;
}

bool BinaryAllocator::TryAllocate(uint targetLevel, AllocatedChunkHandle& result)
{
	int startLevel = targetLevel;
	for (; startLevel >= 0; --startLevel)
	{
		if (!usefulNodes[startLevel].empty()) break;
	}
	if (usefulNodes[startLevel].empty()) return false;
	for (int i = startLevel; i < targetLevel; ++i)
	{
		auto lastIte = usefulNodes[i].end() - 1;
		(*lastIte)->vectorIndex = -1;
		auto& a = usefulNodes[i + 1];
		(*lastIte)->leftNode->vectorIndex = a.size();
		a.push_back((*lastIte)->leftNode);
		(*lastIte)->rightNode->vectorIndex = a.size();
		a.push_back((*lastIte)->rightNode);
		usefulNodes[i].erase(lastIte);
	}
	auto ite = usefulNodes[targetLevel].end() - 1;
	result.node = *ite;
	(*ite)->vectorIndex = -1;
	usefulNodes[targetLevel].erase(ite);
}

BinaryAllocator::~BinaryAllocator()
{
	delete[] nodes;
}

void BinaryAllocator::ReturnChunk(BinaryTreeNode* node)
{
	auto& vec = usefulNodes[node->layer];
	if (node->parentNode)
	{
		BinaryTreeNode* buddyNode =
			node->parentNode->leftNode == node ?
			node->parentNode->rightNode : 
			node->parentNode->leftNode;
		
		if (buddyNode->vectorIndex >= 0)
		{
			auto lastIte = vec.end() - 1;
			vec[buddyNode->vectorIndex] = *lastIte;
			(*lastIte)->vectorIndex = buddyNode->vectorIndex;
			buddyNode->vectorIndex = -1;
			vec.erase(lastIte);
			ReturnChunk(node->parentNode);
		}
		else
		{
			node->vectorIndex = vec.size();
			vec.push_back(node);
		}
	}
	else
	{
		node->vectorIndex = vec.size();
		vec.push_back(node);
	}
}

void BinaryAllocator::Return(AllocatedChunkHandle target)
{
	ReturnChunk(target.node);
}
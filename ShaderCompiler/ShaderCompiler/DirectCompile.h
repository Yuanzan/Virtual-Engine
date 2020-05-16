#pragma once
#include "ICompileIterator.h"

class DirectCompile : public ICompileIterator
{
	bool isDebug;
	std::vector<Command> commands;
public:
	DirectCompile();
	virtual void UpdateCommand();
	virtual std::vector<Command>& GetCommand();
};
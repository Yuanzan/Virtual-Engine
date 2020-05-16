#pragma once
#include "ICompileIterator.h"
class BatchCompile : public ICompileIterator
{
private:
	std::vector<Command> commands;
	bool isDebug;
	std::string lookUpFilePath;
public:
	BatchCompile();
	virtual void UpdateCommand();
	virtual std::vector<Command>& GetCommand();
};
#include "BatchCompile.h"
#include <iostream>
#include <fstream>
#include "StringUtility.h"
using namespace std;
BatchCompile::BatchCompile()
{
RE_CHOOSE:
	cout << "DXC Or FXC? Y for DXC & N for FXC" << endl;
	string s;
	cin >> s;
	if (s.size() != 1 || ((s[0] != 'Y') && s[0] != 'N' && s[0] != 'y' && s[0] != 'n'))
	{
		goto RE_CHOOSE;
	}
	if (s[0] == 'Y' || s[0] == 'y')
		isDebug = false;
	else isDebug = true;
}
void BatchCompile::UpdateCommand()
{
	cout << "Please input batch look-up file: " << endl;
	cin >> lookUpFilePath;

}
std::vector<Command>& BatchCompile::GetCommand()
{
	Command c;
	c.isDebug = isDebug;
	commands.clear();

	ifstream ifs(lookUpFilePath);
	if (!ifs)
	{
		cout << "Look-up File Not Exists!" << endl;
		return commands;
	}
	vector<string> splitedCommands;
	vector<string> inputLines;
	ReadLines(ifs, inputLines);
	for (auto i = inputLines.begin(); i != inputLines.end(); ++i)
	{
		string& s = *i;
		std::ifstream vsIfs(s + ".hlsl");
		std::ifstream csIfs(s + ".compute");
		if (vsIfs)
		{
			c.isCompute = false;
			c.fileName = s + ".hlsl";
		}
		else if (csIfs)
		{
			c.isCompute = true;
			c.fileName = s + ".compute";
		}
		else continue;
		c.propertyFileName = s + ".prop";
		commands.push_back(c);
	}
	return commands;
}
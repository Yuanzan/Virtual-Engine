#include "DirectCompile.h";
#include <fstream>
using namespace std;
DirectCompile::DirectCompile()
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

std::vector<Command>& DirectCompile::GetCommand()
{
	return commands;
}

void DirectCompile::UpdateCommand()
{
	commands.clear();
	string fileName;
	cout << "Please Input the source shader file name: " << endl;
	cin >> fileName;
	Command c;
	{
		ifstream sourceIfs(fileName + ".hlsl");
		ifstream sourceCps(fileName + ".compute");
		if (sourceIfs)
		{
			c.fileName = fileName + ".hlsl";
			c.isCompute = false;
		}
		else if(sourceCps)
		{
			c.fileName = fileName + ".compute";
			c.isCompute = true;
		}
		else
		{
			return;
		}
	}
	
	c.propertyFileName = fileName + ".prop";
	c.isDebug = isDebug;
	commands.push_back(c);
}
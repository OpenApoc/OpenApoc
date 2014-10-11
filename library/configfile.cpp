#include "configfile.h"

#include <fstream>
#include <sstream>

namespace OpenApoc {

ConfigFile::ConfigFile(const std::string fileName, std::map<std::string, std::string> defaults)
	: values(defaults), defaults(defaults)
{
	std::ifstream inFile{fileName, std::ios::in};
	int line = 0;
	while (inFile)
	{
		line++;
		std::string line;
		std::getline(inFile, line);
		if (!inFile)
			break;
		if (line[0] == '#')
			continue;
		auto splitPos = line.find_first_of('=');
		if (splitPos == line.npos)
		{
			std::cerr << "Error reading config \"" << fileName << "\" line " << line << " - no '='\n";
			continue;
		}
		std::string key = line.substr(0, splitPos);
		std::string value = line.substr(splitPos+1);
		values[key] = value;
	}
}

void
ConfigFile::save(const std::string fileName)
{
	std::ofstream outFile{fileName, std::ios::out};
	if (!outFile)
	{
		std::cerr << "Error opening config file \"" << fileName << "\n";
		return;
	}

	for (auto &pair : this->values)
	{
		//If the value is the default, print it commented out
		if (pair.second == defaults[pair.first])
			outFile << "#";
		outFile << pair.first << "=" << pair.second << "\n";
	}
}

std::string
ConfigFile::getString(const std::string key)
{
	auto it = this->values.find(key);
	if (it != this->values.end())
		return it->second;

	it = this->defaults.find(key);
	if (it != this->defaults.end())
		return it->second;

	std::cerr << "Error reading config key \"" << key << "\" - not found\n";
	return "";
}

int
ConfigFile::getInt(const std::string key)
{
	auto string = this->getString(key);
	if (string == "")
	{
		return 0;
	}
	int value = std::atoi(string.c_str());
	return value;
}

static const std::vector<std::string> falseValues =
{
	"0",
	"n",
	"false",
};

static const std::vector<std::string> trueValues =
{
	"1",
	"y",
	"true",
};

bool
ConfigFile::getBool(const std::string key)
{
	auto value = this->getString(key);
	for (auto &v : trueValues)
	{
		if (v == value)
			return true;
	}
	for (auto &v : falseValues)
	{
		if (v == value)
			return false;
	}
	std::cerr << "Error reading boolean key \"" << key << "\" - invalid value \"" << value << "\"\n";
	return false;
}

void
ConfigFile::set(const std::string key, const std::string value)
{
	this->values[key] = value;
}

void
ConfigFile::set(const std::string key, bool value)
{
	if (value)
		this->set(key, trueValues[0]);
	else
		this->set(key, falseValues[0]);
}

void
ConfigFile::set(const std::string key, int value)
{
	std::stringstream ss;
	ss << std::dec << value;
	this->set(key, ss.str());
}

}; //namespace OpenApoc

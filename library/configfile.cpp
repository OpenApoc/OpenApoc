#define _CRT_SECURE_NO_WARNINGS

#include "framework/logger.h"
#include "library/configfile.h"

#include <fstream>
#include <sstream>

namespace OpenApoc
{

ConfigFile::ConfigFile(const UString fileName, std::map<UString, UString> defaults)
    : values(defaults), defaults(defaults)
{
	std::ifstream inFile{fileName.str().c_str(), std::ios::in};
	int lineNo = 0;
	while (inFile)
	{
		lineNo++;
		std::string line;
		std::getline(inFile, line);
		if (!inFile)
			break;
		if (line[0] == '#')
			continue;
		auto splitPos = line.find_first_of('=');
		if (splitPos == line.npos)
		{
			LogError("Error reading config \"%s\" line %d", fileName.str().c_str(), lineNo);
			continue;
		}
		UString key = line.substr(0, splitPos);
		UString value = line.substr(splitPos + 1);
		values[key] = value;
	}
}

void ConfigFile::save(const UString fileName)
{
	std::ofstream outFile{fileName.str().c_str(), std::ios::out};
	if (!outFile)
	{
		LogError("Failed to open config file \"%s\"", fileName.str().c_str());
		return;
	}

	outFile << "# Lines starting with a '#' are ignored\n";

	for (auto &pair : this->values)
	{
		// If the value is the default, print it commented out
		if (pair.second == defaults[pair.first])
			outFile << "#";
		outFile << pair.first.str() << "=" << pair.second.str() << "\n";
	}
}

UString ConfigFile::getString(UString key)
{
	auto it = this->values.find(key);
	if (it != this->values.end())
		return it->second;

	it = this->defaults.find(key);
	if (it != this->defaults.end())
		return it->second;

	LogError("Config key \"%s\" not found", key.str().c_str());
	return "";
}

int ConfigFile::getInt(UString key)
{
	auto string = this->getString(key);
	if (string == "")
	{
		return 0;
	}
	int value = std::atoi(string.str().c_str());
	return value;
}

static const std::vector<UString> falseValues = {
    "0", "n", "no", "false",
};

static const std::vector<UString> trueValues = {
    "1", "y", "yes", "true",
};

bool ConfigFile::getBool(UString key)
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
	LogError("Invalid boolean value of \"%s\" in key \"%s\"", value.str().c_str(),
	         key.str().c_str());
	return false;
}

void ConfigFile::set(const UString key, const UString value) { this->values[key] = value; }

void ConfigFile::set(const UString key, bool value)
{
	if (value)
		this->set(key, trueValues[0]);
	else
		this->set(key, falseValues[0]);
}

void ConfigFile::set(const UString key, int value)
{
	std::stringstream ss;
	ss << std::dec << value;
	this->set(key, ss.str());
}

}; // namespace OpenApoc

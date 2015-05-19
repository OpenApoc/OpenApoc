#include "framework/logger.h"
#include "library/configfile.h"

#include <fstream>
#include <sstream>

namespace OpenApoc {

ConfigFile::ConfigFile(const UString fileName, std::map<UString, UString> defaults)
	: values(defaults), defaults(defaults)
{
	std::string platformString;
	fileName.toUTF8String(platformString);
	std::ifstream inFile{platformString, std::ios::in};
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
			LogError("Error reading config \"%s\" line %d", platformString.c_str(), lineNo);
			continue;
		}
		UString key = U8Str(line.substr(0, splitPos).c_str());
		UString value = U8Str(line.substr(splitPos+1).c_str());
		values[key] = value;
	}
}

void
ConfigFile::save(const UString fileName)
{
	std::string platformString;
	fileName.toUTF8String(platformString);
	std::ofstream outFile{platformString.c_str(), std::ios::out};
	if (!outFile)
	{
		LogError("Failed to open config file \"%s\"", platformString.c_str());
		return;
	}

	outFile << "# Lines starting with a '#' are ignored\n";

	for (auto &pair : this->values)
	{
		std::string u8Key, u8Value;
		pair.first.toUTF8String(u8Key);
		pair.second.toUTF8String(u8Value);
		//If the value is the default, print it commented out
		if (pair.second == defaults[pair.first])
			outFile << "#";
		outFile << u8Key << "=" << u8Value << "\n";
	}
}

UString
ConfigFile::getString(UString key)
{
	auto it = this->values.find(key);
	if (it != this->values.end())
		return it->second;

	it = this->defaults.find(key);
	if (it != this->defaults.end())
		return it->second;

	LogError("Config key \"%S\" not found", key.getTerminatedBuffer());
	return "";
}

int
ConfigFile::getInt(UString key)
{
	auto string = this->getString(key);
	if (string == "")
	{
		return 0;
	}
	std::string u8Str;
	string.toUTF8String(u8Str);
	int value = std::atoi(u8Str.c_str());
	return value;
}

static const std::vector<UString> falseValues =
{
	"0",
	"n",
	"no",
	"false",
};

static const std::vector<UString> trueValues =
{
	"1",
	"y",
	"yes",
	"true",
};

bool
ConfigFile::getBool(UString key)
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
	LogError("Invalid boolean value of \"%S\" in key \"%S\"", value.getTerminatedBuffer(), key.getTerminatedBuffer());
	return false;
}

void
ConfigFile::set(const UString key, const UString value)
{
	this->values[key] = value;
}

void
ConfigFile::set(const UString key, bool value)
{
	if (value)
		this->set(key, trueValues[0]);
	else
		this->set(key, falseValues[0]);
}

void
ConfigFile::set(const UString key, int value)
{
	std::stringstream ss;
	ss << std::dec << value;
	this->set(key, U8Str(ss.str().c_str()));
}

}; //namespace OpenApoc


#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "framework/includes.h"

namespace OpenApoc
{

class ConfigFile
{
  private:
	std::map<UString, UString> values;
	std::map<UString, UString> defaults;

  public:
	ConfigFile(const UString fileName, std::map<UString, UString> defaults);
	void save(const UString fileName);

	int getInt(UString key);
	bool getBool(UString key);
	UString getString(UString key);
	void set(const UString key, bool value);
	void set(const UString key, int value);
	void set(const UString key, const UString value);
};

}; // namespace OpenApoc

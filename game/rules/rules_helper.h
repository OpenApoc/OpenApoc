#pragma once
#include <tinyxml2.h>
#include <map>
#include "library/strings.h"
#include "library/colour.h"
#include "framework/logger.h"

namespace OpenApoc
{
bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, bool &output);
bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, float &output);
bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, Colour &output);
bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, int &output);
bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, UString &output);

bool ReadText(tinyxml2::XMLElement *element, bool &output);
bool ReadText(tinyxml2::XMLElement *element, float &output);
bool ReadText(tinyxml2::XMLElement *element, Colour &output);
bool ReadText(tinyxml2::XMLElement *element, int &output);
bool ReadText(tinyxml2::XMLElement *element, UString &output);

template <typename T>
bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName,
                   const std::map<UString, T> &valueMap, T &output)
{
	UString str;
	if (!ReadAttribute(element, attributeName, str))
		return false;
	auto it = valueMap.find(str);
	if (it == valueMap.end())
	{
		LogWarning("No matching value for \"%s\"", str.c_str());
		return false;
	}
	output = it->second;
	return true;
}

template <typename T>
bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, T &output,
                   const T &defaultValue)
{
	if (ReadAttribute(element, attributeName, output))
		return true;
	output = defaultValue;
	return false;
}

template <typename T>
bool ReadText(tinyxml2::XMLElement *element, const std::map<UString, T> &valueMap, T &output)
{
	UString str = element->GetText();
	auto it = valueMap.find(str);
	if (it == valueMap.end())
	{
		LogWarning("No matching value for \"%s\"", str.c_str());
		return false;
	}
	output = it->second;
	return true;
}

template <typename T> bool ReadText(tinyxml2::XMLElement *element, T &output, const T &defaultValue)
{
	if (ReadText(element, output))
		return true;
	output = defaultValue;
	return false;
}
}; // namespace OpenApoc

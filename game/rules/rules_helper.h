#pragma once
#include <tinyxml2.h>
#include <map>
#include "library/strings.h"
#include "library/colour.h"
#include "library/vec.h"
#include "framework/logger.h"

namespace OpenApoc
{

bool FromString(const UString &str, UString &output);
bool FromString(const UString &str, int &output);
bool FromString(const UString &str, float &output);
bool FromString(const UString &str, bool &output);
bool FromString(const UString &str, Colour &output);

template <typename T>
bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, T &output)
{
	if (!element)
	{
		LogError("Invalid element pointer");
		return false;
	}
	if (!element->Attribute(attributeName.c_str()))
	{
		return false;
	}

	UString str = element->Attribute(attributeName.c_str());

	return FromString(str, output);
}

template <typename T> bool ReadElement(tinyxml2::XMLElement *element, T &output);

template <> bool ReadElement(tinyxml2::XMLElement *element, Vec2<int> &output);
template <> bool ReadElement(tinyxml2::XMLElement *element, Vec3<int> &output);
template <> bool ReadElement(tinyxml2::XMLElement *element, Vec2<float> &output);
template <> bool ReadElement(tinyxml2::XMLElement *element, Vec3<float> &output);
template <> bool ReadElement(tinyxml2::XMLElement *element, Rect<int> &output);
template <> bool ReadElement(tinyxml2::XMLElement *element, Rect<float> &output);

template <typename T> bool ReadElement(tinyxml2::XMLElement *element, T &output)
{
	if (!element)
	{
		LogError("Invalid element pointer");
		return false;
	}
	if (!element->GetText())
	{
		return false;
	}

	UString str = element->GetText();

	return FromString(str, output);
}

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
bool ReadElement(tinyxml2::XMLElement *element, const std::map<UString, T> &valueMap, T &output)
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

template <typename T>
bool ReadElement(tinyxml2::XMLElement *element, T &output, const T &defaultValue)
{
	if (ReadElement(element, output))
		return true;
	output = defaultValue;
	return false;
}
}; // namespace OpenApoc

#pragma once
#include <tinyxml2.h>
#include "library/strings.h"
#include "library/colour.h"
#include "framework/logger.h"

namespace OpenApoc
{
	bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, bool &output);
	bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, float &output);
	bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, Colour &output);
	bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, int &output);

	template <typename T>
	bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, T &output, const T &defaultValue)
	{
		if (ReadAttribute(element, attributeName, output))
			return true;
		output = defaultValue;
		return false;
	}

}; //namespace OpenApoc

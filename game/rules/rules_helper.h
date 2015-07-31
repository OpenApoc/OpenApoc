#pragma once
#include <tinyxml2.h>
#include "library/strings.h"
#include "library/colour.h"

namespace OpenApoc
{

	bool ReadAttributeBool(tinyxml2::XMLElement *element, const UString &attributeName, bool &output);
	bool ReadAttributeBool(tinyxml2::XMLElement *element, const UString &attributeName, bool &output, bool defaultValue);
	bool ReadAttributeFloat(tinyxml2::XMLElement *element, const UString &attributeName, float &output);
	bool ReadAttributeFloat(tinyxml2::XMLElement *element, const UString &attributeName, float &output, float defaultValue);
	bool ReadAttributeColour(tinyxml2::XMLElement *element, const UString &attributeName, Colour &output);
	bool ReadAttributeColour(tinyxml2::XMLElement *element, const UString &attributeName, Colour &output, Colour defaultValue);
	bool ReadAttributeInt(tinyxml2::XMLElement *element, const UString &attributeName, int &output);
	bool ReadAttributeInt(tinyxml2::XMLElement *element, const UString &attributeName, int &output, int defaultValue);

}; //namespace OpenApoc

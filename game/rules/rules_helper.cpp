#include "game/rules/rules_helper.h"

#include "framework/logger.h"

namespace OpenApoc
{

bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, bool &output)
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
	if (str == "true")
	{
		output = true;
		return true;
	}
	else if (str == "false")
	{
		output = false;
		return true;
	}

	LogWarning("Invalid string for bool attribute \"%s\" - \"%s\"", attributeName.c_str(),
	           str.c_str());
	return false;
}

bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, UString &output)
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

	output = element->Attribute(attributeName.c_str());

	return true;
}

bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, float &output)
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

	if (!Strings::IsFloat(str))
	{
		LogWarning("Element \"%s\" doesn't look like a float");
		return false;
	}

	output = Strings::ToFloat(str);
	return true;
}

bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, int &output)
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

	if (!Strings::IsInteger(str))
	{
		LogWarning("Element \"%s\" doesn't look like an integer");
		return false;
	}

	output = Strings::ToInteger(str);
	return true;
}

bool ReadAttribute(tinyxml2::XMLElement *element, const UString &attributeName, Colour &output)
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

	int r, g, b, a;

	auto splitString = str.split(",");

	if (splitString.size() != 3 && splitString.size() != 4)
	{
		LogWarning("Element \"%s\" doesn't look like a colour (invalid element count)");
		return false;
	}

	if (!Strings::IsInteger(splitString[0]) || !Strings::IsInteger(splitString[1]) ||
	    !Strings::IsInteger(splitString[2]))
	{
		LogWarning("Element \"%s\" doesn't look like a colour (non-integer element)");
		return false;
	}

	r = Strings::ToInteger(splitString[0]);
	g = Strings::ToInteger(splitString[1]);
	b = Strings::ToInteger(splitString[2]);

	if (splitString.size() == 4)
	{
		if (!Strings::IsInteger(splitString[3]))
		{
			LogWarning("Element \"%s\" doesn't look like a colour (non-integer element)");
			return false;
		}
		a = Strings::ToInteger(splitString[3]);
	}
	else
		a = 255;

	if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255)
	{
		LogWarning("Element \"%s\" doesn't look like a colour (out-of-range element)");
		return false;
	}

	if (!Strings::IsInteger(str))
	{
		LogWarning("Element \"%s\" doesn't look like an integer");
		return false;
	}

	output = Colour{static_cast<unsigned char>(r), static_cast<unsigned char>(g),
	                static_cast<unsigned char>(b), static_cast<unsigned char>(a)};
	return true;
}

}; // namespace OpenApoc

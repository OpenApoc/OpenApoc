#include "game/rules/rules_helper.h"

#include "framework/logger.h"

namespace OpenApoc
{

bool FromString(const UString &str, Vec2<int> &output)
{
	auto splitString = str.split(",");

	if (splitString.size() != 2)
	{
		LogWarning("String \"%s\" doesn't look like a vec2<int> (invalid element count)",
		           str.c_str());
		return false;
	}

	int x, y;
	if (!FromString(splitString[0], x) || !FromString(splitString[1], y))

	{
		LogWarning("String \"%s\" doesn't look like a vec2<int> (non-integer element)",
		           str.c_str());
		return false;
	}
	output = {x, y};
	return true;
}

bool FromString(const UString &str, Vec2<float> &output)
{
	auto splitString = str.split(",");

	if (splitString.size() != 2)
	{
		LogWarning("String \"%s\" doesn't look like a vec2<float> (invalid element count)",
		           str.c_str());
		return false;
	}

	float x, y;
	if (!FromString(splitString[0], x) || !FromString(splitString[1], y))
	{
		LogWarning("String \"%s\" doesn't look like a vec2<float> (non-float element)",
		           str.c_str());
		return false;
	}
	output = {x, y};
	return true;
}

bool FromString(const UString &str, Vec3<int> &output)
{
	auto splitString = str.split(",");

	if (splitString.size() != 3)
	{

		LogWarning("String \"%s\" doesn't look like a vec3<int> (invalid element count)",
		           str.c_str());
		return false;
	}

	int x, y, z;
	if (!FromString(splitString[0], x) || !FromString(splitString[1], y) ||
	    !FromString(splitString[2], z))
	{
		LogWarning("String \"%s\" doesn't look like a vec3<int> (non-integer element)",
		           str.c_str());
		return false;
	}
	output = Vec3<int>{x, y, z};
	return true;
}

bool FromString(const UString &str, Vec3<float> &output)
{
	auto splitString = str.split(",");

	if (splitString.size() != 3)
	{

		LogWarning("String \"%s\" doesn't look like a vec3<float> (invalid element count)",
		           str.c_str());
		return false;
	}

	float x, y, z;
	if (!FromString(splitString[0], x) || !FromString(splitString[1], y) ||
	    !FromString(splitString[2], z))
	{
		LogWarning("String \"%s\" doesn't look like a vec3<float> (non-integer element)",
		           str.c_str());
		return false;
	}
	output = Vec3<int>{x, y, z};
	return true;
}

bool FromString(const UString &str, bool &output)
{
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

	LogWarning("String \"%s\" doesn't liik like a bool", str.c_str());
	return false;
}

bool FromString(const UString &str, UString &output)
{
	output = str;
	return true;
}

bool FromString(const UString &str, float &output)
{
	if (!Strings::IsFloat(str))
	{
		LogWarning("String \"%s\" doesn't look like a float", str.c_str());
		return false;
	}

	output = Strings::ToFloat(str);
	return true;
}

bool FromString(const UString &str, int &output)
{
	if (!Strings::IsInteger(str))
	{
		LogWarning("Element \"%s\" doesn't look like an integer", str.c_str());
		return false;
	}

	output = Strings::ToInteger(str);
	return true;
}

bool FromString(const UString &str, Colour &output)
{
	int r, g, b, a;

	auto splitString = str.split(",");

	if (splitString.size() != 3 && splitString.size() != 4)
	{
		LogWarning("String \"%s\" doesn't look like a colour (invalid element count)", str.c_str());
		return false;
	}

	if (!Strings::IsInteger(splitString[0]) || !Strings::IsInteger(splitString[1]) ||
	    !Strings::IsInteger(splitString[2]))
	{
		LogWarning("String \"%s\" doesn't look like a colour (non-integer element)", str.c_str());
		return false;
	}

	r = Strings::ToInteger(splitString[0]);
	g = Strings::ToInteger(splitString[1]);
	b = Strings::ToInteger(splitString[2]);

	if (splitString.size() == 4)
	{
		if (!Strings::IsInteger(splitString[3]))
		{
			LogWarning("String \"%s\" doesn't look like a colour (non-integer element)",
			           str.c_str());
			return false;
		}
		a = Strings::ToInteger(splitString[3]);
	}
	else
		a = 255;

	if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255)
	{
		LogWarning("String \"%s\" doesn't look like a colour (out-of-range element)", str.c_str());
		return false;
	}

	if (!Strings::IsInteger(str))
	{
		LogWarning("String \"%s\" doesn't look like an integer", str.c_str());
		return false;
	}

	output = Colour{static_cast<unsigned char>(r), static_cast<unsigned char>(g),
	                static_cast<unsigned char>(b), static_cast<unsigned char>(a)};
	return true;
}

template <> bool ReadElement(tinyxml2::XMLElement *element, Vec3<int> &output)
{
	bool x_found = false;
	bool y_found = false;
	bool z_found = false;
	int x = 0, y = 0, z = 0;
	for (tinyxml2::XMLElement *node = element->FirstChildElement(); node != nullptr;
	     node = node->NextSiblingElement())
	{
		UString node_name = node->Name();
		if (node_name == "x")
		{
			if (x_found == true)
			{
				LogWarning("Multiple 'x' elements");
				return false;
			}
			x_found = true;
			if (!ReadElement(node, x))
			{
				LogWarning("Failed to parse 'x' element");
				return false;
			}
		}
		else if (node_name == "y")
		{
			if (y_found == true)
			{
				LogWarning("Multiple 'y' elements");
				return false;
			}
			y_found = true;
			if (!ReadElement(node, y))
			{
				LogWarning("Failed to parse 'y' element");
				return false;
			}
		}
		else if (node_name == "z")
		{
			if (z_found == true)
			{
				LogWarning("Multiple 'z' elements");
				return false;
			}
			z_found = true;
			if (!ReadElement(node, z))
			{
				LogWarning("Failed to parse 'z' element");
				return false;
			}
		}
		else
		{
			LogWarning("Unexpected node \"%s\"", node_name.c_str());
			return false;
		}
	}
	if (!x_found)
	{
		LogWarning("no \"x\" node");
		return false;
	}
	if (!y_found)
	{
		LogWarning("no \"y\" node");
		return false;
	}
	if (!z_found)
	{
		LogWarning("no \"z\" node");
		return false;
	}
	output = {x, y, z};
	return true;
}

template <> bool ReadElement(tinyxml2::XMLElement *element, Vec3<float> &output)
{
	bool x_found = false;
	bool y_found = false;
	bool z_found = false;
	float x = 0, y = 0, z = 0;
	for (tinyxml2::XMLElement *node = element->FirstChildElement(); node != nullptr;
	     node = node->NextSiblingElement())
	{
		UString node_name = node->Name();
		if (node_name == "x")
		{
			if (x_found == true)
			{
				LogWarning("Multiple 'x' elements");
				return false;
			}
			x_found = true;
			if (!ReadElement(node, x))
			{
				LogWarning("Failed to parse 'x' element");
				return false;
			}
		}
		else if (node_name == "y")
		{
			if (y_found == true)
			{
				LogWarning("Multiple 'y' elements");
				return false;
			}
			y_found = true;
			if (!ReadElement(node, y))
			{
				LogWarning("Failed to parse 'y' element");
				return false;
			}
		}
		else if (node_name == "z")
		{
			if (z_found == true)
			{
				LogWarning("Multiple 'z' elements");
				return false;
			}
			z_found = true;
			if (!ReadElement(node, z))
			{
				LogWarning("Failed to parse 'z' element");
				return false;
			}
		}
		else
		{
			LogWarning("Unexpected node \"%s\"", node_name.c_str());
			return false;
		}
	}
	if (!x_found)
	{
		LogWarning("no \"x\" node");
		return false;
	}
	if (!y_found)
	{
		LogWarning("no \"y\" node");
		return false;
	}
	if (!z_found)
	{
		LogWarning("no \"z\" node");
		return false;
	}
	output = {x, y, z};
	return true;
}

template <> bool ReadElement(tinyxml2::XMLElement *element, Vec2<int> &output)
{
	bool x_found = false;
	bool y_found = false;
	int x = 0, y = 0;
	for (tinyxml2::XMLElement *node = element->FirstChildElement(); node != nullptr;
	     node = node->NextSiblingElement())
	{
		UString node_name = node->Name();
		if (node_name == "x")
		{
			if (x_found == true)
			{
				LogWarning("Multiple 'x' elements");
				return false;
			}
			x_found = true;
			if (!ReadElement(node, x))
			{
				LogWarning("Failed to parse 'x' element");
				return false;
			}
		}
		else if (node_name == "y")
		{
			if (y_found == true)
			{
				LogWarning("Multiple 'y' elements");
				return false;
			}
			y_found = true;
			if (!ReadElement(node, y))
			{
				LogWarning("Failed to parse 'y' element");
				return false;
			}
		}
		else
		{
			LogWarning("Unexpected node \"%s\"", node_name.c_str());
			return false;
		}
	}
	if (!x_found)
	{
		LogWarning("no \"x\" node");
		return false;
	}
	if (!y_found)
	{
		LogWarning("no \"y\" node");
		return false;
	}
	output = {x, y};
	return true;
}

template <> bool ReadElement(tinyxml2::XMLElement *element, Vec2<float> &output)
{
	bool x_found = false;
	bool y_found = false;
	float x = 0, y = 0;
	for (tinyxml2::XMLElement *node = element->FirstChildElement(); node != nullptr;
	     node = node->NextSiblingElement())
	{
		UString node_name = node->Name();
		if (node_name == "x")
		{
			if (x_found == true)
			{
				LogWarning("Multiple 'x' elements");
				return false;
			}
			x_found = true;
			if (!ReadElement(node, x))
			{
				LogWarning("Failed to parse 'x' element");
				return false;
			}
		}
		else if (node_name == "y")
		{
			if (y_found == true)
			{
				LogWarning("Multiple 'y' elements");
				return false;
			}
			y_found = true;
			if (!ReadElement(node, y))
			{
				LogWarning("Failed to parse 'y' element");
				return false;
			}
		}
		else
		{
			LogWarning("Unexpected node \"%s\"", node_name.c_str());
			return false;
		}
	}
	if (!x_found)
	{
		LogWarning("no \"x\" node");
		return false;
	}
	if (!y_found)
	{
		LogWarning("no \"y\" node");
		return false;
	}
	output = {x, y};
	return true;
}

template <> bool ReadElement(tinyxml2::XMLElement *element, Rect<int> &output)
{
	bool p0_found = false;
	bool p1_found = false;
	Vec2<int> p0, p1;
	for (tinyxml2::XMLElement *node = element->FirstChildElement(); node != nullptr;
	     node = node->NextSiblingElement())
	{
		UString node_name = node->Name();
		if (node_name == "p0")
		{
			if (p0_found == true)
			{
				LogWarning("Multiple 'p0' elements");
				return false;
			}
			p0_found = true;
			if (!ReadElement(node, p0))
			{
				LogWarning("Failed to parse 'p0' element");
				return false;
			}
		}
		else if (node_name == "p1")
		{
			if (p1_found == true)
			{
				LogWarning("Multiple 'p1' elements");
				return false;
			}
			p1_found = true;
			if (!ReadElement(node, p1))
			{
				LogWarning("Failed to parse 'p1' element");
				return false;
			}
		}
		else
		{
			LogWarning("Unexpected node \"%s\"", node_name.c_str());
			return false;
		}
	}
	if (!p1_found)
	{
		LogWarning("no \"p1\" node");
		return false;
	}
	if (!p1_found)
	{
		LogWarning("no \"p1\" node");
		return false;
	}
	output = {p0, p1};
	return true;
}

template <> bool ReadElement(tinyxml2::XMLElement *element, Rect<float> &output)
{
	bool p0_found = false;
	bool p1_found = false;
	Vec2<float> p0, p1;
	for (tinyxml2::XMLElement *node = element->FirstChildElement(); node != nullptr;
	     node = node->NextSiblingElement())
	{
		UString node_name = node->Name();
		if (node_name == "p0")
		{
			if (p0_found == true)
			{
				LogWarning("Multiple 'p0' elements");
				return false;
			}
			p0_found = true;
			if (!ReadElement(node, p0))
			{
				LogWarning("Failed to parse 'p0' element");
				return false;
			}
		}
		else if (node_name == "p1")
		{
			if (p1_found == true)
			{
				LogWarning("Multiple 'p1' elements");
				return false;
			}
			p1_found = true;
			if (!ReadElement(node, p1))
			{
				LogWarning("Failed to parse 'p1' element");
				return false;
			}
		}
		else
		{
			LogWarning("Unexpected node \"%s\"", node_name.c_str());
			return false;
		}
	}
	if (!p1_found)
	{
		LogWarning("no \"p1\" node");
		return false;
	}
	if (!p1_found)
	{
		LogWarning("no \"p1\" node");
		return false;
	}
	output = {p0, p1};
	return true;
}

}; // namespace OpenApoc

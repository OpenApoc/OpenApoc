#include "library/sp.h"

#include "framework/framework.h"
#include "framework/image.h"
#include "framework/palette.h"
#include "game/apocresources/apocfont.h"

#include <boost/locale.hpp>

namespace OpenApoc
{

sp<BitmapFont> ApocalypseFont::loadFont(tinyxml2::XMLElement *fontElement)
{
	int spacewidth = 0;
	int height = 0;
	UString fontName;

	std::map<UniChar, UString> charMap;

	const char *attr = fontElement->Attribute("name");
	if (!attr)
	{
		LogError("apocfont element with no \"name\" attribute");
		return nullptr;
	}
	fontName = attr;

	auto err = fontElement->QueryIntAttribute("height", &height);
	if (err != tinyxml2::XML_NO_ERROR || height <= 0)
	{
		LogError("apocfont \"%s\" with invalid \"height\" attribute", fontName.c_str());
		return nullptr;
	}
	err = fontElement->QueryIntAttribute("spacewidth", &spacewidth);
	if (err != tinyxml2::XML_NO_ERROR || spacewidth <= 0)
	{
		LogError("apocfont \"%s\" with invalid \"spacewidth\" attribute", fontName.c_str());
		return nullptr;
	}

	for (auto *glyphNode = fontElement->FirstChildElement(); glyphNode;
	     glyphNode = glyphNode->NextSiblingElement())
	{
		UString nodeName = glyphNode->Name();
		if (nodeName != "glyph")
		{
			LogError("apocfont \"%s\" has unexpected child node \"%s\", skipping", fontName.c_str(),
			         nodeName.c_str());
			continue;
		}
		auto *glyphPath = glyphNode->Attribute("glyph");
		if (!glyphPath)
		{
			LogError("Font \"%s\" has glyph with missing string attribute - skipping glyph",
			         fontName.c_str());
			continue;
		}
		auto *glyphString = glyphNode->Attribute("string");
		if (!glyphString)
		{
			LogError("apocfont \"%s\" has glyph with missing string attribute - skipping glyph",
			         fontName.c_str());
			continue;
		}

		auto pointString = boost::locale::conv::utf_to_utf<UniChar>(glyphString);

		if (pointString.length() != 1)
		{
			LogError("apocfont \"%s\" glyph \"%s\" has %d codepoints, expected one - skipping "
			         "glyph",
			         fontName.c_str(), glyphString, pointString.length());
			continue;
		}
		UniChar c = pointString[0];

		if (charMap.find(c) != charMap.end())
		{
			LogError("Font \"%s\" has multiple glyphs for string \"%s\" - skipping glyph",
			         fontName.c_str(), glyphString);
			continue;
		}

		charMap[c] = glyphPath;
	}

	auto font = BitmapFont::loadFont(charMap, spacewidth, height, fontName,
	                                 fw().data->load_palette(fontElement->Attribute("palette")));
	return font;
}
}; // namespace OpenApoc

#include "framework/apocresources/apocfont.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "framework/data.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "library/sp.h"

#include <boost/locale.hpp>

namespace OpenApoc
{

sp<BitmapFont> ApocalypseFont::loadFont(const UString &fontDescPath)
{
	auto file = fw().data->fs.open(fontDescPath);
	if (!file)
	{
		LogWarning("Failed to open font file at path \"%s\"", fontDescPath);
		return nullptr;
	}

	auto data = file.readAll();
	if (!data)
	{
		LogWarning("Failed to read font file at path \"%s\"", fontDescPath);
		return nullptr;
	}

	pugi::xml_document doc;

	auto parseResult = doc.load_buffer(data.get(), file.size());

	if (!parseResult)
	{
		LogWarning("Failed to parse font file at \"%s\" - \"%s\" at \"%llu\"", fontDescPath,
		           parseResult.description(), (unsigned long long)parseResult.offset);
		return nullptr;
	}

	auto openapocNode = doc.child("openapoc");
	if (!openapocNode)
	{
		LogWarning("Failed to find \"openapoc\" root node in font file at \"%s\"", fontDescPath);
		return nullptr;
	}

	auto fontNode = openapocNode.child("apocfont");
	if (!fontNode)
	{
		LogWarning("Failed to find \"openapoc::apocfont\" node in font file at \"%s\"",
		           fontDescPath);
		return nullptr;
	}

	int spacewidth = 0;
	int height = 0;
	int kerning = 0;
	UString fontName;

	std::map<char32_t, UString> charMap;

	fontName = fontNode.attribute("name").value();
	if (fontName.empty())
	{
		LogError("apocfont element with no \"name\" attribute");
		return nullptr;
	}

	height = fontNode.attribute("height").as_int();
	if (height <= 0)
	{
		LogError("apocfont \"%s\" with invalid \"height\" attribute", fontName);
		return nullptr;
	}
	spacewidth = fontNode.attribute("spacewidth").as_int();
	if (spacewidth <= 0)
	{
		LogError("apocfont \"%s\" with invalid \"spacewidth\" attribute", fontName);
		return nullptr;
	}
	kerning = fontNode.attribute("kerning").as_int();
	if (kerning <= 0)
	{
		LogError("apocfont \"%s\" with invalid \"kerning\" attribute", fontName);
		return nullptr;
	}

	for (auto glyphNode = fontNode.child("glyph"); glyphNode;
	     glyphNode = glyphNode.next_sibling("glyph"))
	{
		UString glyphPath = glyphNode.attribute("glyph").value();
		if (glyphPath.empty())
		{
			LogError("Font \"%s\" has glyph with missing string attribute - skipping glyph",
			         fontName);
			continue;
		}
		UString glyphString = glyphNode.attribute("string").value();
		if (glyphString.empty())
		{
			LogError("apocfont \"%s\" has glyph with missing string attribute - skipping glyph",
			         fontName);
			continue;
		}

		auto pointString = boost::locale::conv::utf_to_utf<char32_t>(glyphString.c_str());

		if (pointString.length() != 1)
		{
			LogError("apocfont \"%s\" glyph \"%s\" has %lu codepoints, expected one - skipping "
			         "glyph",
			         fontName, glyphString, pointString.length());
			continue;
		}
		char32_t c = pointString[0];

		if (charMap.find(c) != charMap.end())
		{
			LogError("Font \"%s\" has multiple glyphs for string \"%s\" - skipping glyph", fontName,
			         glyphString);
			continue;
		}

		charMap[c] = glyphPath;
	}

	auto font = BitmapFont::loadFont(charMap, spacewidth, height, kerning, fontName,
	                                 fw().data->loadPalette(fontNode.attribute("palette").value()));
	return font;
}
}; // namespace OpenApoc

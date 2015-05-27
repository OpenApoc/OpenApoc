
#include "game/apocresources/apocfont.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/palette.h"

namespace OpenApoc {

std::shared_ptr<ApocalypseFont>
ApocalypseFont::loadFont( Framework &fw, tinyxml2::XMLElement *fontElement)
{
	int spacewidth = 0;
	int height = 0;
	int width = 0;
	UString fileName;
	UString fontName;

	const char *attr = fontElement->Attribute("name");
	if (!attr)
	{
		LogError("apocfont element with no \"name\" attribute");
		return nullptr;
	}
	fontName = attr;
	attr = fontElement->Attribute("path");
	if (!attr)
	{
		LogError("apocfont \"%s\" with no \"path\" attribute", fontName.str().c_str());
		return nullptr;
	}
	fileName = attr;
	
	auto err = fontElement->QueryIntAttribute("height", &height);
	if (err != tinyxml2::XML_NO_ERROR || height <= 0)
	{
		LogError("apocfont \"%s\" with invalid \"height\" attribute", fontName.str().c_str());
		return nullptr;
	}
	err = fontElement->QueryIntAttribute("width", &width);
	if (err != tinyxml2::XML_NO_ERROR || width <= 0)
	{
		LogError("apocfont \"%s\" with invalid \"width\" attribute", fontName.str().c_str());
		return nullptr;
	}
	err = fontElement->QueryIntAttribute("spacewidth", &spacewidth);
	if (err != tinyxml2::XML_NO_ERROR || spacewidth <= 0)
	{
		LogError("apocfont \"%s\" with invalid \"spacewidth\" attribute", fontName.str().c_str());
		return nullptr;
	}

	auto *file = fw.data->load_file(fileName, Data::FileMode::Read);
	if (!file)
	{
		LogError("apocfont \"%s\" - Failed to open font path \"%s\"", fontName.str().c_str(), fileName.str().c_str());
		return nullptr;
	}

	auto fileSize = PHYSFS_fileLength(file);
	int glyphSize = height * width;
	int glyphCount = fileSize / glyphSize;

	if (!glyphCount)
	{
		LogError("apocfont \"%s\" - file \"%s\" contains no glyphs", fontName.str().c_str(), fileName.str().c_str());
	}
	std::shared_ptr<ApocalypseFont> font(new ApocalypseFont);

	font->name = fontName;
	font->spacewidth = spacewidth;
	font->fontheight = height;

	for (auto *glyphNode = fontElement->FirstChildElement(); glyphNode; glyphNode = glyphNode->NextSiblingElement())
	{
		UString nodeName = glyphNode->Name();
		if (nodeName != "glyph")
		{
			LogError("apocfont \"%s\" has unexpected child node \"%s\", skipping", fontName.str().c_str(), nodeName.str().c_str());
			continue;
		}
		int offset;
		err = glyphNode->QueryIntAttribute("offset", &offset);
		if (err != tinyxml2::XML_NO_ERROR)
		{
			LogError("apocfont \"%s\" has glyph with invalid/missing offset attribute - skipping glyph", fontName.str().c_str());
			continue;
		}
		attr = glyphNode->Attribute("string");
		if (!attr)
		{
			LogError("apocfont \"%s\" has glyph with missing string attribute - skipping glyph", fontName.str().c_str());
			continue;
		}

		UString glyphString(attr);
		if (glyphString.length() != 1)
		{
			LogError("apocfont \"%s\" glyph w/offset %d has %d codepoints, expected one - skipping glyph", fontName.str().c_str(), offset, glyphString.length());
			continue;
		}
		if (offset >= glyphCount)
		{
			LogError("apocfont \"%s\" glyph \"%s\" has invalid offset %d - file contains a max of %d - skipping glyph", fontName.str().c_str(), glyphString.str().c_str(), offset, glyphCount);
			continue;
		}
		UniChar c = glyphString[0];
		if (font->fontbitmaps.find(c) != font->fontbitmaps.end())
		{
			LogError("apocfont \"%s\" glyph \"%s\" has multiple definitions - skipping re-definition", fontName.str().c_str(), glyphString.str().c_str());
			continue;
		}
		PHYSFS_seek(file, glyphSize * offset);
		int glyphWidth = 0;

		auto glyphImage = std::make_shared<PaletteImage>(Vec2<int>(width,height));	
		{
			PaletteImageLock imgLock(glyphImage, ImageLockUse::Write);


			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					uint8_t idx;
					PHYSFS_readBytes(file, (char*)&idx, 1);
					imgLock.set(Vec2<int>{x,y}, idx);
					if (idx != 0 && glyphWidth < x)
						glyphWidth = x;
				}
			}
		}
		auto trimmedGlyph = std::make_shared<PaletteImage>(Vec2<int>{glyphWidth + 2, height});
		PaletteImage::blit(glyphImage, Vec2<int>{0,0}, trimmedGlyph);
		font->fontbitmaps[c] = trimmedGlyph;
	}

	//FIXME: Bit of a hack to handle spaces?
	auto spaceImage = std::make_shared<PaletteImage>(Vec2<int>{spacewidth,height});
	//Defaults to transparent (0)
	font->fontbitmaps[UString::u8Char(' ')] = spaceImage;


	PHYSFS_close(file);

	return font;
}

std::shared_ptr<PaletteImage>
ApocalypseFont::getGlyph(UniChar codepoint)
{
	if (fontbitmaps.find(codepoint) == fontbitmaps.end())
	{
		//FIXME: Hack - assume all missing glyphs are spaces
		//TODO: Fallback fonts?
		LogError("Font %s missing glyph for character \"%s\"", this->getName().str().c_str(), UString(codepoint).str().c_str());
		return this->getGlyph(UString::u8Char(' '));
	}
	return fontbitmaps[codepoint];
}

ApocalypseFont::~ApocalypseFont()
{
}

int ApocalypseFont::GetFontHeight()
{
	return fontheight;
}

UString ApocalypseFont::getName()
{
	return this->name;
}



}; //namespace OpenApoc

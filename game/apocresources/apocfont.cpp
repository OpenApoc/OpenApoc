
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
	fontName = U8Str(attr);
	attr = fontElement->Attribute("path");
	if (!attr)
	{
		LogError("apocfont \"%s\" with no \"path\" attribute", fontName.getTerminatedBuffer());
		return nullptr;
	}
	fileName = U8Str(attr);
	
	auto err = fontElement->QueryIntAttribute("height", &height);
	if (err != tinyxml2::XML_NO_ERROR || height <= 0)
	{
		LogError("apocfont \"%s\" with invalid \"height\" attribute", fontName.getTerminatedBuffer());
		return nullptr;
	}
	err = fontElement->QueryIntAttribute("width", &width);
	if (err != tinyxml2::XML_NO_ERROR || width <= 0)
	{
		LogError("apocfont \"%s\" with invalid \"width\" attribute", fontName.getTerminatedBuffer());
		return nullptr;
	}
	err = fontElement->QueryIntAttribute("spacewidth", &spacewidth);
	if (err != tinyxml2::XML_NO_ERROR || spacewidth <= 0)
	{
		LogError("apocfont \"%s\" with invalid \"spacewidth\" attribute", fontName.getTerminatedBuffer());
		return nullptr;
	}

	auto *file = fw.data->load_file(fileName, Data::FileMode::Read);
	if (!file)
	{
		LogError("apocfont \"%S\" - Failed to open font path \"%S\"", fontName.getTerminatedBuffer(), fileName.getTerminatedBuffer());
		return nullptr;
	}

	auto fileSize = PHYSFS_fileLength(file);
	int glyphSize = height * width;
	int glyphCount = fileSize / glyphSize;

	if (!glyphCount)
	{
		LogError("apocfont \"%S\" - file \"%S\" contains no glyphs", fontName.getTerminatedBuffer(), fileName.getTerminatedBuffer());
	}
	std::shared_ptr<ApocalypseFont> font(new ApocalypseFont);

	font->name = fontName;
	font->spacewidth = spacewidth;
	font->fontheight = height;

	for (auto *glyphNode = fontElement->FirstChildElement(); glyphNode; glyphNode = glyphNode->NextSiblingElement())
	{
		UString nodeName = U8Str(glyphNode->Name());
		if (nodeName != "glyph")
		{
			LogError("apocfont \"%S\" has unexpected child node \"%S\", skipping", fontName.getTerminatedBuffer(), nodeName.getTerminatedBuffer());
			continue;
		}
		int offset;
		err = glyphNode->QueryIntAttribute("offset", &offset);
		if (err != tinyxml2::XML_NO_ERROR)
		{
			LogError("apocfont \"%S\" has glyph with invalid/missing offset attribute - skipping glyph", fontName.getTerminatedBuffer());
			continue;
		}
		attr = glyphNode->Attribute("string");
		if (!attr)
		{
			LogError("apocfont \"%S\" has glyph with missing string attribute - skipping glyph", fontName.getTerminatedBuffer());
			continue;
		}

		icu::UnicodeString glyphString(attr, "UTF-8");
		if (glyphString.length() != 1)
		{
			LogError("apocfont \"%S\" glyph w/offset %d has %d codepoints, expected one - skipping glyph", fontName.getTerminatedBuffer(), offset, glyphString.length());
			continue;
		}
		if (offset >= glyphCount)
		{
			LogError("apocfont \"%S\" glyph \"%S\" has invalid offset %d - file contains a max of %d - skipping glyph", fontName.getTerminatedBuffer(), glyphString.getTerminatedBuffer(), offset, glyphCount);
			continue;
		}
		UChar c = glyphString.charAt(0);
		if (font->fontbitmaps.find(c) != font->fontbitmaps.end())
		{
			LogError("apocfont \"%S\" glyph \"%S\" has multiple definitions - skipping re-definition", fontName.getTerminatedBuffer(), glyphString.getTerminatedBuffer());
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
	icu::UnicodeString spaceString(" ", "UTF-8");
	font->fontbitmaps[spaceString.charAt(0)] = spaceImage;


	PHYSFS_close(file);

	return font;
}

std::shared_ptr<PaletteImage>
ApocalypseFont::getGlyph(UChar codepoint)
{
	if (fontbitmaps.find(codepoint) == fontbitmaps.end())
	{
		//FIXME: Hack - assume all missing glyphs are spaces
		//TODO: Fallback fonts?
		icu::UnicodeString glyphString;
		glyphString.append(codepoint);
		
		LogError("Font %S missing glyph for character \"%S\"", this->getName().getTerminatedBuffer(), glyphString.getTerminatedBuffer());
		glyphString = icu::UnicodeString(" ", "UTF-8");
		return this->getGlyph(glyphString.charAt(0));
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

#include "framework/font.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "library/sp.h"

// Disable automatic #pragma linking for boost - only enabled in msvc and that should provide boost
// symbols as part of the module that uses it
#define BOOST_ALL_NO_LIB
#include <boost/locale.hpp>

namespace OpenApoc
{

BitmapFont::~BitmapFont() {}

sp<PaletteImage> BitmapFont::getString(const UString &Text)
{
	int height = this->GetFontHeight();
	int width = this->GetFontWidth(Text);
	auto img = mksp<PaletteImage>(Vec2<int>{width, height});
	int pos = 0;

	auto u8Str = Text.str();
	auto pointString = boost::locale::conv::utf_to_utf<UniChar>(u8Str);

	for (size_t i = 0; i < pointString.length(); i++)
	{
		UniChar c = pointString[i];
		auto glyph = this->getGlyph(c);
		PaletteImage::blit(glyph, img, {0, 0}, {pos, 0});
		pos += glyph->size.x;
	}

	return img;
}

int BitmapFont::GetFontWidth(const UString &Text)
{
	int textlen = 0;
	auto u8Str = Text.str();
	auto pointString = boost::locale::conv::utf_to_utf<UniChar>(u8Str);

	for (size_t i = 0; i < Text.length(); i++)
	{
		auto glyph = this->getGlyph(pointString[i]);
		textlen += glyph->size.x;
	}
	return textlen;
}

int BitmapFont::GetFontHeight() const { return fontheight; }

int BitmapFont::GetFontHeight(const UString &Text, int MaxWidth)
{
	std::list<UString> lines = WordWrapText(Text, MaxWidth);
	return lines.size() * fontheight;
}

UString BitmapFont::getName() const { return this->name; }

int BitmapFont::GetEstimateCharacters(int FitInWidth) const
{
	return FitInWidth / averagecharacterwidth;
}

sp<PaletteImage> BitmapFont::getGlyph(UniChar codepoint)
{
	if (fontbitmaps.find(codepoint) == fontbitmaps.end())
	{
		// FIXME: Hack - assume all missing glyphs are spaces
		// TODO: Fallback fonts?
		LogWarning("Font %s missing glyph for character \"%s\" (codepoint %u)",
		           this->getName().c_str(), UString(codepoint).c_str(), codepoint);
		auto missingGlyph = this->getGlyph(UString::u8Char(' '));
		fontbitmaps.emplace(codepoint, missingGlyph);
	}
	return fontbitmaps[codepoint];
}

sp<Palette> BitmapFont::getPalette() const { return this->palette; }

sp<BitmapFont> BitmapFont::loadFont(const std::map<UniChar, UString> &glyphMap, int spaceWidth,
                                    int fontHeight, UString fontName, sp<Palette> defaultPalette)
{
	auto font = mksp<BitmapFont>();

	font->spacewidth = spaceWidth;
	font->fontheight = fontHeight;
	font->averagecharacterwidth = 0;
	font->name = fontName;
	font->palette = defaultPalette;

	size_t totalGlyphWidth = 0;

	for (auto &p : glyphMap)
	{
		auto fontImage = fw().data->load_image(p.second);
		if (!fontImage)
		{
			LogError("Failed to read glyph image \"%s\"", p.second.c_str());
			continue;
		}
		auto paletteImage = std::dynamic_pointer_cast<PaletteImage>(fontImage);
		if (!paletteImage)
		{
			LogError("Glyph image \"%s\" doesn't look like a PaletteImage", p.second.c_str());
			continue;
		}
		int maxWidth = 0;

		// FIXME: Proper kerning
		// First find the widest non-transparent part of the glyph
		{
			PaletteImageLock imgLock(paletteImage, ImageLockUse::Read);
			for (unsigned int y = 0; y < paletteImage->size.y; y++)
			{
				for (unsigned int x = 0; x < paletteImage->size.x; x++)
				{
					if (imgLock.get({x, y}) != 0)
					{
						if (x > maxWidth)
							maxWidth = x;
					}
				}
			}
		}
		// Trim the glyph to the max non-transparent width + 2 px
		auto trimmedGlyph = mksp<PaletteImage>(Vec2<int>{maxWidth + 2, fontHeight});
		PaletteImage::blit(paletteImage, trimmedGlyph);

		font->fontbitmaps[p.first] = trimmedGlyph;
		totalGlyphWidth += trimmedGlyph->size.x;
	}

	font->averagecharacterwidth = totalGlyphWidth / font->fontbitmaps.size();

	// Always add a 'space' image:

	auto spaceImage = mksp<PaletteImage>(Vec2<int>{spaceWidth, fontHeight});
	font->fontbitmaps[UString::u8Char(' ')] = spaceImage;

	return font;
}

std::list<UString> BitmapFont::WordWrapText(const UString &Text, int MaxWidth)
{
	int txtwidth;
	std::list<UString> lines;

	txtwidth = GetFontWidth(Text);

	if (txtwidth > MaxWidth)
	{
		// TODO: Need to implement a list of line break characters
		auto remainingChunks = Text.splitlist(" ");
		UString currentLine;

		while (!remainingChunks.empty())
		{
			UString currentTestLine;
			if (currentLine != "")
				currentTestLine = currentLine + " ";

			auto &currentChunk = remainingChunks.front();
			currentTestLine += currentChunk;

			auto estimatedLength = GetFontWidth(currentTestLine);

			if (estimatedLength < MaxWidth)
			{
				currentLine = currentTestLine;
				remainingChunks.pop_front();
			}
			else
			{
				if (currentLine == "")
				{
					LogWarning(
					    "No break in line \"%s\" found - this will probably overflow the control",
					    currentTestLine.c_str());
					currentLine = currentTestLine;
					remainingChunks.pop_front();
				}
				else
				{
					lines.push_back(currentLine);
					currentLine = "";
				}
			}
		}
		if (currentLine != "")
			lines.push_back(currentLine);
	}
	else
	{
		lines.push_back(Text);
	}

	return lines;
}

}; // namespace OpenApoc

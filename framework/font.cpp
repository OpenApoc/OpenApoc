#include "framework/font.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "library/sp.h"

namespace OpenApoc
{

BitmapFont::~BitmapFont() = default;

sp<PaletteImage> BitmapFont::getString(const UString &Text)
{
	int height = this->getFontHeight();
	int width = this->getFontWidth(Text);
	int pos = 0;

	auto img = fw().data->getFontStringCacheEntry(this->name, Text);
	if (img)
		return img;

	img = mksp<PaletteImage>(Vec2<int>{width, height});

	for (const auto &c : Text)
	{
		auto glyph = this->getGlyph(c);
		PaletteImage::blit(glyph, img, {0, 0}, {pos, 0});
		pos += glyph->size.x;
	}

	fw().data->putFontStringCacheEntry(this->name, Text, img);

	return img;
}

int BitmapFont::getFontWidth(const UString &Text)
{
	int textlen = 0;

	for (const auto &c : Text)
	{
		auto glyph = this->getGlyph(c);
		textlen += glyph->size.x;
	}
	return textlen;
}

int BitmapFont::getFontHeight() const { return fontheight; }

int BitmapFont::getFontHeight(const UString &Text, int MaxWidth)
{
	std::list<UString> lines = wordWrapText(Text, MaxWidth);
	return lines.size() * fontheight;
}

UString BitmapFont::getName() const { return this->name; }

int BitmapFont::getEstimateCharacters(int FitInWidth) const
{
	return FitInWidth / averagecharacterwidth;
}

sp<PaletteImage> BitmapFont::getGlyph(UniChar codepoint)
{
	if (fontbitmaps.find(codepoint) == fontbitmaps.end())
	{
		// FIXME: Hack - assume all missing glyphs are spaces
		// TODO: Fallback fonts?
		LogWarning("Font %s missing glyph for character \"%s\" (codepoint %u)", this->getName(),
		           UString(codepoint), codepoint);
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
		auto fontImage = fw().data->loadImage(p.second);
		if (!fontImage)
		{
			LogError("Failed to read glyph image \"%s\"", p.second);
			continue;
		}
		auto paletteImage = std::dynamic_pointer_cast<PaletteImage>(fontImage);
		if (!paletteImage)
		{
			LogError("Glyph image \"%s\" doesn't look like a PaletteImage", p.second);
			continue;
		}
		unsigned int maxWidth = 0;

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

std::list<UString> BitmapFont::wordWrapText(const UString &Text, int MaxWidth)
{
	int txtwidth;
	std::list<UString> lines = Text.splitlist("\n");
	std::list<UString> wrappedLines;

	for (UString str : lines)
	{
		txtwidth = getFontWidth(str);

		if (txtwidth > MaxWidth)
		{
			auto remainingChunks = str.splitlist(" ");
			UString currentLine;

			while (!remainingChunks.empty())
			{
				UString currentTestLine;
				if (currentLine != "")
					currentTestLine = currentLine + " ";

				auto &currentChunk = remainingChunks.front();
				currentTestLine += currentChunk;

				auto estimatedLength = getFontWidth(currentTestLine);

				if (estimatedLength < MaxWidth)
				{
					currentLine = currentTestLine;
					remainingChunks.pop_front();
				}
				else
				{
					if (currentLine == "")
					{
						LogWarning("No break in line \"%s\" found - this will probably overflow "
						           "the control",
						           currentTestLine);
						currentLine = currentTestLine;
						remainingChunks.pop_front();
					}
					else
					{
						wrappedLines.push_back(currentLine);
						currentLine = "";
					}
				}
			}
			if (currentLine != "")
				wrappedLines.push_back(currentLine);
		}
		else
		{
			wrappedLines.push_back(str);
		}
	}

	return wrappedLines;
}

}; // namespace OpenApoc

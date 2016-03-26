#include "framework/font.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "library/sp.h"

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
		PaletteImage::blit(glyph, Vec2<int>{pos, 0}, img);
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

int BitmapFont::GetFontHeight() { return fontheight; }

UString BitmapFont::getName() { return this->name; }

int BitmapFont::GetEstimateCharacters(int FitInWidth) { return FitInWidth / averagecharacterwidth; }

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

sp<Palette> BitmapFont::getPalette() { return this->palette; }

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
			for (int y = 0; y < paletteImage->size.y; y++)
			{
				for (int x = 0; x < paletteImage->size.x; x++)
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
		PaletteImage::blit(paletteImage, {0, 0}, trimmedGlyph);

		font->fontbitmaps[p.first] = trimmedGlyph;
		totalGlyphWidth += trimmedGlyph->size.x;
	}

	font->averagecharacterwidth = totalGlyphWidth / font->fontbitmaps.size();

	// Always add a 'space' image:

	auto spaceImage = mksp<PaletteImage>(Vec2<int>{spaceWidth, fontHeight});
	font->fontbitmaps[UString::u8Char(' ')] = spaceImage;

	return font;
}

}; // namespace OpenApoc

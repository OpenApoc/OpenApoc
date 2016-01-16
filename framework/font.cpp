#include "library/sp.h"
#include "framework/framework.h"
#include "framework/image.h"

#include <boost/locale.hpp>

namespace OpenApoc
{

BitmapFont::~BitmapFont() {}

sp<PaletteImage> BitmapFont::getString(const UString &Text)
{
	int height = this->GetFontHeight();
	int width = this->GetFontWidth(Text);
	auto img = std::make_shared<PaletteImage>(Vec2<int>{width, height});
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

}; // namespace OpenApoc

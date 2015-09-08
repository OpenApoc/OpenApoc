#include "framework/framework.h"
#include "framework/image.h"

namespace OpenApoc
{

BitmapFont::~BitmapFont() {}

std::shared_ptr<PaletteImage> BitmapFont::getString(const UString &Text)
{
	int height = this->GetFontHeight();
	int width = this->GetFontWidth(Text);
	auto img = std::make_shared<PaletteImage>(Vec2<int>{width, height});
	int pos = 0;

	for (size_t i = 0; i < Text.length(); i++)
	{
		UniChar c = Text[i];
		auto glyph = this->getGlyph(c);
		PaletteImage::blit(glyph, Vec2<int>{pos, 0}, img);
		pos += glyph->size.x;
	}

	return img;
}

int BitmapFont::GetFontWidth(const UString &Text)
{
	int textlen = 0;
	for (size_t i = 0; i < Text.length(); i++)
	{
		auto glyph = this->getGlyph(Text[i]);
		textlen += glyph->size.x;
	}
	return textlen;
}

}; // namespace OpenApoc

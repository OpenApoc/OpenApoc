#include "framework/framework.h"
#include "framework/image.h"

namespace OpenApoc {

BitmapFont::~BitmapFont()
{

}

std::shared_ptr<PaletteImage>
BitmapFont::getString(std::string Text)
{
	int height = this->GetFontHeight();
	int width = this->GetFontWidth(Text);
	auto img = std::make_shared<PaletteImage>(Vec2<int>{width, height});
	int pos = 0;

	icu::UnicodeString str(Text.c_str(), "UTF-8");

	for (int i = 0; i < str.length(); i++)
	{
		UChar c = str.charAt(i);
		auto glyph = this->getGlyph(c);
		PaletteImage::blit(glyph, Vec2<int>{pos, 0}, img);
		pos += glyph->size.x;
	}


	return img;
}

int BitmapFont::GetFontWidth( std::string Text )
{
	int textlen = 0;
	icu::UnicodeString str(Text.c_str(), "UTF-8");
	for( int i = 0; i < str.length(); i++ )
	{
		auto glyph = this->getGlyph(str.charAt(i));
		textlen += glyph->size.x;
	}
	return textlen;
}

}; //namespace OpenApoc

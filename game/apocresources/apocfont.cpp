
#include "apocfont.h"
#include "framework/framework.h"
#include "framework/palette.h"

namespace OpenApoc {

// TODO: Fix charstring
std::string ApocalypseFont::FontCharacterSet = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ??ÙÚÛÜÝÞŸßàáâãäåæçèéêëìíîïðñòóôõö??ùúûüýþÿ???????????????????????????????????????????????????????????????????????????????????????????Š??Ž?????š??ž??Œœ????øØ????????????????????????????????????????????????????????????????????????????????";


ApocalypseFont::ApocalypseFont( Framework &fw, FontType Face, std::shared_ptr<Palette> ColourPalette )
{
	int fontchars;
	int charmaxwidth;

	std::string datfile( "UFODATA/" );
	switch( Face )
	{
		case ApocalypseFont::LargeFont:
			datfile.append( "BIGFONT" );
			fontheight = 24;
			fontchars = 129;
			charmaxwidth = 14;
			spacewidth = 6;
			break;
		case ApocalypseFont::SmallFont:
			datfile.append( "SMALFONT" );
			fontheight = 15;
			fontchars = 140;
			charmaxwidth = 14;
			spacewidth = 3;
			break;
		case ApocalypseFont::TinyFont:
			datfile.append( "SMALLSET" );
			fontheight = 12;
			fontchars = 128;
			charmaxwidth = 8;
			spacewidth = 3;
			break;
	}
	std::string spcfile( datfile );
	datfile.append( ".DAT" );

	ALLEGRO_FILE* dathnd = fw.data.load_file( datfile, "rb" );

	for( int c = 0; c < fontchars; c++ )
	{
		int w = 0;
		std::shared_ptr<PaletteImage> pimg = std::make_shared<PaletteImage>(Vec2<int>{charmaxwidth, fontheight} );
		PaletteImageLock r(pimg, ImageLockUse::Write);
		for( int y = 0; y < fontheight; y++ )
		{
			for( int x = 0; x < charmaxwidth; x++ )
			{
				int palidx = al_fgetc( dathnd );
				r.set(Vec2<int>{x,y}, palidx);
				if ( palidx > 0 && x > w )
				{
					w = x;
				}
			}
		}
		this->fontbitmaps.push_back(pimg->toRGBImage(ColourPalette));
	}

	al_fclose( dathnd );
}

ApocalypseFont::~ApocalypseFont()
{
}

void ApocalypseFont::DrawString( Renderer &r, int X, int Y, std::string Text, int Alignment )
{
	int xpos = X;
	int textlen = 0;

	if( Alignment != APOCFONT_ALIGN_LEFT )
	{
		textlen = GetFontWidth( Text );
		switch( Alignment )
		{
			case APOCFONT_ALIGN_CENTRE:
				xpos -= textlen / 2;
				break;
			case APOCFONT_ALIGN_RIGHT:
				xpos -= textlen;
				break;
		}
	}

	for( unsigned int i = 0; i < Text.length(); i++ )
	{
		int charidx = FontCharacterSet.find_first_of( Text.at( i ) );
		if( charidx >= 0 && charidx < fontbitmaps.size() )
		{
			r.draw(*fontbitmaps.at(charidx), Vec2<float>{xpos,Y});
			xpos += fontwidths.at( charidx );
		} else {
			xpos += spacewidth;
		}
	}
}

int ApocalypseFont::GetFontHeight()
{
	return fontheight;
}

int ApocalypseFont::GetFontWidth( std::string Text )
{
	int textlen = 0;
	for( unsigned int i = 0; i < Text.length(); i++ )
	{
		int charidx = FontCharacterSet.find_first_of( Text.at( i ) );
		if( charidx >= 0 && charidx < fontbitmaps.size() )
		{
			textlen += fontwidths.at( charidx );
		} else {
			textlen += spacewidth;
		}
	}
	return textlen;
}

}; //namespace OpenApoc

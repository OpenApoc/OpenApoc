
#pragma once

#include "../../framework/includes.h"
#include "../../library/memory.h"


class ApocalypseFont
{

	private:
		std::vector<ALLEGRO_BITMAP*> fontbitmaps;
		Memory* spacingdata;	// TODO: Fix


	public:
		ApocalypseFont( std::string FontName );
		~ApocalypseFont();

		void DrawString( int X, int Y, std::string Text );
};

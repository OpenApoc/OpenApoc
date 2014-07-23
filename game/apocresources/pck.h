
#pragma once

#include "../../framework/includes.h"
#include "../../library/memory.h"
#include "palette.h"

typedef struct PCKCompression1ImageHeader
{
	char Reserved1;
	char Reserved2;
	int16_t LeftMostPixel;
	int16_t RightMostPixel;
	int16_t TopMostPixel;
	int16_t BottomMostPixel;
} PCKImageHeader;

typedef struct PCKCompression1RowHeader
{
	// int16_t SkipPixels; -- Read seperately to get eof record
	char ColumnToStartAt;
	char PixelsInRow;
	char Reserved1;
	char PaddingInRow;
} PCKCompression1Header;

class PCK
{

	private:
		std::vector<ALLEGRO_BITMAP*> images;
		Palette* Colours;

		void ProcessFile(std::string PckFilename, std::string TabFilename, Palette* ColourPalette, int Index);
		void LoadVersion1Format(ALLEGRO_FILE* pck, ALLEGRO_FILE* tab, int Index);
		void LoadVersion2Format(ALLEGRO_FILE* pck, ALLEGRO_FILE* tab, int Index);

	public:
		PCK( std::string PckFilename, std::string TabFilename, Palette* ColourPalette );
		PCK( std::string PckFilename, std::string TabFilename, Palette* ColourPalette, int Index );
		~PCK();

		int GetImageCount();
		void RenderImage( int Index, int X, int Y );
};

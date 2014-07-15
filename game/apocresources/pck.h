
#pragma once

#include "../../framework/includes.h"
#include "../../library/memory.h"
#include "palette.h"

typedef struct PCKImageHeader
{
	char CompressionMethod;
	char Reserved1;
	char Reserved2;
	char Reserved3;
	int16_t LeftMostPixel;
	int16_t RightMostPixel;
	int16_t TopMostPixel;
	int16_t BottomMostPixel;
} PCKImageHeader;

typedef struct PCKCompression1Header
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
		bool terrain;
		std::vector<ALLEGRO_BITMAP*> images;

	public:
		PCK( std::string PckFilename, std::string TabFilename, bool ContainsTerrain, Palette* ColourPalette );
		~PCK();

		int GetImageCount();
		void RenderImage( int Index, int X, int Y );
};

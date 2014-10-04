
#pragma once

#include "framework/includes.h"
#include "palette.h"

namespace OpenApoc {

typedef struct PCKCompression1ImageHeader
{
	uint8_t Reserved1;
	uint8_t Reserved2;
	uint16_t LeftMostPixel;
	uint16_t RightMostPixel;
	uint16_t TopMostPixel;
	uint16_t BottomMostPixel;
} PCKImageHeader;

typedef struct PCKCompression1RowHeader
{
	// int16_t SkipPixels; -- Read seperately to get eof record
	uint8_t ColumnToStartAt;
	uint8_t PixelsInRow;
	uint8_t BytesInRow;
	uint8_t PaddingInRow;
} PCKCompression1Header;

class PCK
{

	private:
		std::vector<std::shared_ptr<Image> > images;

		void ProcessFile(std::string PckFilename, std::string TabFilename, Palette &ColourPalette, int Index);
		void LoadVersion1Format(ALLEGRO_FILE* pck, ALLEGRO_FILE* tab, int Index, Palette &Colours);
		void LoadVersion2Format(ALLEGRO_FILE* pck, ALLEGRO_FILE* tab, int Index, Palette &Colours);

	public:
		PCK( std::string PckFilename, std::string TabFilename, Palette &ColourPalette );
		PCK( std::string PckFilename, std::string TabFilename, Palette &ColourPalette, int Index );
		~PCK();

		int GetImageCount();
		void RenderImage( int Index, int X, int Y );
		std::shared_ptr<Image> GetImage( int Index );
};

}; //namespace OpenApoc

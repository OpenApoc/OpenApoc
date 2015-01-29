
#pragma once

#include "framework/includes.h"

#include <allegro5/file.h>

namespace OpenApoc {

class Data;
class PaletteImage;

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
		std::vector<std::shared_ptr<PaletteImage> > images;

		void ProcessFile(Data &d, std::string PckFilename, std::string TabFilename, int Index);
		void LoadVersion1Format(ALLEGRO_FILE* pck, ALLEGRO_FILE* tab, int Index);
		void LoadVersion2Format(ALLEGRO_FILE* pck, ALLEGRO_FILE* tab, int Index);

	public:
		PCK( Data &d, std::string PckFilename, std::string TabFilename);
		PCK( Data &d, std::string PckFilename, std::string TabFilename, int Index );
		~PCK();

		int GetImageCount();
		std::shared_ptr<PaletteImage> GetImage( int Index );
};

}; //namespace OpenApoc

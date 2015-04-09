
#include "pck.h"
#include "framework/data.h"
#include "framework/image.h"
#include "framework/renderer.h"

namespace OpenApoc {

namespace  {

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

		void ProcessFile(Data &d, std::string PckFilename, std::string TabFilename, int Index);
		void LoadVersion1Format(ALLEGRO_FILE* pck, ALLEGRO_FILE* tab, int Index);
		void LoadVersion2Format(ALLEGRO_FILE* pck, ALLEGRO_FILE* tab, int Index);

	public:
		PCK( Data &d, std::string PckFilename, std::string TabFilename);
		~PCK();

		std::vector<std::shared_ptr<PaletteImage> > images;
};
PCK::PCK(Data &d, std::string PckFilename, std::string TabFilename)
{
	ProcessFile(d, PckFilename, TabFilename, -1);
}

PCK::~PCK()
{
}

void PCK::ProcessFile(Data &d, std::string PckFilename, std::string TabFilename, int Index)
{
	ALLEGRO_FILE* pck = d.load_file(PckFilename, "rb");
	ALLEGRO_FILE* tab = d.load_file(TabFilename, "rb");

	int16_t version = al_fread16le(pck);
	al_fseek(pck, 0, ALLEGRO_SEEK_SET);
	switch (version)
	{
	case 0:
		LoadVersion1Format(pck, tab, Index);
		break;
	case 1:
		LoadVersion2Format(pck, tab, Index);
		break;
	}

	al_fclose(tab);
	al_fclose(pck);
}

void PCK::LoadVersion1Format(ALLEGRO_FILE* pck, ALLEGRO_FILE* tab, int Index)
{
	std::shared_ptr<PaletteImage> img;

	int16_t c0_offset;
	int16_t c0_maxwidth;
	int16_t c0_height;
	Memory* c0_imagedata;
	size_t c0_bufferptr;
	int c0_idx;
	std::vector<int16_t> c0_rowwidths;

	int minrec = (Index < 0 ? 0 : Index);
	int maxrec = (Index < 0 ? al_fsize(tab) / 4 : Index + 1);
	for( int i = minrec; i < maxrec; i++ )
	{
		al_fseek( tab, i * 4, ALLEGRO_SEEK_SET );
		unsigned int offset = al_fread32le( tab );

		al_fseek( pck, offset, ALLEGRO_SEEK_SET );

		// Raw Data
		c0_offset = al_fread16le( pck );
		c0_imagedata = new Memory(0);
		c0_rowwidths.clear();
		c0_maxwidth = 0;
		c0_height = 0;

		while( c0_offset != -1 )
		{
			int16_t c0_width = al_fread16le( pck );	// I hope they never change width mid-image
			c0_rowwidths.push_back( c0_width );
			if( c0_maxwidth < c0_width )
			{
				c0_maxwidth = c0_width;
			}

			c0_bufferptr = c0_imagedata->GetSize();
			c0_imagedata->Resize( c0_bufferptr + c0_width + (c0_offset % 640) );
			memset( c0_imagedata->GetDataOffset( c0_bufferptr ), 0, c0_width + (c0_offset % 640) );
			al_fread( pck, c0_imagedata->GetDataOffset( c0_bufferptr + (c0_offset % 640) ), c0_width );
			c0_height++;

			c0_offset = al_fread16le( pck );	// Always a 640px row (that I've seen)
		}
		img = std::make_shared<PaletteImage>(Vec2<int>{c0_maxwidth, c0_height});
		PaletteImageLock region(img);
		c0_idx = 0;
		for( int c0_y = 0; c0_y < c0_height; c0_y++ )
		{
			for( int c0_x = 0; c0_x < c0_maxwidth; c0_x++ )
			{
				if( c0_x < c0_rowwidths.at( c0_y ) )
				{
					region.set(Vec2<int>{c0_x, c0_y}, ((char*)c0_imagedata->GetDataOffset( c0_idx ))[0] );
				} else {
					region.set(Vec2<int>{c0_x, c0_y}, 0);
				}
				c0_idx++;
			}
		}
		images.push_back( img );
		delete c0_imagedata;

	}
}

void PCK::LoadVersion2Format(ALLEGRO_FILE* pck, ALLEGRO_FILE* tab, int Index)
{
	int16_t compressionmethod;
	Memory* tmp;

	std::shared_ptr<PaletteImage> img;

	PCKCompression1ImageHeader c1_imgheader;
	uint32_t c1_pixelstoskip;
	PCKCompression1RowHeader c1_header;

	int minrec = (Index < 0 ? 0 : Index);
	int maxrec = (Index < 0 ? al_fsize(tab) / 4 : Index + 1);
	for (int i = minrec; i < maxrec; i++)
	{
		al_fseek( tab, i * 4, ALLEGRO_SEEK_SET );
		unsigned int offset = al_fread32le( tab ) * 4;

		al_fseek( pck, offset, ALLEGRO_SEEK_SET );

		compressionmethod = al_fread16le( pck );
		switch( compressionmethod )
		{
			case 0:

				break;

			case 1:
			{
				// Raw Data with RLE
				al_fread( pck, &c1_imgheader, sizeof( PCKCompression1ImageHeader ) );
				img = std::make_shared<PaletteImage>(Vec2<int>{c1_imgheader.RightMostPixel, c1_imgheader.BottomMostPixel});

				PaletteImageLock lock(img);
				c1_pixelstoskip = (uint32_t)al_fread32le( pck );
				while( c1_pixelstoskip != 0xFFFFFFFF )
				{
					al_fread( pck, &c1_header, sizeof( PCKCompression1Header ) );
					uint32_t c1_y = (c1_pixelstoskip / 640);

					if( c1_y < c1_imgheader.BottomMostPixel)
					{
						if( c1_header.BytesInRow != 0 )
						{
							// No idea what this is
							uint32_t chunk = al_fread32le( pck );

							for (uint32_t c1_x = c1_imgheader.LeftMostPixel; c1_x < c1_header.BytesInRow; c1_x++)
							{
								if (c1_x < c1_imgheader.RightMostPixel)
								{
									lock.set(Vec2<int>{c1_x, c1_y}, al_fgetc(pck));
								} else {
									// Pretend to process data
									al_fgetc( pck );
								}
							}

						} else {
							for( uint32_t c1_x = 0; c1_x < c1_header.PixelsInRow; c1_x++ )
							{
								if( (c1_header.ColumnToStartAt + c1_x - c1_imgheader.LeftMostPixel) < c1_imgheader.RightMostPixel - c1_imgheader.LeftMostPixel )
								{
									lock.set(Vec2<int>{c1_header.ColumnToStartAt + c1_x, c1_y}, al_fgetc(pck));
								} else {
									// Pretend to process data
									al_fgetc( pck );
								}
							}
						}
					}
					c1_pixelstoskip = al_fread32le( pck );
				}
				images.push_back( img );
				break;
			}

			case 2:
				// Divide by 0 should indicate we've tried to load a Mode 2 compressed image
				compressionmethod = 0;
				compressionmethod = 2 / compressionmethod;
				break;

			case 3:
				// Divide by 0 should indicate we've tried to load a Mode 3 compressed image
				compressionmethod = 0;
				compressionmethod = 2 / compressionmethod;
				break;

			case 128:
				// Divide by 0 should indicate we've tried to load a Mode 128 compressed image
				compressionmethod = 0;
				compressionmethod = 2 / compressionmethod;
				break;

			default:
				// No idea
				compressionmethod = 0;
				compressionmethod = 2 / compressionmethod;
				break;
		}

	}
}
}; //anonymous namespace

std::shared_ptr<ImageSet>
PCKLoader::load(Data &data, const std::string PckFilename, const std::string TabFilename)
{
	PCK *p = new PCK(data, PckFilename, TabFilename);
	auto imageSet = std::make_shared<ImageSet>();
	imageSet->maxSize = Vec2<int>{0,0};
	imageSet->images.resize(p->images.size());
	for (int i = 0; i < p->images.size(); i++)
	{
		imageSet->images[i] = p->images[i];
		imageSet->images[i]->owningSet = imageSet;
		imageSet->images[i]->indexInSet = i;
		if (imageSet->images[i]->size.x > imageSet->maxSize.x)
			imageSet->maxSize.x = imageSet->images[i]->size.x;
		if (imageSet->images[i]->size.y > imageSet->maxSize.y)
			imageSet->maxSize.y = imageSet->images[i]->size.y;
	}

	std::cerr << "loaded \"" << PckFilename << "\" - " << imageSet->images.size() << " images, max size {" << imageSet->maxSize.x << "," << imageSet->maxSize.y << "}\n";

	return imageSet;
}
}; //namespace OpenApoc

#include "framework/logger.h"
#include "game/apocresources/pck.h"
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
		void LoadVersion1Format(PHYSFS_file* pck, PHYSFS_file* tab, int Index);
		void LoadVersion2Format(PHYSFS_file* pck, PHYSFS_file* tab, int Index);

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
	PHYSFS_file* pck = d.load_file(PckFilename, "rb");
	PHYSFS_file* tab = d.load_file(TabFilename, "rb");

	uint16_t version;
	PHYSFS_readULE16(pck, &version);
	PHYSFS_seek(pck, 0);
	switch (version)
	{
	case 0:
		LoadVersion1Format(pck, tab, Index);
		break;
	case 1:
		LoadVersion2Format(pck, tab, Index);
		break;
	}

	PHYSFS_close(tab);
	PHYSFS_close(pck);
}

void PCK::LoadVersion1Format(PHYSFS_file* pck, PHYSFS_file* tab, int Index)
{
	std::shared_ptr<PaletteImage> img;

	uint16_t c0_offset;
	uint16_t c0_maxwidth;
	uint16_t c0_height;
	Memory* c0_imagedata;
	size_t c0_bufferptr;
	int c0_idx;
	std::vector<int16_t> c0_rowwidths;

	int minrec = (Index < 0 ? 0 : Index);
	int maxrec = (Index < 0 ? PHYSFS_fileLength(tab) / 4 : Index + 1);
	for( int i = minrec; i < maxrec; i++ )
	{
		PHYSFS_seek( tab, i * 4);
		unsigned int offset;
		PHYSFS_readULE32(tab, &offset);

		PHYSFS_seek( pck, offset);

		// Raw Data
		PHYSFS_readULE16(pck, &c0_offset);
		c0_imagedata = new Memory(0);
		c0_rowwidths.clear();
		c0_maxwidth = 0;
		c0_height = 0;

		while( c0_offset != 0xffff )
		{
			uint16_t c0_width;
			PHYSFS_readULE16(pck, &c0_width);	// I hope they never change width mid-image
			c0_rowwidths.push_back( c0_width );
			if( c0_maxwidth < c0_width )
			{
				c0_maxwidth = c0_width;
			}

			c0_bufferptr = c0_imagedata->GetSize();
			c0_imagedata->Resize( c0_bufferptr + c0_width + (c0_offset % 640) );
			memset( c0_imagedata->GetDataOffset( c0_bufferptr ), 0, c0_width + (c0_offset % 640) );
			PHYSFS_readBytes( pck, c0_imagedata->GetDataOffset( c0_bufferptr + (c0_offset % 640) ), c0_width );
			c0_height++;

			PHYSFS_readULE16(pck, &c0_offset);	// Always a 640px row (that I've seen)
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

void PCK::LoadVersion2Format(PHYSFS_file* pck, PHYSFS_file* tab, int Index)
{
	uint16_t compressionmethod;

	std::shared_ptr<PaletteImage> img;

	PCKCompression1ImageHeader c1_imgheader;
	uint32_t c1_pixelstoskip;
	PCKCompression1RowHeader c1_header;

	int minrec = (Index < 0 ? 0 : Index);
	int maxrec = (Index < 0 ? PHYSFS_fileLength(tab) / 4 : Index + 1);
	for (int i = minrec; i < maxrec; i++)
	{
		PHYSFS_seek( tab, i * 4);
		unsigned int offset;
		PHYSFS_readULE32( tab, &offset );
		offset *= 4;

		PHYSFS_seek(pck, offset);

		PHYSFS_readULE16(pck, &compressionmethod);
		switch( compressionmethod )
		{
			case 0:

				break;

			case 1:
			{
				// Raw Data with RLE
				PHYSFS_readBytes( pck, &c1_imgheader, sizeof( PCKCompression1ImageHeader ) );
				img = std::make_shared<PaletteImage>(Vec2<int>{c1_imgheader.RightMostPixel, c1_imgheader.BottomMostPixel});

				PaletteImageLock lock(img);
				PHYSFS_readULE32(pck, &c1_pixelstoskip);
				while( c1_pixelstoskip != 0xFFFFFFFF )
				{
					PHYSFS_readBytes( pck, &c1_header, sizeof( PCKCompression1Header ) );
					uint32_t c1_y = (c1_pixelstoskip / 640);

					if( c1_y < c1_imgheader.BottomMostPixel)
					{
						if( c1_header.BytesInRow != 0 )
						{
							// No idea what this is
							uint32_t chunk;
							PHYSFS_readULE32(pck, &chunk);

							for (uint32_t c1_x = c1_imgheader.LeftMostPixel; c1_x < c1_header.BytesInRow; c1_x++)
							{
								if (c1_x < c1_imgheader.RightMostPixel)
								{
									char idx;
									PHYSFS_readBytes(pck, &idx, 1);
									lock.set(Vec2<int>{c1_x, c1_y}, idx);
								} else {
									// Pretend to process data
									char dummy;
									PHYSFS_readBytes(pck, &dummy, 1);
								}
							}

						} else {
							for( uint32_t c1_x = 0; c1_x < c1_header.PixelsInRow; c1_x++ )
							{
								if( (c1_header.ColumnToStartAt + c1_x) < c1_imgheader.RightMostPixel)
								{
									char idx;
									PHYSFS_readBytes(pck, &idx, 1);
									lock.set(Vec2<int>{c1_header.ColumnToStartAt + c1_x, c1_y}, idx);
								} else {
									// Pretend to process data
									char dummy;
									PHYSFS_readBytes(pck, &dummy, 1);
								}
							}
						}
					}
					PHYSFS_readULE32(pck, &c1_pixelstoskip);
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
	for (unsigned int i = 0; i < p->images.size(); i++)
	{
		imageSet->images[i] = p->images[i];
		imageSet->images[i]->owningSet = imageSet;
		imageSet->images[i]->indexInSet = i;
		if (imageSet->images[i]->size.x > imageSet->maxSize.x)
			imageSet->maxSize.x = imageSet->images[i]->size.x;
		if (imageSet->images[i]->size.y > imageSet->maxSize.y)
			imageSet->maxSize.y = imageSet->images[i]->size.y;
	}
	delete p;

	LogInfo("Loaded \"%s\" - %u images, max size {%d,%d}", PckFilename.c_str(), (unsigned int)imageSet->images.size(), imageSet->maxSize.x, imageSet->maxSize.y);

	return imageSet;
}
}; //namespace OpenApoc

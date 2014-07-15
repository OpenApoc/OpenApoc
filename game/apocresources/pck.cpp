
#include "pck.h"

PCK::PCK( std::string PckFilename, std::string TabFilename, bool ContainsTerrain, Palette* ColourPalette )
{
	Memory* tmp;

	ALLEGRO_FILE* pck = al_fopen( PckFilename.c_str(), "rb" );
	ALLEGRO_FILE* tab = al_fopen( TabFilename.c_str(), "rb" );

	terrain = ContainsTerrain;

	PCKImageHeader imgheader;

	ALLEGRO_BITMAP* bitmap;
	ALLEGRO_LOCKED_REGION* region;

	int32_t c1_pixelstoskip;
	PCKCompression1Header c1_header;

	for( int i = 0; i < al_fsize( tab ) / 4; i++ )
	{
		unsigned int offset = al_fread32le( tab );

		offset *= 4;

		// Terrain blocks
		if( terrain )
		{
			offset *= 4;
		}

		al_fseek( pck, offset, ALLEGRO_SEEK_SET );

		// Read each "angle"
		for( int tidx = 1; tidx <= (terrain ? 4 : 1); tidx++ )
		{
			al_fread( pck, &imgheader, sizeof( PCKImageHeader ) );
			switch( imgheader.CompressionMethod )
			{
				case 0:
					// No images
					break;

				case 1:
					// No compression
					bitmap = al_create_bitmap( imgheader.RightMostPixel - imgheader.LeftMostPixel, imgheader.BottomMostPixel - imgheader.TopMostPixel );
					region = al_lock_bitmap( bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, 0 );

					c1_pixelstoskip = al_fread32le( pck );
					while( c1_pixelstoskip != -1 )
					{
						al_fread( pck, &c1_header, sizeof( PCKCompression1Header ) );
						int c1_y = (c1_pixelstoskip / 640) - imgheader.TopMostPixel;

						for( int c1_x = 0; c1_x < c1_header.PixelsInRow - c1_header.PaddingInRow; c1_x++ )
						{
							if( (c1_header.ColumnToStartAt + c1_x - imgheader.LeftMostPixel) < imgheader.RightMostPixel - imgheader.LeftMostPixel )
							{
								int dataptr = (c1_y * region->pitch) + ((c1_header.ColumnToStartAt + c1_x - imgheader.LeftMostPixel) * 4);

								Colour* c1_rowptr = (Colour*)(&((char*)region->data)[ dataptr ]);
								Colour* c1_palcol = ColourPalette->GetColour( al_fgetc( pck ) );
								c1_rowptr->a = c1_palcol->a;
								c1_rowptr->r = c1_palcol->r;
								c1_rowptr->g = c1_palcol->g;
								c1_rowptr->b = c1_palcol->b;
							} else {
								// Pretend to process data
								al_fgetc( pck );
							}
						}
						tmp = new Memory(c1_header.PaddingInRow);
						al_fread( pck, tmp->GetData(), c1_header.PaddingInRow );
						delete tmp;

						c1_pixelstoskip = al_fread32le( pck );
					}
					al_unlock_bitmap( bitmap );
					images.push_back( bitmap );
					break;

				case 2:
					// Unknown/unused
					break;

				case 3:
					// Divide by 0 should indicate we've tried to load a Mode 3 compressed image
					imgheader.Reserved2 = 0;
					imgheader.Reserved1 = 2 / imgheader.Reserved2;
					break;

				case 128:
					// Divide by 0 should indicate we've tried to load a Mode 128 compressed image
					imgheader.Reserved2 = 0;
					imgheader.Reserved1 = 2 / imgheader.Reserved2;
					break;

				default:
					// No idea
					break;
			}
		}

	}


	al_fclose( tab );
	al_fclose( pck );
}

PCK::~PCK()
{
}

int PCK::GetImageCount()
{
	return images.size();
}

void PCK::RenderImage( int Index, int X, int Y )
{
	al_draw_bitmap( images.at(Index), X, Y, 0 );
}


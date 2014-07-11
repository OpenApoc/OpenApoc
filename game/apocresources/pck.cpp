
#include "pck.h"

PCK::PCK( std::string PckFilename, std::string TabFilename, bool ContainsTerrain, Palette* ColourPalette )
{
	ALLEGRO_FILE* pck = al_fopen( PckFilename.c_str(), "rb" );
	ALLEGRO_FILE* tab = al_fopen( TabFilename.c_str(), "rb" );

	terrain = ContainsTerrain;

	PCKImageHeader imgheader;

	ALLEGRO_BITMAP* bitmap;
	ALLEGRO_LOCKED_REGION* region;
	int c1_regionlocation;
	int32_t c1_pixelstoskip;
	int c1_startcol;
	int c1_pixelsinrecord;
	int c1_reserved;
	int c1_pxcount2;

	for( int i = 0; i < al_fsize( tab ) / 4; i++ )
	{
		unsigned int offset = al_fread32le( tab );
		al_fseek( pck, offset, ALLEGRO_SEEK_SET );

		// Terrain blocks
		if( terrain )
		{
			offset *= 4;
		}

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
					bitmap = al_create_bitmap( 640, 480 );
					region = al_lock_bitmap( bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, 0 );
					c1_regionlocation = 0;

					c1_pixelstoskip = al_fread32le( pck );
					while( c1_pixelstoskip != -1 )
					{
						c1_regionlocation = ((c1_pixelstoskip / 640) * region->pitch) + (c1_pixelstoskip % 640);

						c1_startcol = al_fgetc( pck );
						c1_pixelsinrecord = al_fgetc( pck );
						c1_reserved = al_fgetc( pck );
						c1_pxcount2 = al_fgetc( pck );

						for( int c1_x = 0; c1_x < c1_pixelsinrecord; c1_x++ )
						{
							Colour* c1_rowptr = (Colour*)(&((char*)region->data)[ c1_regionlocation + (c1_x * 4) ]);
							Colour* c1_palcol = ColourPalette->GetColour( al_fgetc( pck ) );
							c1_rowptr->a = c1_palcol->a;
							c1_rowptr->r = c1_palcol->r;
							c1_rowptr->g = c1_palcol->g;
							c1_rowptr->b = c1_palcol->b;
						}

						c1_pixelstoskip = al_fread32le( pck );
					}
					al_unlock_bitmap( bitmap );
					images.push_back( bitmap );
					break;

				case 2:
					// Unknown/unused
					break;

				case 3:
					break;

				case 128:
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


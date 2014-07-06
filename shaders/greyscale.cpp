
#include "greyscale.h"
#include "../framework/framework.h"

void ShaderGreyscale::Apply( ALLEGRO_BITMAP* Target )
{
	PackedARGB8888* gsCol;

	int imgW = al_get_bitmap_width( Target );
	int imgH = al_get_bitmap_height( Target );

	ALLEGRO_LOCKED_REGION* rgn = al_lock_bitmap( Target, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE );

	for( int y = 0; y < imgH; y++ )
	{
		for( int x = 0; x < imgW; x++ )
		{
			//gsCol = &((PackedARGB8888*)rgn->data)[(y * rgn->pitch) + x];
			PackedARGB8888* pxRow = (PackedARGB8888*)((char*)rgn->data + (y * rgn->pitch));
			gsCol = &pxRow[x];
			gsCol->r = (unsigned char)(((float)gsCol->r * 0.35) + ((float)gsCol->g * 0.5) + ((float)gsCol->b * 0.15));
			gsCol->g = gsCol->r;
			gsCol->b = gsCol->r;
		}
	}

	al_unlock_bitmap( Target );

}

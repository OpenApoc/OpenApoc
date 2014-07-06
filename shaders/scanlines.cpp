
#include "scanlines.h"
#include "../framework/framework.h"

ShaderScanlines::ShaderScanlines()
{
	nonScanLineWidth = 5;
	scanLineWidth = 2;
	scanDecrease = 32;
}

ShaderScanlines::ShaderScanlines(int NonScanWidth, int ScanWidth, int ScanDecrease)
{
	nonScanLineWidth = NonScanWidth;
	scanLineWidth = ScanWidth;
	scanDecrease = ScanDecrease;
}

void ShaderScanlines::Apply( ALLEGRO_BITMAP* Target )
{
	ALLEGRO_BITMAP* orig = FRAMEWORK->Display_GetCurrentTarget();

	int imgW = al_get_bitmap_width( Target );
	int imgH = al_get_bitmap_height( Target );
	int linesForScan = nonScanLineWidth + scanLineWidth;

	FRAMEWORK->Display_SetTarget( Target );

	int y = nonScanLineWidth;
	while( y < imgH )
	{
		al_draw_line( 0, y, imgW, y, al_map_rgba( 0, 0, 0, scanDecrease ), scanLineWidth );
		y += linesForScan;
	}

	FRAMEWORK->Display_SetTarget( orig );

}

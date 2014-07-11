
#include "palette.h"

Palette::Palette( std::string Filename )
{
	unsigned char colourblock[3];

	std::string path = "data/";
	path.append( Filename );

	ALLEGRO_FILE* f = al_fopen( path.c_str(), "rb" );

	colours = (Colour*)malloc( (al_fsize( f ) / 3) * sizeof( Colour ) );

	int idx = 0;
	while( !al_feof( f ) )
	{
		al_fread( f, (void*)&colourblock, 3 );
		colours[idx].a = (idx == 0 ? 0 : 255);
		colours[idx].r = colourblock[2] << 2;
		colours[idx].g = colourblock[1] << 2;
		colours[idx].b = colourblock[0] << 2;
		idx++;
	}

	al_fclose( f );
}

Palette::~Palette()
{
	free( (void*)colours );
}

Colour* Palette::GetColour(int Index)
{
	return &colours[Index];
}

void Palette::SetColour(int Index, Colour* Col)
{
	colours[Index].a = Col->a;
	colours[Index].r = Col->r;
	colours[Index].g = Col->g;
	colours[Index].b = Col->b;
}



#include "palette.h"
#include "framework/image.h"
#include "framework/data.h"

namespace OpenApoc {

Palette::Palette( Data &d, std::string Filename )
{
	std::shared_ptr<Image> paletteimage;
	unsigned char colourblock[3];
	unsigned int idx = 0;

	paletteimage = d.load_image( Filename );
	if( !paletteimage )
	{
		ALLEGRO_FILE* f = d.load_file( Filename, "rb" );
		size_t numEntries = al_fsize(f) / 3;

		colours.reset(new Colour[numEntries]);


		while(idx < numEntries)
		{
			al_fread( f, (void*)&colourblock, 3 );
			if (al_feof(f))
			{
				break;
			}
			colours[idx].a = (idx == 0 ? 0 : 255);
			colours[idx].b = colourblock[2] << 2;
			colours[idx].g = colourblock[1] << 2;
			colours[idx].r = colourblock[0] << 2;
			idx++;
		}

		al_fclose( f );
	} else {
		auto rgbImage = std::dynamic_pointer_cast<RGBImage>(paletteimage);
		assert(rgbImage);
		colours.reset (new Colour[ paletteimage->width * paletteimage->height] );
		RGBImageLock img(rgbImage, ImageLockUse::Read);

		for( int y = 0; y < paletteimage->height; y++ )
		{
			for( int x = 0; x < paletteimage->width; x++ )
			{
				Colour c = img.get(x, y);
				colours[idx] = c;
				idx++;
			}
		}
	}
}

Palette::~Palette()
{
}

Colour &Palette::GetColour(int Index)
{
	return colours[Index];
}

void Palette::SetColour(int Index, Colour &Col)
{
	colours[Index] = Col;
}

void Palette::DumpPalette( std::string Filename )
{
	std::shared_ptr<RGBImage> img = std::make_shared<RGBImage>(16,16);
	{
		RGBImageLock lock(img);

		int c = 0;
		for( int y = 0; y < 16; y++ )
		{
			for( int x = 0; x < 16; x++ )
			{
				lock.set(x, y, GetColour(c));
				c++;
			}
		}
	}

	img->saveBitmap(Filename);
}
}; //namespace OpenApoc

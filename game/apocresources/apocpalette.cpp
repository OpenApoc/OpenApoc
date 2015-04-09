#include "apocpalette.h"
#include "framework/palette.h"
#include "framework/data.h"

namespace OpenApoc {

Palette*
loadApocPalette(Data &data, const std::string fileName)
{
	ALLEGRO_FILE *f = data.load_file(fileName, "rb");
	if (!f)
		return nullptr;
	size_t numEntries = al_fsize(f) / 3;
	Palette *p = new Palette(numEntries);
	for (int i = 0; i < numEntries; i++)
	{
		uint8_t colour[3];
		Colour c;

		al_fread(f, (void*)&colour, 3);
		if (al_feof(f))
			break;
		if (i == 0)
			c = {0,0,0,0};
		else
			c = {(uint8_t)(colour[0] << 2), (uint8_t)(colour[1] << 2), (uint8_t)(colour[2] << 2), 255};
		p->SetColour(i, c);
	}
	al_fclose(f);

	return p;
}

}; //namespace OpenApoc

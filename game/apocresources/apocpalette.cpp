#include "game/apocresources/apocpalette.h"
#include "framework/palette.h"
#include "framework/data.h"

namespace OpenApoc {

Palette*
loadApocPalette(Data &data, const UString fileName)
{
	auto f = data.load_file(fileName);
	if (!f)
		return nullptr;
	auto numEntries = f.size() / 3;
	Palette *p = new Palette(numEntries);
	for (unsigned int i = 0; i < numEntries; i++)
	{
		uint8_t colour[3];
		Colour c;

		f.read((char*)&colour, 3);
		if (!f)
			break;
		if (i == 0)
			c = {0,0,0,0};
		else
			c = {(uint8_t)(colour[0] << 2), (uint8_t)(colour[1] << 2), (uint8_t)(colour[2] << 2), 255};
		p->SetColour(i, c);
	}

	return p;
}

}; //namespace OpenApoc

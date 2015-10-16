#include "game/apocresources/apocpalette.h"
#include "framework/palette.h"
#include "framework/data.h"

namespace OpenApoc
{

Palette *loadApocPalette(Data &data, const UString fileName)
{
	auto f = data.fs.open(fileName);
	if (!f)
		return nullptr;
	auto numEntries = f.size() / 3;
	auto p = new Palette(numEntries);
	for (unsigned int i = 0; i < numEntries; i++)
	{
		uint8_t colour[3];
		Colour c;

		f.read(reinterpret_cast<char *>(&colour), 3);
		if (!f)
			break;
		if (i == 0)
			c = {0, 0, 0, 0};
		else
			c = {static_cast<uint8_t>(colour[0] << 2), static_cast<uint8_t>(colour[1] << 2),
			     static_cast<uint8_t>(colour[2] << 2), 255};
		p->SetColour(i, c);
	}

	return p;
}

}; // namespace OpenApoc

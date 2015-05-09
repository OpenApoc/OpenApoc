#include "apocpalette.h"
#include "framework/palette.h"
#include "framework/data.h"

namespace OpenApoc {

Palette*
loadApocPalette(Data &data, const std::string fileName)
{
	PHYSFS_file *f = data.load_file(fileName, "rb");
	if (!f)
		return nullptr;
	size_t numEntries = PHYSFS_fileLength(f) / 3;
	Palette *p = new Palette(numEntries);
	for (unsigned int i = 0; i < numEntries; i++)
	{
		uint8_t colour[3];
		Colour c;

		PHYSFS_readBytes(f, (void*)&colour, 3);
		if (PHYSFS_eof(f))
			break;
		if (i == 0)
			c = {0,0,0,0};
		else
			c = {(uint8_t)(colour[0] << 2), (uint8_t)(colour[1] << 2), (uint8_t)(colour[2] << 2), 255};
		p->SetColour(i, c);
	}
	PHYSFS_close(f);

	return p;
}

}; //namespace OpenApoc

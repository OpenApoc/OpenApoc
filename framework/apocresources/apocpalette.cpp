#include "framework/apocresources/apocpalette.h"
#include "framework/data.h"
#include "framework/logger.h"
#include "framework/palette.h"

namespace OpenApoc
{

sp<Palette> loadApocPalette(Data &data, const UString fileName)
{
	auto f = data.fs.open(fileName);
	if (!f)
		return nullptr;
	auto numEntries = (unsigned)f.size() / 3;
	auto p = mksp<Palette>(numEntries);
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
		{
			/* Scale from 0-63 to 0-255 */
			uint8_t r = static_cast<uint8_t>(colour[0] << 2 | (colour[0] >> 4 & 0x3));
			uint8_t g = static_cast<uint8_t>(colour[1] << 2 | (colour[1] >> 4 & 0x3));
			uint8_t b = static_cast<uint8_t>(colour[2] << 2 | (colour[2] >> 4 & 0x3));
			uint8_t a = 255;
			c = {r, g, b, a};
		}
		p->setColour(i, c);
	}

	return p;
}

struct PcxHeader
{
	uint8_t Identifier;      /* PCX Id Number (Always 0x0A) */
	uint8_t Version;         /* Version Number */
	uint8_t Encoding;        /* Encoding Format */
	uint8_t BitsPerPixel;    /* Bits per Pixel */
	uint16_t XStart;         /* Left of image */
	uint16_t YStart;         /* Top of Image */
	uint16_t XEnd;           /* Right of Image */
	uint16_t YEnd;           /* Bottom of image */
	uint16_t HorzRes;        /* Horizontal Resolution */
	uint16_t VertRes;        /* Vertical Resolution */
	uint8_t Palette[48];     /* 16-Color EGA Palette */
	uint8_t Reserved1;       /* Reserved (Always 0) */
	uint8_t NumBitPlanes;    /* Number of Bit Planes */
	uint16_t BytesPerLine;   /* Bytes per Scan-line */
	uint16_t PaletteType;    /* Palette Type */
	uint16_t HorzScreenSize; /* Horizontal Screen Size */
	uint16_t VertScreenSize; /* Vertical Screen Size */
	uint8_t Reserved2[54];   /* Reserved (Always 0) */
};

static const uint8_t PcxIdentifier = 0x0A;

static_assert(sizeof(struct PcxHeader) == 128, "PcxHeader unexpected size");

sp<Palette> loadPCXPalette(Data &data, const UString fileName)
{
	auto length = fileName.length();
	if (length < 4 || to_upper(fileName.substr(length - 4, 4)) != ".PCX")
	{
		LogInfo("Skipping file \"%s\" as it doesn't look like a .pcx", fileName);
		return nullptr;
	}

	auto file = data.fs.open(fileName);
	if (!file)
	{
		LogInfo("File \"%s\" failed to be opened", fileName);
		return nullptr;
	}

	// We only support images with 256-colour palettes, so even if it has a pcx header supported
	// files will never be smaller than sizeof(header) + sizeof(palette)
	if (file.size() < sizeof(PcxHeader) + 256 * 3)
	{
		LogInfo("File \"%s\" has size %zu - too small for header and palette", fileName,
		        file.size());
		return nullptr;
	}

	PcxHeader header;

	file.read(reinterpret_cast<char *>(&header), sizeof(header));
	if (!file)
	{
		LogInfo("File \"%s\" failed to read PCX header", fileName);
		return nullptr;
	}

	if (header.Identifier != PcxIdentifier)
	{
		LogInfo("File \"%s\" doesn't have PCX header magic", fileName);
		return nullptr;
	}

	if (header.BitsPerPixel != 8)
	{
		LogInfo("File \"%s\" has non-8-bit image", fileName);
		return nullptr;
	}

	file.seekg(file.size() - (256 * 3));

	auto p = mksp<Palette>(256);

	for (unsigned int i = 0; i < 256; i++)
	{
		uint8_t colour[3];
		Colour c;

		file.read(reinterpret_cast<char *>(&colour), 3);
		if (!file)
		{
			LogWarning("Unexpected EOF at index %u", i);
			return nullptr;
		}
		if (i == 0)
			c = {0, 0, 0, 0};
		else
			c = {static_cast<uint8_t>(colour[0]), static_cast<uint8_t>(colour[1]),
			     static_cast<uint8_t>(colour[2]), 255};
		p->setColour(i, c);
	}

	return p;
}

}; // namespace OpenApoc

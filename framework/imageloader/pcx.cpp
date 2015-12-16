#include "library/sp.h"
#include "framework/imageloader_interface.h"
#include "framework/logger.h"
#include "framework/palette.h"
#include "library/vec.h"

using namespace OpenApoc;

namespace
{

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

class PCXImageLoader : public OpenApoc::ImageLoader
{
  public:
	PCXImageLoader() {}
	virtual ~PCXImageLoader() {}

	virtual sp<OpenApoc::Image> loadImage(IFile &file) override
	{
		auto size = file.size();
		auto path = file.systemPath();
		auto data = file.readAll();
		if (size < sizeof(struct PcxHeader) + 256 * 3)
		{
			LogWarning("File \"%s\" size %u too small for PCX header + palette\n", path.c_str(),
			           (unsigned)size);
			return nullptr;
		}
		struct PcxHeader *header = (struct PcxHeader *)&data[0];

		if (header->Identifier != PcxIdentifier)
		{
			LogWarning("File \"%s\" had invalid PCX identifier 0x%02x", path.c_str(),
			           (unsigned)header->Identifier);
			return nullptr;
		}

		if (header->Encoding != 1)
		{
			LogWarning("File \"%s\" had invalid PCX Encoding 0x%02x", path.c_str(),
			           (unsigned)header->Encoding);
			return nullptr;
		}

		if (header->BitsPerPixel != 8)
		{
			LogWarning("File \"%s\" had invalid PCX BitsPerPixel 0x%02x", path.c_str(),
			           (unsigned)header->BitsPerPixel);
			return nullptr;
		}

		if (header->NumBitPlanes != 1)
		{
			LogWarning("File \"%s\" had invalid PCX NumBitPlanes 0x%02x", path.c_str(),
			           (unsigned)header->NumBitPlanes);
			return nullptr;
		}

		uint8_t b = data[size - (256 * 3) - 1];
		if (b != 0x0C)
		{
			LogWarning("File \"%s\" had invalid PCX palette identifier byte 0x%02x", path.c_str(),
			           (unsigned)b);
			return nullptr;
		}

		auto p = std::make_shared<Palette>(256);

		const uint8_t *palette_data = (uint8_t *)&data[size - (256 * 3)];
		for (unsigned i = 0; i < 256; i++)
		{
			uint8_t r = *palette_data++;
			uint8_t g = *palette_data++;
			uint8_t b = *palette_data++;
			Colour c{r, g, b, 255};
			p->SetColour(i, c);
		}

		auto sizeX = (header->XEnd - header->XStart + 1);
		auto sizeY = (header->YEnd - header->YStart + 1);

		auto pimg = std::make_shared<PaletteImage>(Vec2<unsigned int>{sizeX, sizeY});

		{
			PaletteImageLock lock(pimg);

			const uint8_t *img_data = (uint8_t *)&data[sizeof(struct PcxHeader)];

			for (unsigned y = header->YStart; y <= header->YEnd; y++)
			{
				unsigned x = header->XStart;
				while (x <= header->XEnd)
				{
					uint8_t b;
					int run_length;
					uint8_t idx;
					if (img_data >= (uint8_t *)&data[size])
					{
						LogWarning("Unexpected EOF reading PCX file \"%s\" ", path.c_str());
						return nullptr;
					}
					b = *img_data++;
					// If the two top bits are set it's a run of the bottom 6 bits
					if ((b & 0xC0) == 0xC0)
					{
						run_length = b & 0x3F;
						if (img_data >= (uint8_t *)&data[size])
						{
							LogWarning("Unexpected EOF reading PCX file \"%s\" ", path.c_str());
							return nullptr;
						}
						idx = *img_data++;
					}
					// Otherwise it's just a single index (I suppose this means you
					// can't use palette indices above 0xC0 outside a run?)
					else
					{
						run_length = 1;
						idx = b;
					}
					while (run_length > 0 && x <= header->XEnd)
					{
						lock.set({x, y}, idx);
						x++;
						run_length--;
					}
				}
			}
		}

		return pimg->toRGBImage(p);
	}

	virtual UString getName() override { return "PCX"; }
};

class PCXImageLoaderFactory : public OpenApoc::ImageLoaderFactory
{
  public:
	virtual OpenApoc::ImageLoader *create() override { return new PCXImageLoader(); }
	virtual ~PCXImageLoaderFactory() {}
};

OpenApoc::ImageLoaderRegister<PCXImageLoaderFactory> register_at_load_pcx_image("pcx");
} // anonymous namespace

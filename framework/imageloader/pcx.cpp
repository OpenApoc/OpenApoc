#include "framework/fs.h"
#include "framework/image.h"
#include "framework/imageloader_interface.h"
#include "framework/logger.h"
#include "framework/palette.h"
#include "library/sp.h"
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

const uint8_t PcxIdentifier = 0x0A;

static_assert(sizeof(struct PcxHeader) == 128, "PcxHeader unexpected size");

class PCXImageLoader : public OpenApoc::ImageLoader
{
  public:
	PCXImageLoader() {}
	~PCXImageLoader() override = default;

	sp<OpenApoc::Image> loadImage(IFile &file) override
	{
		auto size = file.size();
		auto path = file.systemPath();
		auto data = file.readAll();
		if (size < sizeof(struct PcxHeader) + 256 * 3)
		{
			LogInfo("File \"{}\" size {} too small for PCX header + palette\n", path,
			        static_cast<unsigned>(size));
			return nullptr;
		}
		struct PcxHeader *header = reinterpret_cast<struct PcxHeader *>(&data[0]);

		if (header->Identifier != PcxIdentifier)
		{
			LogInfo("File \"{}\" had invalid PCX identifier 0x{:02x}", path,
			        static_cast<unsigned>(header->Identifier));
			return nullptr;
		}

		if (header->Encoding != 1)
		{
			LogWarning("File \"{}\" had invalid PCX Encoding 0x{:02x}", path,
			           static_cast<unsigned>(header->Encoding));
			return nullptr;
		}

		if (header->BitsPerPixel != 8)
		{
			LogWarning("File \"{}\" had invalid PCX BitsPerPixel 0x{:02x}", path,
			           static_cast<unsigned>(header->BitsPerPixel));
			return nullptr;
		}

		if (header->NumBitPlanes != 1)
		{
			LogWarning("File \"{}\" had invalid PCX NumBitPlanes 0x{:02x}", path,
			           static_cast<unsigned>(header->NumBitPlanes));
			return nullptr;
		}

		uint8_t pal = data[size - (256 * 3) - 1];
		if (pal != 0x0C)
		{
			LogWarning("File \"{}\" had invalid PCX palette identifier byte 0x{:02x}", path,
			           static_cast<unsigned>(pal));
			return nullptr;
		}

		auto p = mksp<Palette>(256);

		const uint8_t *palette_data = reinterpret_cast<uint8_t *>(&data[size - (256 * 3)]);
		for (unsigned i = 0; i < 256; i++)
		{
			uint8_t r = *palette_data++;
			uint8_t g = *palette_data++;
			uint8_t b = *palette_data++;
			Colour c{r, g, b, 255};
			p->setColour(i, c);
		}

		auto sizeX = (header->XEnd - header->XStart + 1);
		auto sizeY = (header->YEnd - header->YStart + 1);

		auto img = mksp<RGBImage>(Vec2<unsigned int>{sizeX, sizeY});

		{
			RGBImageLock lock(img);

			const uint8_t *img_data = reinterpret_cast<uint8_t *>(&data[sizeof(struct PcxHeader)]);

			for (unsigned y = header->YStart; y <= header->YEnd; y++)
			{
				unsigned x = header->XStart;
				while (x <= header->XEnd)
				{
					uint8_t b;
					int run_length;
					uint8_t idx;
					if (img_data >= reinterpret_cast<uint8_t *>(&data[size]))
					{
						LogWarning("Unexpected EOF reading PCX file \"{}\" ", path);
						return nullptr;
					}
					b = *img_data++;
					// If the two top bits are set it's a run of the bottom 6 bits
					if ((b & 0xC0) == 0xC0)
					{
						run_length = b & 0x3F;
						if (img_data >= reinterpret_cast<uint8_t *>(&data[size]))
						{
							LogWarning("Unexpected EOF reading PCX file \"{}\" ", path);
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
						lock.set({x, y}, p->getColour(idx));
						x++;
						run_length--;
					}
				}
			}
		}

		return img;
	}

	UString getName() override { return "PCX"; }
};

class PCXImageLoaderFactory : public OpenApoc::ImageLoaderFactory
{
  public:
	OpenApoc::ImageLoader *create() override { return new PCXImageLoader(); }
	~PCXImageLoaderFactory() override = default;
};

} // anonymous namespace

namespace OpenApoc
{
ImageLoaderFactory *getPCXImageLoaderFactory() { return new PCXImageLoaderFactory(); }
} // namespace OpenApoc

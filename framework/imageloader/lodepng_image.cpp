//
// Created by Alexey on 20.11.2015.
//

#include "framework/imageloader/lodepng.h"
#include "framework/apocresources/apocpalette.h"
#include "framework/data.h"
#include "framework/imageloader_interface.h"
#include "framework/logger.h"
#include "framework/palette.h"
#include "library/sp.h"
#include "library/vec.h"
#include <vector>

using namespace OpenApoc;

sp<Palette> OpenApoc::loadPNGPalette(Data &d, const UString fileName)
{
	auto f = d.fs.open(fileName);
	if (!f)
	{
		LogInfo("Failed to open file \"%s\" - skipping", fileName.c_str());
		return nullptr;
	}
	auto data = f.readAll();
	auto dataSize = f.size();
	unsigned int width, height;
	lodepng::State png_state;
	auto err = lodepng_inspect(&width, &height, &png_state,
	                           reinterpret_cast<unsigned char *>(data.get()), dataSize);
	if (err)
	{
		LogInfo("Failed to read PNG headers from \"%s\" (%u) : %s - skipping",
		        f.systemPath().c_str(), err, lodepng_error_text(err));
		return nullptr;
	}
	// 2 possible types of PNG palette - either a PALETTE section, or a 256-pixel long image
	if (png_state.info_png.color.colortype == LCT_PALETTE)
	{
		if (png_state.info_png.color.bitdepth != 8)
		{
			LogWarning("PNG \"%s\" has unsupported palette bit depth %u (expected '8')",
			           f.systemPath().c_str(), png_state.info_png.color.bitdepth);
			return nullptr;
		}
		if (png_state.info_png.color.palettesize != 256)
		{
			LogWarning("PNG \"%s\" has unsupported palette size %u (expected '256')",
			           f.systemPath().c_str(), png_state.info_png.color.palettesize);
			return nullptr;
		}
		auto pal = mksp<Palette>();
		// The lodepng palette is already in R G B A byte order and is u8
		uint8_t *palPos = png_state.info_png.color.palette;
		for (unsigned int i = 0; i < 256; i++)
		{
			pal->SetColour(i, Colour{*palPos++, *palPos++, *palPos++, *palPos++});
		}
		return pal;
	}
	else
	{
		if (width * height != 256)
		{

			LogWarning("PNG \"%s\" size {%u,%u} too large for palette (must be 256 pixels total)",
			           f.systemPath().c_str(), width, height);
			return nullptr;
		}
		std::vector<unsigned char> pixels;
		auto error = lodepng::decode(pixels, width, height,
		                             reinterpret_cast<unsigned char *>(data.get()), dataSize);
		if (error)
		{
			LogInfo("Failed to read PNG \"%s\" (%u) : %s", f.systemPath().c_str(), err,
			        lodepng_error_text(err));
			return nullptr;
		}
		auto pal = mksp<Palette>();
		// The lodepng default data output is already in R G B A byte order and is u8
		uint8_t *palPos = pixels.data();
		for (unsigned int i = 0; i < 256; i++)
		{
			uint8_t r = *palPos++;
			uint8_t g = *palPos++;
			uint8_t b = *palPos++;
			uint8_t a = *palPos++;
			pal->SetColour(i, Colour{r, g, b, a});
		}
		return pal;
	}
}

namespace
{

class LodepngImageLoader : public OpenApoc::ImageLoader
{
  public:
	LodepngImageLoader()
	{
		// empty?
	}

	sp<Image> loadImage(IFile &file) override
	{
		auto data = file.readAll();
		auto dataSize = file.size();
		unsigned int width, height;
		lodepng::State png_state;

		unsigned int err = lodepng_inspect(&width, &height, &png_state,
		                                   reinterpret_cast<unsigned char *>(data.get()), dataSize);
		if (err)
		{
			LogInfo("Failed to read PNG headers from \"%s\" (%u) : %s", file.systemPath().c_str(),
			        err, lodepng_error_text(err));
			return nullptr;
		}

		LogInfo("Loading PNG \"%s\" size {%u,%d} - colour mode %d depth %u",
		        file.systemPath().c_str(), width, height, png_state.info_png.color.colortype,
		        png_state.info_png.color.bitdepth);

		if (png_state.info_png.color.colortype == LCT_PALETTE)
		{
			if (png_state.info_png.color.bitdepth != 8)
			{
				LogWarning("PNG \"%s\" has unsupported palette bit depth %u (expected '8')",
				           file.systemPath().c_str(), png_state.info_png.color.bitdepth);
				return nullptr;
			}
			std::vector<unsigned char> pixels;
			err = lodepng::decode(pixels, width, height,
			                      reinterpret_cast<unsigned char *>(data.get()), dataSize,
			                      LCT_PALETTE, 8);
			if (err)
			{
				LogInfo("Failed to read PNG \"%s\" (%u) : %s", file.systemPath().c_str(), err,
				        lodepng_error_text(err));
				return nullptr;
			}
			if (pixels.size() < width * height)
			{
				LogWarning("PNG \"%s\" has insufficient size %u for {%u,%u} image",
				           file.systemPath().c_str(), pixels.size(), width, height);
				return nullptr;
			}
			auto img = mksp<PaletteImage>(Vec2<unsigned int>{width, height});
			PaletteImageLock dst(img, ImageLockUse::Write);
			uint8_t *readPos = pixels.data();

			for (unsigned y = 0; y < height; y++)
			{
				for (unsigned x = 0; x < width; x++)
				{
					dst.set({x, y}, *readPos);
					readPos++;
				}
			}

			return img;
		}

		// Otherwise just convert to RGBA

		std::vector<unsigned char> image;
		unsigned int error = lodepng::decode(
		    image, width, height, reinterpret_cast<unsigned char *>(data.get()), file.size());
		if (error)
		{
			LogInfo("LodePNG error code: %d: %s", error, lodepng_error_text(error));
		}
		if (!image.size())
		{
			LogInfo("Failed to load image %s (not a PNG?)", file.systemPath().c_str());
			return nullptr;
		}
		OpenApoc::Vec2<int> size(width, height);
		auto img = mksp<OpenApoc::RGBImage>(size);
		OpenApoc::RGBImageLock dst(img, OpenApoc::ImageLockUse::Write);

		for (int y = 0; y < size.y; y++)
		{
			for (int x = 0; x < size.x; x++)
			{
				OpenApoc::Colour c(
				    image[4 * size.x * y + 4 * x + 0], image[4 * size.x * y + 4 * x + 1],
				    image[4 * size.x * y + 4 * x + 2], image[4 * size.x * y + 4 * x + 3]);
				dst.set(OpenApoc::Vec2<int>(x, y), c);
			}
		}

		return img;
	}

	UString getName() override { return "lodepng"; }
};

class LodepngImageLoaderFactory : public OpenApoc::ImageLoaderFactory
{
  public:
	OpenApoc::ImageLoader *create() override { return new LodepngImageLoader(); }
	virtual ~LodepngImageLoaderFactory() {}
};

class LodepngImageWriter : public OpenApoc::ImageWriter
{
  public:
	LodepngImageWriter() {}

	bool writeImage(sp<PaletteImage> img, std::ostream &outStream, sp<Palette> pal) override
	{
		if (pal->colours.size() != 256)
		{
			LogWarning("Only 256 colour palettes supported (got %u)",
			           (unsigned)pal->colours.size());
			return false;
		}
		lodepng::State state;

		state.info_raw.colortype = LCT_PALETTE;
		state.info_raw.bitdepth = 8;

		lodepng_palette_clear(&state.info_raw);

		for (unsigned i = 0; i < 256; i++)
		{
			auto &c = pal->GetColour(i);
			auto err = lodepng_palette_add(&state.info_raw, c.r, c.g, c.b, c.a);
			if (err)
			{
				LogWarning("Failed to add palette index %u to PNG: %d: %s", i, err,
				           lodepng_error_text(err));
				return false;
			}
		}

		std::vector<unsigned char> outBuf;
		PaletteImageLock l(img, ImageLockUse::Read);
		auto err = lodepng::encode(outBuf, reinterpret_cast<unsigned char *>(l.getData()),
		                           img->size.x, img->size.y, state);
		if (err)
		{
			LogWarning("Failed to encode PNG: %d: %s", err, lodepng_error_text(err));
			return false;
		}
		outStream.write(reinterpret_cast<char *>(outBuf.data()), outBuf.size());
		if (!outStream)
		{
			LogWarning("Failed to write %u bytes to stream", outBuf.size());
			return false;
		}

		LogInfo("Successfully wrote palette PNG image");

		return true;
	}
	bool writeImage(sp<RGBImage> img, std::ostream &outStream) override
	{
		std::vector<unsigned char> outBuf;
		// We already keep RGBImages in a tightly packed R G B A byte array
		RGBImageLock l(img, ImageLockUse::Read);
		auto err = lodepng::encode(outBuf, reinterpret_cast<unsigned char *>(l.getData()),
		                           img->size.x, img->size.y);
		if (err)
		{
			LogWarning("Failed to encode PNG: %d: %s", err, lodepng_error_text(err));
			return false;
		}
		outStream.write(reinterpret_cast<char *>(outBuf.data()), outBuf.size());
		if (!outStream)
		{
			LogWarning("Failed to write %u bytes to stream", outBuf.size());
			return false;
		}
		LogInfo("Successfully wrote RGB PNG image");
		return true;
	}

	UString getName() override { return "lodepng"; }
};

class LodepngImageWriterFactory : public OpenApoc::ImageWriterFactory
{
  public:
	OpenApoc::ImageWriter *create() override { return new LodepngImageWriter(); }
	virtual ~LodepngImageWriterFactory() {}
};

} // anonymous namespace

namespace OpenApoc
{
ImageWriterFactory *getLodePNGImageWriterFactory() { return new LodepngImageWriterFactory(); }
ImageLoaderFactory *getLodePNGImageLoaderFactory() { return new LodepngImageLoaderFactory(); }
} // namespace OpenApoc

//
// Created by Alexey on 20.11.2015.
//

#include "library/sp.h"
#include "framework/imageloader_interface.h"
#include "framework/logger.h"
#include "library/vec.h"

#include <vector>
#include <physfs.h>
#include "framework/imageloader/lodepng.h"

using namespace OpenApoc;

namespace
{

class LodepngImageLoader : public OpenApoc::ImageLoader
{
  private:
  public:
	LodepngImageLoader()
	{
		// empty?
	}

	virtual sp<OpenApoc::Image> loadImage(IFile &file) override
	{
		auto data = file.readAll();
		std::vector<unsigned char> image;
		unsigned int width, height;
		unsigned int error =
		    lodepng::decode(image, width, height, (unsigned char *)data.get(), file.size());
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
		auto img = std::make_shared<OpenApoc::RGBImage>(size);
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

	virtual UString getName() override { return "lodepng"; }
};

class LodepngImageLoaderFactory : public OpenApoc::ImageLoaderFactory
{
  public:
	virtual OpenApoc::ImageLoader *create() override { return new LodepngImageLoader(); }
	virtual ~LodepngImageLoaderFactory() {}
};

OpenApoc::ImageLoaderRegister<LodepngImageLoaderFactory> register_at_load_lodepng_image("lodepng");
}

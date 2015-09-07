#include "framework/imageloader_interface.h"
#include "framework/logger.h"
#include "library/vec.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_physfs.h>

using namespace OpenApoc;

namespace
{

class AllegroImageLoader : public OpenApoc::ImageLoader
{
  public:
	AllegroImageLoader()
	{
		al_init_image_addon();
		al_set_physfs_file_interface();
	}
	virtual ~AllegroImageLoader() { al_shutdown_image_addon(); }

	virtual std::shared_ptr<OpenApoc::Image> loadImage(UString path) override
	{
		ALLEGRO_BITMAP *bmp = al_load_bitmap(path.str().c_str());
		if (!bmp) {
			LogInfo("Failed to read image %s", path.str().c_str());
			return nullptr;
		}

		OpenApoc::Vec2<int> size{al_get_bitmap_width(bmp), al_get_bitmap_height(bmp)};
		auto img = std::make_shared<OpenApoc::RGBImage>(size);
		OpenApoc::RGBImageLock dst{img, OpenApoc::ImageLockUse::Write};

		ALLEGRO_LOCKED_REGION *src =
		    al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READONLY);

		char *srcLinePtr = (char *)src->data;

		for (int y = 0; y < size.y; y++) {
			OpenApoc::Colour *c = (OpenApoc::Colour *)srcLinePtr;
			for (int x = 0; x < size.x; x++) {
				dst.set(OpenApoc::Vec2<int>{x, y}, c[x]);
			}
			srcLinePtr += src->pitch;
		}

		al_unlock_bitmap(bmp);
		al_destroy_bitmap(bmp);
		return img;
	}

	virtual UString getName() override { return "allegro"; }
};

class AllegroImageLoaderFactory : public OpenApoc::ImageLoaderFactory
{
  public:
	virtual OpenApoc::ImageLoader *create() override { return new AllegroImageLoader(); }
	virtual ~AllegroImageLoaderFactory() {}
};

OpenApoc::ImageLoaderRegister<AllegroImageLoaderFactory> register_at_load_allegro_image("allegro");

} // namespace

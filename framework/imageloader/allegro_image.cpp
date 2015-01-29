#include "framework/image.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>

namespace {

class AllegroImageLoader : public OpenApoc::ImageLoader
{
public:
	AllegroImageLoader()
	{
		
	}
	virtual ~AllegroImageLoader()
	{

	}

	virtual OpenApoc::Image* loadImage(std::string path)
	{

	}
};

}; //anonymous namespace

namespace OpenApoc{

ImageLoader*
createImageLoader()
{
	return new AllegroImageLoader();
}

}; //namespace OpenApoc

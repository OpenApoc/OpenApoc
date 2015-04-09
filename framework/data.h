#pragma once

#include "includes.h"
#include "image.h"

#include <allegro5/file.h>
#include <queue>

namespace OpenApoc {

class Data
{

	private:
		std::string root;
		const char DIR_SEP;

		std::map<std::string, std::weak_ptr<Image> >imageCache;
		std::map<std::string, std::weak_ptr<ImageSet> >imageSetCache;
		std::unique_ptr<ImageLoader> imageLoader;

		//Pin open 'imageCacheSize' images
		std::queue<std::shared_ptr<Image> > pinnedImages;
		//Pin open 'imageSetCacheSize' image sets
		std::queue<std::shared_ptr<ImageSet> > pinnedImageSets;

	public:
		Data(const std::string root, int imageCacheSize = 1, int imageSetCacheSize = 1);
		~Data();

		std::shared_ptr<Image> load_image(const std::string path);
		std::shared_ptr<ImageSet> load_image_set(const std::string path);
		std::shared_ptr<Palette> load_palette(const std::string path);
		ALLEGRO_FILE* load_file(const std::string path, const char *mode);

		std::string GetActualFilename(std::string Filename);
};

}; //namspace OpenApoc

#pragma once

#include "includes.h"
#include "image.h"

#include <physfs.h>
#include <queue>
#include <vector>
#include <string>

namespace OpenApoc {

#define PROGRAM_NAME "OpenApoc"
#define PROGRAM_ORGANISATION "OpenApoc"

class ImageLoader;

class Data
{

	private:
		std::string writeDir;
		std::map<std::string, std::weak_ptr<Image> >imageCache;
		std::map<std::string, std::weak_ptr<ImageSet> >imageSetCache;

		//Pin open 'imageCacheSize' images
		std::queue<std::shared_ptr<Image> > pinnedImages;
		//Pin open 'imageSetCacheSize' image sets
		std::queue<std::shared_ptr<ImageSet> > pinnedImageSets;
		std::list<std::unique_ptr<ImageLoader>> imageLoaders;

	public:
		Data(std::vector<std::string> paths, int imageCacheSize = 1, int imageSetCacheSize = 1);
		~Data();

		std::shared_ptr<Image> load_image(const std::string path);
		std::shared_ptr<ImageSet> load_image_set(const std::string path);
		std::shared_ptr<Palette> load_palette(const std::string path);
		PHYSFS_file* load_file(const std::string path, const char *mode);
		std::string GetActualFilename(std::string Filename);
		std::string GetCorrectCaseFilename(std::string Filename);
};

}; //namspace OpenApoc

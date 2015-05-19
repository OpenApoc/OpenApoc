#pragma once

#include "includes.h"
#include "image.h"
#include "sound.h"

#include <physfs.h>
#include <queue>
#include <vector>

namespace OpenApoc {

#define PROGRAM_NAME "OpenApoc"
#define PROGRAM_ORGANISATION "OpenApoc"

class ImageLoader;
class SampleLoader;
class MusicLoader;
class Framework;

class Data
{

	private:
		UString writeDir;
		std::map<UString, std::weak_ptr<Image> >imageCache;
		std::map<UString, std::weak_ptr<ImageSet> >imageSetCache;

		std::map<UString, std::weak_ptr<Sample> >sampleCache;
		std::map<UString, std::weak_ptr<MusicTrack> >musicCache;

		//Pin open 'imageCacheSize' images
		std::queue<std::shared_ptr<Image> > pinnedImages;
		//Pin open 'imageSetCacheSize' image sets
		std::queue<std::shared_ptr<ImageSet> > pinnedImageSets;
		std::list<std::unique_ptr<ImageLoader>> imageLoaders;
		std::list<std::unique_ptr<SampleLoader>> sampleLoaders;
		std::list<std::unique_ptr<MusicLoader>> musicLoaders;

	public:
		Data(Framework &fw, std::vector<UString> paths, int imageCacheSize = 1, int imageSetCacheSize = 1);
		~Data();

		enum class FileMode
		{
			Read,
			Write,
			ReadWrite,
		};

		std::shared_ptr<Sample> load_sample(UString path);
		std::shared_ptr<MusicTrack> load_music(UString path);
		std::shared_ptr<Image> load_image(UString path);
		std::shared_ptr<ImageSet> load_image_set(UString path);
		std::shared_ptr<Palette> load_palette(UString path);
		PHYSFS_file* load_file(UString path, FileMode mode);
		UString GetActualFilename(UString Filename);
		UString GetCorrectCaseFilename(UString Filename);

};

}; //namspace OpenApoc

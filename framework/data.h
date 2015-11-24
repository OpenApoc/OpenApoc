#pragma once
#include "library/sp.h"

#include "framework/includes.h"
#include "framework/image.h"
#include "framework/sound.h"
#include "framework/fs.h"

#include <memory>
#include <map>
#include <queue>
#include <list>
#include <vector>

namespace OpenApoc
{
class ImageLoader;
class SampleLoader;
class MusicLoader;
class Framework;
class VoxelSlice;
class LOFTemps;
class ResourceAliases;

class Data
{

  private:
	std::map<UString, std::weak_ptr<Image>> imageCache;
	std::map<UString, std::weak_ptr<ImageSet>> imageSetCache;

	std::map<UString, std::weak_ptr<Sample>> sampleCache;
	std::map<UString, std::weak_ptr<MusicTrack>> musicCache;
	std::map<UString, std::weak_ptr<LOFTemps>> LOFVoxelCache;

	// Pin open 'imageCacheSize' images
	std::queue<sp<Image>> pinnedImages;
	// Pin open 'imageSetCacheSize' image sets
	std::queue<sp<ImageSet>> pinnedImageSets;
	std::queue<sp<LOFTemps>> pinnedLOFVoxels;
	std::list<std::unique_ptr<ImageLoader>> imageLoaders;
	std::list<std::unique_ptr<SampleLoader>> sampleLoaders;
	std::list<std::unique_ptr<MusicLoader>> musicLoaders;

  public:
	std::weak_ptr<ResourceAliases> aliases;
	FileSystem fs;

	Data(std::vector<UString> paths, int imageCacheSize = 100, int imageSetCacheSize = 10,
	     int voxelCacheSize = 1);
	~Data();

	sp<Sample> load_sample(UString path);
	sp<MusicTrack> load_music(const UString &path);
	sp<Image> load_image(const UString &path);
	sp<ImageSet> load_image_set(const UString &path);
	sp<Palette> load_palette(const UString &path);
	sp<VoxelSlice> load_voxel_slice(const UString &path);
};

} // namespace OpenApoc

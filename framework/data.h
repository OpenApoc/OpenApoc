#pragma once

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
class Data
{

  private:
	std::map<UString, std::weak_ptr<Image>> imageCache;
	std::map<UString, std::weak_ptr<ImageSet>> imageSetCache;

	std::map<UString, std::weak_ptr<Sample>> sampleCache;
	std::map<UString, std::weak_ptr<MusicTrack>> musicCache;
	std::map<UString, std::weak_ptr<LOFTemps>> LOFVoxelCache;

	// Pin open 'imageCacheSize' images
	std::queue<std::shared_ptr<Image>> pinnedImages;
	// Pin open 'imageSetCacheSize' image sets
	std::queue<std::shared_ptr<ImageSet>> pinnedImageSets;
	std::queue<std::shared_ptr<LOFTemps>> pinnedLOFVoxels;
	std::list<std::unique_ptr<ImageLoader>> imageLoaders;
	std::list<std::unique_ptr<SampleLoader>> sampleLoaders;
	std::list<std::unique_ptr<MusicLoader>> musicLoaders;

  public:
	FileSystem fs;

	Data(std::vector<UString> paths, int imageCacheSize = 100, int imageSetCacheSize = 10,
	     int voxelCacheSize = 1);
	~Data();

	std::shared_ptr<Sample> load_sample(const UString &path);
	std::shared_ptr<MusicTrack> load_music(const UString &path);
	std::shared_ptr<Image> load_image(const UString &path);
	std::shared_ptr<ImageSet> load_image_set(const UString &path);
	std::shared_ptr<Palette> load_palette(const UString &path);
	std::shared_ptr<VoxelSlice> load_voxel_slice(const UString &path);
};

} // namespace OpenApoc

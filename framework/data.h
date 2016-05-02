#pragma once
#include "library/sp.h"

#include "framework/fs.h"
#include "framework/image.h"
#include "framework/sound.h"

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

namespace OpenApoc
{
class ImageLoader;
class SampleLoader;
class MusicLoader;
class ImageWriter;
class VoxelSlice;
class LOFTemps;
class ResourceAliases;

class ImageLoaderFactory;
class ImageWriterFactory;
class SampleLoaderFactory;
class MusicLoaderFactory;

class Data
{

  private:
	std::map<UString, std::weak_ptr<Image>> imageCache;
	std::map<UString, std::weak_ptr<Image>> imageCacheLazy;
	std::recursive_mutex imageCacheLock;
	std::map<UString, std::weak_ptr<ImageSet>> imageSetCache;
	std::recursive_mutex imageSetCacheLock;

	std::map<UString, std::weak_ptr<Sample>> sampleCache;
	std::recursive_mutex sampleCacheLock;
	std::map<UString, std::weak_ptr<MusicTrack>> musicCache;
	std::recursive_mutex musicCacheLock;
	std::map<UString, std::weak_ptr<LOFTemps>> LOFVoxelCache;
	std::recursive_mutex voxelCacheLock;

	// Pin open 'imageCacheSize' images
	std::queue<sp<Image>> pinnedImages;
	// Pin open 'imageSetCacheSize' image sets
	std::queue<sp<ImageSet>> pinnedImageSets;
	std::queue<sp<LOFTemps>> pinnedLOFVoxels;
	std::list<std::unique_ptr<ImageLoader>> imageLoaders;
	std::list<std::unique_ptr<SampleLoader>> sampleLoaders;
	std::list<std::unique_ptr<MusicLoader>> musicLoaders;
	std::list<std::unique_ptr<ImageWriter>> imageWriters;

	std::map<UString, std::unique_ptr<ImageLoaderFactory>> registeredImageBackends;
	std::map<UString, std::unique_ptr<ImageWriterFactory>> registeredImageWriters;
	std::map<UString, std::unique_ptr<SampleLoaderFactory>> registeredSampleLoaders;
	std::map<UString, std::unique_ptr<MusicLoaderFactory>> registeredMusicLoaders;

  public:
	std::weak_ptr<ResourceAliases> aliases;
	FileSystem fs;

	Data(std::vector<UString> paths, int imageCacheSize = 100, int imageSetCacheSize = 10,
	     int voxelCacheSize = 1);
	~Data();

	sp<Sample> load_sample(UString path);
	sp<MusicTrack> load_music(const UString &path);
	sp<Image> load_image(const UString &path, bool lazy = false);
	sp<ImageSet> load_image_set(const UString &path);
	sp<Palette> load_palette(const UString &path);
	sp<VoxelSlice> load_voxel_slice(const UString &path);
	bool write_image(UString systemPath, sp<Image> image, sp<Palette> palette = nullptr);
};

} // namespace OpenApoc

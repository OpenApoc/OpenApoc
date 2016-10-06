#pragma once

#include "framework/fs.h"
#include "library/sp.h"
#include <list>

namespace OpenApoc
{

class ResourceAliases;
class Sample;
class MusicTrack;
class Image;
class ImageSet;
class Palette;
class VoxelSlice;
class Video;
class PaletteImage;
class UString;

class Data
{
  public:
	wp<ResourceAliases> aliases;
	FileSystem fs;

	static Data *createData(std::vector<UString> paths, int imageCacheSize = 100,
	                        int imageSetCacheSize = 10, int voxelCacheSize = 1,
	                        int fontStringCacheSize = 100, int paletteCacheSize = 10);
	virtual ~Data() = default;
	Data(std::vector<UString> paths) : fs(paths) {}

	virtual sp<Sample> loadSample(UString path) = 0;
	virtual sp<MusicTrack> loadMusic(const UString &path) = 0;
	virtual sp<Image> loadImage(const UString &path, bool lazy = false) = 0;
	virtual sp<ImageSet> loadImageSet(const UString &path) = 0;
	virtual sp<Palette> loadPalette(const UString &path) = 0;
	virtual sp<VoxelSlice> loadVoxelSlice(const UString &path) = 0;
	virtual sp<Video> loadVideo(const UString &path) = 0;

	virtual sp<PaletteImage> getFontStringCacheEntry(const UString &font_name,
	                                                 const UString &text) = 0;
	virtual void putFontStringCacheEntry(const UString &font_name, const UString &text,
	                                     sp<PaletteImage> &img) = 0;

	virtual bool writeImage(UString systemPath, sp<Image> image, sp<Palette> palette = nullptr) = 0;
};

} // namespace OpenApoc

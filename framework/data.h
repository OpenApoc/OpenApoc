#pragma once

#include "framework/fs.h"
#include "library/sp.h"
#include "library/strings.h"
#include <vector>

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

class Data
{
  public:
	FileSystem fs;

	static Data *createData(std::vector<UString> paths);
	virtual ~Data() = default;
	Data(std::vector<UString> paths) : fs(paths) {}

	virtual sp<Sample> loadSample(UString path) = 0;
	virtual sp<MusicTrack> loadMusic(const UString &path) = 0;
	virtual sp<Image> loadImage(const UString &path, bool lazy = false) = 0;
	virtual sp<ImageSet> loadImageSet(const UString &path) = 0;
	virtual sp<Palette> loadPalette(const UString &path) = 0;
	virtual sp<VoxelSlice> loadVoxelSlice(const UString &path) = 0;
	virtual sp<Video> loadVideo(const UString &path) = 0;

	virtual void addSampleAlias(const UString &name, const UString &value) = 0;
	virtual void addMusicAlias(const UString &name, const UString &value) = 0;
	virtual void addImageAlias(const UString &name, const UString &value) = 0;
	virtual void addImageSetAlias(const UString &name, const UString &value) = 0;
	virtual void addPaletteAlias(const UString &name, const UString &value) = 0;
	virtual void addVoxelSliceAlias(const UString &name, const UString &value) = 0;

	virtual sp<PaletteImage> getFontStringCacheEntry(const UString &font_name,
	                                                 const UString &text) = 0;
	virtual void putFontStringCacheEntry(const UString &font_name, const UString &text,
	                                     sp<PaletteImage> &img) = 0;

	virtual bool writeImage(UString systemPath, sp<Image> image, sp<Palette> palette = nullptr) = 0;
};

} // namespace OpenApoc

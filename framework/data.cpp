#include "framework/data.h"
#include "framework/apocresources/apocpalette.h"
#include "framework/apocresources/loftemps.h"
#include "framework/apocresources/pck.h"
#include "framework/apocresources/rawimage.h"
#include "framework/configfile.h"
#include "framework/image.h"
#include "framework/imageloader_interface.h"
#include "framework/logger.h"
#include "framework/musicloader_interface.h"
#include "framework/palette.h"
#include "framework/sampleloader_interface.h"
#include "framework/trace.h"
#include "framework/video.h"
#include "game/state/rules/resource_aliases.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/voxel.h"
#include <fstream>
#include <mutex>
#include <queue>

#define BOOST_ALL_NO_LIB
// boost::fs used to create directories in writeImage
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

using namespace OpenApoc;

namespace OpenApoc
{

ConfigOptionInt imageCacheSize("Framework.Data", "ImageCacheSize",
                               "Number of Images to keep in data cache", 100);
ConfigOptionInt imageSetCacheSize("Framework.Data", "ImageSetCacheSize",
                                  "Number of ImageSets to keep in data cache", 10);
ConfigOptionInt voxelCacheSize("Framework.Data", "VoxelCacheSize",
                               "Number of VoxelMaps to keep in data cache", 1);
ConfigOptionInt fontStringCacheSize("Framework.Data", "FontStringCacheSize",
                                    "Number of rendered font stings to keep in data cache", 100);
ConfigOptionInt paletteCacheSize("Framework.Data", "PaletteCacheSize",
                                 "Number of Palettes to keep in data cache", 10);

class DataImpl final : public Data
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

	std::map<UString, std::weak_ptr<Palette>> paletteCache;
	std::recursive_mutex paletteCacheLock;

	// The cache is organised in <font name , <text, image>>
	std::map<UString, std::map<UString, std::weak_ptr<PaletteImage>>> fontStringCache;
	std::recursive_mutex fontStringCacheLock;

	// Pin open 'imageCacheSize' images
	std::queue<sp<Image>> pinnedImages;
	// Pin open 'imageSetCacheSize' image sets
	std::queue<sp<ImageSet>> pinnedImageSets;
	std::queue<sp<LOFTemps>> pinnedLOFVoxels;
	std::queue<sp<PaletteImage>> pinnedFontStrings;
	std::queue<sp<Palette>> pinnedPalettes;
	std::list<std::unique_ptr<ImageLoader>> imageLoaders;
	std::list<std::unique_ptr<SampleLoader>> sampleLoaders;
	std::list<std::unique_ptr<MusicLoader>> musicLoaders;
	std::list<std::unique_ptr<ImageWriter>> imageWriters;

	std::map<UString, std::unique_ptr<ImageLoaderFactory>> registeredImageBackends;
	std::map<UString, std::unique_ptr<ImageWriterFactory>> registeredImageWriters;
	std::map<UString, std::unique_ptr<SampleLoaderFactory>> registeredSampleLoaders;
	std::map<UString, std::unique_ptr<MusicLoaderFactory>> registeredMusicLoaders;

  public:
	DataImpl(std::vector<UString> paths);
	~DataImpl() override = default;

	sp<Sample> loadSample(UString path) override;
	sp<MusicTrack> loadMusic(const UString &path) override;
	sp<Image> loadImage(const UString &path, bool lazy = false) override;
	sp<ImageSet> loadImageSet(const UString &path) override;
	sp<Palette> loadPalette(const UString &path) override;
	sp<VoxelSlice> loadVoxelSlice(const UString &path) override;
	sp<Video> loadVideo(const UString &path) override;

	sp<PaletteImage> getFontStringCacheEntry(const UString &font_name,
	                                         const UString &text) override;
	void putFontStringCacheEntry(const UString &font_name, const UString &text,
	                             sp<PaletteImage> &img) override;

	bool writeImage(UString systemPath, sp<Image> image, sp<Palette> palette = nullptr) override;
};

Data *Data::createData(std::vector<UString> paths) { return new DataImpl(paths); }

DataImpl::DataImpl(std::vector<UString> paths) : Data(paths)
{
	registeredImageBackends["lodepng"].reset(getLodePNGImageLoaderFactory());
	registeredImageBackends["pcx"].reset(getPCXImageLoaderFactory());
	for (auto &imageBackend : registeredImageBackends)
	{
		auto t = imageBackend.first;
		ImageLoader *l = imageBackend.second->create();
		if (l)
		{
			this->imageLoaders.emplace_back(l);
			LogInfo("Initialised image loader %s", t.cStr());
		}
		else
			LogWarning("Failed to load image loader %s", t.cStr());
	}

	registeredImageWriters["lodepng"].reset(getLodePNGImageWriterFactory());

	for (auto &imageWriter : registeredImageWriters)
	{
		auto t = imageWriter.first;
		ImageWriter *l = imageWriter.second->create();
		if (l)
		{
			this->imageWriters.emplace_back(l);
			LogInfo("Initialised image writer %s", t.cStr());
		}
		else
			LogWarning("Failed to load image writer %s", t.cStr());
	}

	registeredSampleLoaders["raw"].reset(getRAWSampleLoaderFactory());
	for (auto &sampleBackend : registeredSampleLoaders)
	{
		auto t = sampleBackend.first;
		SampleLoader *s = sampleBackend.second->create(*this);
		if (s)
		{
			this->sampleLoaders.emplace_back(s);
			LogInfo("Initialised sample loader %s", t.cStr());
		}
		else
			LogWarning("Failed to load sample loader %s", t.cStr());
	}

	registeredMusicLoaders["raw"].reset(getRAWMusicLoaderFactory());
	for (auto &musicLoader : registeredMusicLoaders)
	{
		auto t = musicLoader.first;
		MusicLoader *m = musicLoader.second->create(*this);
		if (m)
		{
			this->musicLoaders.emplace_back(m);
			LogInfo("Initialised music loader %s", t.cStr());
		}
		else
			LogWarning("Failed to load music loader %s", t.cStr());
	}
	for (int i = 0; i < imageCacheSize.get(); i++)
		pinnedImages.push(nullptr);
	for (int i = 0; i < imageSetCacheSize.get(); i++)
		pinnedImageSets.push(nullptr);
	for (int i = 0; i < voxelCacheSize.get(); i++)
		pinnedLOFVoxels.push(nullptr);
	for (int i = 0; i < fontStringCacheSize.get(); i++)
		pinnedFontStrings.push(nullptr);
	for (int i = 0; i < paletteCacheSize.get(); i++)
		pinnedPalettes.push(nullptr);
}

sp<VoxelSlice> DataImpl::loadVoxelSlice(const UString &path)
{
	std::lock_guard<std::recursive_mutex> l(this->voxelCacheLock);
	if (path == "")
		return nullptr;
	sp<VoxelSlice> slice;
	if (path.substr(0, 9) == "LOFTEMPS:")
	{
		auto splitString = path.split(':');
		// "LOFTEMPS:DATFILE:TABFILE:INDEX"
		//  or
		// "LOFTEMPS:DATFILE:TABFILE:INDEX:X:Y"
		if (splitString.size() != 4)
		{
			LogError("Invalid LOFTEMPS string \"%s\"", path.cStr());
			return nullptr;
		}
		// Cut off the index to get the LOFTemps file
		UString cacheKey = splitString[0] + splitString[1] + splitString[2];
		cacheKey = cacheKey.toUpper();
		sp<LOFTemps> lofTemps = this->LOFVoxelCache[cacheKey].lock();
		if (!lofTemps)
		{
			TRACE_FN_ARGS1("path", path);
			auto datFile = this->fs.open(splitString[1]);
			if (!datFile)
			{
				LogError("Failed to open LOFTemps dat file \"%s\"", splitString[1].cStr());
				return nullptr;
			}
			auto tabFile = this->fs.open(splitString[2]);
			if (!tabFile)
			{
				LogError("Failed to open LOFTemps tab file \"%s\"", splitString[2].cStr());
				return nullptr;
			}
			lofTemps = mksp<LOFTemps>(datFile, tabFile);
			this->LOFVoxelCache[cacheKey] = lofTemps;
			this->pinnedLOFVoxels.push(lofTemps);
			this->pinnedLOFVoxels.pop();
		}
		int idx = Strings::toInteger(splitString[3]);
		slice = lofTemps->getSlice(idx);
		if (!slice)
		{
			LogError("Invalid idx %d", idx);
		}
	}

	if (!slice)
	{
		LogError("Failed to load VoxelSlice \"%s\"", path.cStr());
		return nullptr;
	}
	slice->path = path;
	return slice;
}

sp<ImageSet> DataImpl::loadImageSet(const UString &path)
{
	std::lock_guard<std::recursive_mutex> l(this->imageSetCacheLock);
	UString cacheKey = path.toUpper();
	sp<ImageSet> imgSet = this->imageSetCache[cacheKey].lock();
	if (imgSet)
	{
		return imgSet;
	}
	TRACE_FN_ARGS1("path", path);
	// Raw resources come in the format:
	//"RAW:PATH:WIDTH:HEIGHT[:optional/ignored]"
	if (path.substr(0, 4) == "RAW:")
	{
		auto splitString = path.split(':');
		imgSet =
		    RawImage::loadSet(*this, splitString[1], Vec2<int>{Strings::toInteger(splitString[2]),
		                                                       Strings::toInteger(splitString[3])});
	}
	// PCK resources come in the format:
	//"PCK:PCKFILE:TABFILE[:optional/ignored]"
	else if (path.substr(0, 4) == "PCK:")
	{
		auto splitString = path.split(':');
		imgSet = PCKLoader::load(*this, splitString[1], splitString[2]);
	}
	else if (path.substr(0, 9) == "PCKSTRAT:")
	{
		auto splitString = path.split(':');
		imgSet = PCKLoader::loadStrat(*this, splitString[1], splitString[2]);
	}
	else if (path.substr(0, 10) == "PCKSHADOW:")
	{
		auto splitString = path.split(':');
		imgSet = PCKLoader::loadShadow(*this, splitString[1], splitString[2]);
	}
	else
	{
		LogError("Unknown image set format \"%s\"", path.cStr());
		return nullptr;
	}

	this->pinnedImageSets.push(imgSet);
	this->pinnedImageSets.pop();

	this->imageSetCache[cacheKey] = imgSet;
	imgSet->path = path;
	return imgSet;
}

sp<Sample> DataImpl::loadSample(UString path)
{
	std::lock_guard<std::recursive_mutex> l(this->sampleCacheLock);
	auto aliasMap = this->aliases.lock();
	if (aliasMap)
	{
		auto aliasIt = aliasMap->sample.find(path);
		if (aliasIt != aliasMap->sample.end())
		{
			LogInfo("Aliasing sample \"%s\" to \"%s\"", path.cStr(), aliasIt->second.cStr());
			path = aliasIt->second;
		}
	}
	UString cacheKey = path.toUpper();
	sp<Sample> sample = this->sampleCache[cacheKey].lock();
	if (sample)
		return sample;

	TRACE_FN_ARGS1("path", path);
	for (auto &loader : this->sampleLoaders)
	{
		sample = loader->loadSample(path);
		if (sample)
			break;
	}
	if (!sample)
	{
		LogInfo("Failed to load sample \"%s\"", path.cStr());
		return nullptr;
	}
	this->sampleCache[cacheKey] = sample;
	sample->path = path;
	return sample;
}

sp<MusicTrack> DataImpl::loadMusic(const UString &path)
{
	std::lock_guard<std::recursive_mutex> l(this->musicCacheLock);
	TRACE_FN_ARGS1("path", path);
	// No cache for music tracks, just stream of disk
	for (auto &loader : this->musicLoaders)
	{
		auto track = loader->loadMusic(path);
		if (track)
		{
			track->path = path;
			return track;
		}
	}
	LogInfo("Failed to load music track \"%s\"", path.cStr());
	return nullptr;
}

sp<Image> DataImpl::loadImage(const UString &path, bool lazy)
{
	std::lock_guard<std::recursive_mutex> l(this->imageCacheLock);
	if (path == "")
	{
		return nullptr;
	}
	// Use an uppercase version of the path for the cache key
	UString cacheKey = path.toUpper();
	sp<Image> img;
	// Don't cache lazy loading image wrappers, the image data when really loaded will go through
	// the cache as normal, but we don't want to think we've loaded an image when it's just a lazy
	// wrapper
	if (!lazy)
	{
		img = this->imageCache[cacheKey].lock();
	}
	if (img)
	{
		return img;
	}

	// Only trace stuff that misses the cache
	TRACE_FN_ARGS1("path", path);

	if (lazy)
	{
		img = mksp<LazyImage>();
	}
	else if (path.substr(0, 4) == "RAW:")
	{
		auto splitString = path.split(':');
		// Raw resources come in the format:
		//"RAW:PATH:WIDTH:HEIGHT[:PALETTE]"
		// or
		//"RAW:PATH:WIDTH:HEIGHT:INDEX[:PALETTE]" (for imagesets)
		if (splitString.size() != 4 && splitString.size() != 5 && splitString.size() != 6)
		{
			LogError("Invalid RAW resource string: \"%s\"", path.cStr());
			return nullptr;
		}

		sp<PaletteImage> pImg;
		size_t palettePos;
		if (splitString.size() >= 5 && Strings::isInteger(splitString[4]))
		{
			auto imageSet = this->loadImageSet(splitString[0] + ":" + splitString[1] + ":" +
			                                   splitString[2] + ":" + splitString[3]);
			if (!imageSet)
			{
				return nullptr;
			}
			pImg = std::dynamic_pointer_cast<PaletteImage>(
			    imageSet->images[Strings::toInteger(splitString[4])]);
			palettePos = 5;
		}
		else
		{
			pImg = RawImage::load(
			    *this, splitString[1],
			    Vec2<int>{Strings::toInteger(splitString[2]), Strings::toInteger(splitString[3])});
			palettePos = 4;
		}
		if (!pImg)
		{
			LogError("Failed to load RAW image: \"%s\"", path.cStr());
			return nullptr;
		}
		if (splitString.size() > palettePos)
		{
			auto pal = this->loadPalette(splitString[palettePos]);
			if (!pal)
			{
				LogError("Failed to load palette for RAW image: \"%s\"", path.cStr());
				return nullptr;
			}
			img = pImg->toRGBImage(pal);
		}
		else
		{
			img = pImg;
		}
	}
	else if (path.substr(0, 4) == "PCK:")
	{
		auto splitString = path.split(':');
		if (splitString.size() != 3 && splitString.size() != 4 && splitString.size() != 5)
		{
			LogError("Invalid PCK resource string: \"%s\"", path.cStr());
			return nullptr;
		}
		auto imageSet =
		    this->loadImageSet(splitString[0] + ":" + splitString[1] + ":" + splitString[2]);
		if (!imageSet)
		{
			return nullptr;
		}
		// PCK resources come in the format:
		//"PCK:PCKFILE:TABFILE:INDEX"
		// or
		//"PCK:PCKFILE:TABFILE:INDEX:PALETTE" if we want them already in rgb space
		switch (splitString.size())
		{
			case 4:
			{
				img = imageSet->images[Strings::toInteger(splitString[3])];
				break;
			}
			case 5:
			{
				sp<PaletteImage> pImg = std::dynamic_pointer_cast<PaletteImage>(this->loadImage(
				    "PCK:" + splitString[1] + ":" + splitString[2] + ":" + splitString[3]));
				// In some cases, PCKS do not have enough pictures even though TAB references them.
				// Example: tacdata/unit/xcom1a.pck
				// We should not throw in this case.
				if (!pImg)
					return nullptr;
				LogAssert(pImg);
				auto pal = this->loadPalette(splitString[4]);
				LogAssert(pal);
				img = pImg->toRGBImage(pal);
				break;
			}
			default:
				LogError("Invalid PCK resource string \"%s\"", path.cStr());
				return nullptr;
		}
	}
	else if (path.substr(0, 9) == "PCKSTRAT:")
	{
		auto splitString = path.split(':');
		if (splitString.size() != 3 && splitString.size() != 4 && splitString.size() != 5)
		{
			LogError("Invalid PCKSTRAT resource string: \"%s\"", path.cStr());
			return nullptr;
		}
		auto imageSet =
		    this->loadImageSet(splitString[0] + ":" + splitString[1] + ":" + splitString[2]);
		if (!imageSet)
		{
			return nullptr;
		}
		// PCK resources come in the format:
		//"PCK:PCKFILE:TABFILE:INDEX"
		// or
		//"PCK:PCKFILE:TABFILE:INDEX:PALETTE" if we want them already in rgb space
		switch (splitString.size())
		{
			case 4:
			{
				img = imageSet->images[Strings::toInteger(splitString[3])];
				break;
			}
			case 5:
			{
				sp<PaletteImage> pImg = std::dynamic_pointer_cast<PaletteImage>(this->loadImage(
				    "PCKSTRAT:" + splitString[1] + ":" + splitString[2] + ":" + splitString[3]));
				LogAssert(pImg);
				auto pal = this->loadPalette(splitString[4]);
				LogAssert(pal);
				img = pImg->toRGBImage(pal);
				break;
			}
			default:
				LogError("Invalid PCKSTRAT resource string \"%s\"", path.cStr());
				return nullptr;
		}
	}
	else if (path.substr(0, 10) == "PCKSHADOW:")
	{
		auto splitString = path.split(':');
		if (splitString.size() != 3 && splitString.size() != 4 && splitString.size() != 5)
		{
			LogError("Invalid PCKSHADOW resource string: \"%s\"", path.cStr());
			return nullptr;
		}
		auto imageSet =
		    this->loadImageSet(splitString[0] + ":" + splitString[1] + ":" + splitString[2]);
		if (!imageSet)
		{
			return nullptr;
		}
		// PCK resources come in the format:
		//"PCK:PCKFILE:TABFILE:INDEX"
		// or
		//"PCK:PCKFILE:TABFILE:INDEX:PALETTE" if we want them already in rgb space
		switch (splitString.size())
		{
			case 4:
			{
				img = imageSet->images[Strings::toInteger(splitString[3])];
				break;
			}
			case 5:
			{
				sp<PaletteImage> pImg = std::dynamic_pointer_cast<PaletteImage>(this->loadImage(
				    "PCKSHADOW:" + splitString[1] + ":" + splitString[2] + ":" + splitString[3]));
				LogAssert(pImg);
				auto pal = this->loadPalette(splitString[4]);
				LogAssert(pal);
				img = pImg->toRGBImage(pal);
				break;
			}
			default:
				LogError("Invalid PCKSHADOW resource string \"%s\"", path.cStr());
				return nullptr;
		}
	}
	else if (path.substr(0, 9) == "LOFTEMPS:")
	{
		LogInfo("Loading LOFTEMPS \"%s\" as image", path.cStr());
		auto voxelSlice = this->loadVoxelSlice(path);
		if (!voxelSlice)
		{
			return nullptr;
		}
		Colour emptyColour{0, 0, 0, 0};
		Colour filledColour{255, 255, 255, 255};
		auto rgbImg = mksp<RGBImage>(voxelSlice->size);
		RGBImageLock l(rgbImg, ImageLockUse::Write);
		for (int y = 0; y < voxelSlice->size.y; y++)
		{
			for (int x = 0; x < voxelSlice->size.x; x++)
			{
				if (voxelSlice->getBit({x, y}))
					l.set({x, y}, filledColour);
				else
					l.set({x, y}, emptyColour);
			}
		}
		img = rgbImg;
	}
	else
	{
		for (auto &loader : imageLoaders)
		{
			auto file = fs.open(path);
			if (!file)
				break;
			img = loader->loadImage(file);
			if (img)
			{
				break;
			}
		}
		if (!img)
		{
			LogInfo("Failed to load image \"%s\"", path.cStr());
			return nullptr;
		}
	}

	if (!img)
	{
		LogInfo("Failed to load image \"%s\"", path.cStr());
		return nullptr;
	}

	this->pinnedImages.push(img);
	this->pinnedImages.pop();

	if (!lazy)
	{
		this->imageCache[cacheKey] = img;
	}
	img->path = path;
	return img;
}

sp<Palette> DataImpl::loadPalette(const UString &path)
{
	std::lock_guard<std::recursive_mutex> l(this->paletteCacheLock);
	if (path == "")
	{
		LogWarning("Invalid palette path");
		return nullptr;
	}
	// Use an uppercase version of the path for the cache key
	UString cacheKey = path.toUpper();

	auto pal = this->paletteCache[cacheKey].lock();
	if (pal)
	{
		return pal;
	}

	pal = loadPCXPalette(*this, path);
	if (pal)
	{
		LogInfo("Read \"%s\" as PCX palette", path.cStr());
		this->paletteCache[cacheKey] = pal;
		this->pinnedPalettes.push(pal);
		this->pinnedPalettes.pop();
		return pal;
	}
	pal = loadPNGPalette(*this, path);
	if (pal)
	{
		LogInfo("Read \"%s\" as PNG palette", path.cStr());
		this->paletteCache[cacheKey] = pal;
		this->pinnedPalettes.push(pal);
		this->pinnedPalettes.pop();
		return pal;
	}

	sp<RGBImage> img = std::dynamic_pointer_cast<RGBImage>(this->loadImage(path));
	if (img)
	{
		unsigned int idx = 0;
		auto p = mksp<Palette>(img->size.x * img->size.y);
		RGBImageLock src{img, ImageLockUse::Read};
		for (unsigned int y = 0; y < img->size.y; y++)
		{
			for (unsigned int x = 0; x < img->size.x; x++)
			{
				Colour c = src.get(Vec2<int>{x, y});
				p->setColour(idx, c);
				idx++;
			}
		}
		LogInfo("Read \"%s\" as Image palette", path.cStr());
		this->paletteCache[cacheKey] = pal;
		this->pinnedPalettes.push(pal);
		this->pinnedPalettes.pop();
		return p;
	}

	pal = loadApocPalette(*this, path);
	if (pal)
	{
		LogInfo("Read \"%s\" as RAW palette", path.cStr());
		this->paletteCache[cacheKey] = pal;
		this->pinnedPalettes.push(pal);
		this->pinnedPalettes.pop();
		return pal;
	}
	LogError("Failed to open palette \"%s\"", path.cStr());
	return nullptr;
}

sp<Video> DataImpl::loadVideo(const UString &path)
{

	if (path.substr(0, 4) == "SMK:")
	{
		auto splitString = path.split(':');
		if (splitString.size() != 2)
		{
			LogError("Invalid SMK string: \"%s\"", path.cStr());
			return nullptr;
		}
		auto file = this->fs.open(splitString[1]);
		if (!file)
		{
			LogWarning("Failed to open SMK file \"%s\"", splitString[1].cStr());
			return nullptr;
		}
		return loadSMKVideo(file);
	}
	LogError("Unknown video string \"%s\"", path.cStr());
	return nullptr;
}

bool DataImpl::writeImage(UString systemPath, sp<Image> image, sp<Palette> palette)
{
	auto outPath = fs::path(systemPath.str());
	auto outDir = outPath.parent_path();
	try
	{
		fs::create_directories(outDir);
	}
	// Just catch any problem and continue anyway?
	catch (fs::filesystem_error e)
	{
		LogWarning("create_directories failed with \"%s\"", e.what());
	}
	std::ofstream outFile(systemPath.str(), std::ios::binary);
	if (!outFile)
	{
		LogWarning("Failed to open \"%s\" for writing", systemPath.cStr());
		return false;
	}

	for (auto &writer : imageWriters)
	{
		auto palImg = std::dynamic_pointer_cast<PaletteImage>(image);
		auto rgbImg = std::dynamic_pointer_cast<RGBImage>(image);
		if (palImg)
		{
			if (!palette)
			{
				UString defaultPalettePath = "xcom3/ufodata/pal_01.dat";
				LogInfo("Loading default palette \"%s\"", defaultPalettePath.cStr());
				palette = this->loadPalette(defaultPalettePath);
				if (!palette)
				{
					LogWarning("Failed to write palette image - no palette supplied and failed to "
					           "load default");
					return false;
				}
			}
			if (writer->writeImage(palImg, outFile, palette))
			{
				LogInfo("Successfully wrote palette image \"%s\" using \"%s\"", systemPath.cStr(),
				        writer->getName().cStr());
				return true;
			}
			else
			{
				LogWarning("Failed to write palette image \"%s\" using \"%s\"", systemPath.cStr(),
				           writer->getName().cStr());
				continue;
			}
		}
		else if (rgbImg)
		{
			if (writer->writeImage(rgbImg, outFile))
			{
				LogInfo("Successfully wrote RGB image \"%s\" using \"%s\"", systemPath.cStr(),
				        writer->getName().cStr());
				return true;
			}
			else
			{
				LogWarning("Failed to write RGB image \"%s\" using \"%s\"", systemPath.cStr(),
				           writer->getName().cStr());
				continue;
			}
		}
		else
		{
			LogError("Unknown image type");
			return false;
		}
	}

	LogWarning("No writes succeeded for image \"%s\"", systemPath.cStr());
	return false;
}

sp<PaletteImage> DataImpl::getFontStringCacheEntry(const UString &font_name, const UString &string)
{
	std::lock_guard<std::recursive_mutex> l(this->fontStringCacheLock);
	if (font_name == "")
	{
		LogError("invalid font_name");
		return nullptr;
	}
	if (string == "")
	{
		// LogWarning("Empty string");
		return nullptr;
	}
	auto img = this->fontStringCache[font_name][string].lock();
	return img;
}

void DataImpl::putFontStringCacheEntry(const UString &font_name, const UString &string,
                                       sp<PaletteImage> &img)
{
	std::lock_guard<std::recursive_mutex> l(this->fontStringCacheLock);
	if (font_name == "")
	{
		LogError("invalid font_name");
		return;
	}
	if (string == "")
	{
		// LogWarning("Empty string");
		return;
	}
	this->fontStringCache[font_name][string] = img;
	this->pinnedFontStrings.push(img);
	this->pinnedFontStrings.pop();
}

}; // namespace OpenApoc

#include "framework/data.h"
#include "framework/apocresources/apocpalette.h"
#include "framework/apocresources/loftemps.h"
#include "framework/apocresources/pck.h"
#include "framework/apocresources/rawimage.h"
#include "framework/logger.h"
#include "framework/palette.h"
#include "framework/trace.h"
#include "game/state/rules/resource_aliases.h"
#include "library/sp.h"
#include "library/strings.h"

#include "framework/imageloader_interface.h"
#include "framework/musicloader_interface.h"
#include "framework/sampleloader_interface.h"

#include <fstream>

using namespace OpenApoc;

namespace OpenApoc
{

Data::Data(std::vector<UString> paths, int imageCacheSize, int imageSetCacheSize,
           int voxelCacheSize)
    : fs(paths)
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
			LogInfo("Initialised image loader %s", t.c_str());
		}
		else
			LogWarning("Failed to load image loader %s", t.c_str());
	}

	registeredImageWriters["lodepng"].reset(getLodePNGImageWriterFactory());

	for (auto &imageWriter : registeredImageWriters)
	{
		auto t = imageWriter.first;
		ImageWriter *l = imageWriter.second->create();
		if (l)
		{
			this->imageWriters.emplace_back(l);
			LogInfo("Initialised image writer %s", t.c_str());
		}
		else
			LogWarning("Failed to load image writer %s", t.c_str());
	}

	registeredSampleLoaders["raw"].reset(getRAWSampleLoaderFactory());
	for (auto &sampleBackend : registeredSampleLoaders)
	{
		auto t = sampleBackend.first;
		SampleLoader *s = sampleBackend.second->create(*this);
		if (s)
		{
			this->sampleLoaders.emplace_back(s);
			LogInfo("Initialised sample loader %s", t.c_str());
		}
		else
			LogWarning("Failed to load sample loader %s", t.c_str());
	}

	registeredMusicLoaders["raw"].reset(getRAWMusicLoaderFactory());
	for (auto &musicLoader : registeredMusicLoaders)
	{
		auto t = musicLoader.first;
		MusicLoader *m = musicLoader.second->create(*this);
		if (m)
		{
			this->musicLoaders.emplace_back(m);
			LogInfo("Initialised music loader %s", t.c_str());
		}
		else
			LogWarning("Failed to load music loader %s", t.c_str());
	}
	for (int i = 0; i < imageCacheSize; i++)
		pinnedImages.push(nullptr);
	for (int i = 0; i < imageSetCacheSize; i++)
		pinnedImageSets.push(nullptr);
	for (int i = 0; i < voxelCacheSize; i++)
		pinnedLOFVoxels.push(nullptr);
}

Data::~Data() {}

sp<VoxelSlice> Data::load_voxel_slice(const UString &path)
{
	std::lock_guard<std::recursive_mutex> l(this->voxelCacheLock);
	if (path == "")
		return nullptr;
	sp<VoxelSlice> slice;
	if (path.substr(0, 9) == "LOFTEMPS:")
	{
		auto splitString = path.split(':');
		//"LOFTEMPS:DATFILE:TABFILE:INDEX"
		if (splitString.size() != 4)
		{
			LogError("Invalid LOFTEMPS string \"%s\"", path.c_str());
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
				LogError("Failed to open LOFTemps dat file \"%s\"", splitString[1].c_str());
				return nullptr;
			}
			auto tabFile = this->fs.open(splitString[2]);
			if (!tabFile)
			{
				LogError("Failed to open LOFTemps tab file \"%s\"", splitString[2].c_str());
				return nullptr;
			}
			lofTemps = mksp<LOFTemps>(datFile, tabFile);
			this->LOFVoxelCache[cacheKey] = lofTemps;
			this->pinnedLOFVoxels.push(lofTemps);
			this->pinnedLOFVoxels.pop();
		}
		int idx = Strings::ToInteger(splitString[3]);
		slice = lofTemps->getSlice(idx);
		if (!slice)
		{
			LogError("Invalid idx %d", idx);
		}
	}

	if (!slice)
	{
		LogError("Failed to load VoxelSlice \"%s\"", path.c_str());
		return nullptr;
	}
	slice->path = path;
	return slice;
}

sp<ImageSet> Data::load_image_set(const UString &path)
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
		imgSet = RawImage::load_set(
		    *this, splitString[1],
		    Vec2<int>{Strings::ToInteger(splitString[2]), Strings::ToInteger(splitString[3])});
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
		imgSet = PCKLoader::load_strat(*this, splitString[1], splitString[2]);
	}
	else if (path.substr(0, 10) == "PCKSHADOW:")
	{
		auto splitString = path.split(':');
		imgSet = PCKLoader::load_shadow(*this, splitString[1], splitString[2]);
	}
	else
	{
		LogError("Unknown image set format \"%s\"", path.c_str());
		return nullptr;
	}

	this->pinnedImageSets.push(imgSet);
	this->pinnedImageSets.pop();

	this->imageSetCache[cacheKey] = imgSet;
	imgSet->path = path;
	return imgSet;
}

sp<Sample> Data::load_sample(UString path)
{
	std::lock_guard<std::recursive_mutex> l(this->sampleCacheLock);
	auto aliasMap = this->aliases.lock();
	if (aliasMap)
	{
		auto aliasIt = aliasMap->sample.find(path);
		if (aliasIt != aliasMap->sample.end())
		{
			LogInfo("Aliasing sample \"%s\" to \"%s\"", path.c_str(), aliasIt->second.c_str());
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
		LogInfo("Failed to load sample \"%s\"", path.c_str());
		return nullptr;
	}
	this->sampleCache[cacheKey] = sample;
	sample->path = path;
	return sample;
}

sp<MusicTrack> Data::load_music(const UString &path)
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
	LogInfo("Failed to load music track \"%s\"", path.c_str());
	return nullptr;
}

sp<Image> Data::load_image(const UString &path, bool lazy)
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
			LogError("Invalid RAW resource string: \"%s\"", path.c_str());
			return nullptr;
		}

		sp<PaletteImage> pImg;
		size_t palettePos;
		if (splitString.size() >= 5 && Strings::IsInteger(splitString[4]))
		{
			auto imageSet = this->load_image_set(splitString[0] + ":" + splitString[1] + ":" +
			                                     splitString[2] + ":" + splitString[3]);
			if (!imageSet)
			{
				return nullptr;
			}
			pImg = std::dynamic_pointer_cast<PaletteImage>(
			    imageSet->images[Strings::ToInteger(splitString[4])]);
			palettePos = 5;
		}
		else
		{
			pImg = RawImage::load(
			    *this, splitString[1],
			    Vec2<int>{Strings::ToInteger(splitString[2]), Strings::ToInteger(splitString[3])});
			palettePos = 4;
		}
		if (!pImg)
		{
			LogError("Failed to load RAW image: \"%s\"", path.c_str());
			return nullptr;
		}
		if (splitString.size() > palettePos)
		{
			auto pal = this->load_palette(splitString[palettePos]);
			if (!pal)
			{
				LogError("Failed to load palette for RAW image: \"%s\"", path.c_str());
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
			LogError("Invalid PCK resource string: \"%s\"", path.c_str());
			return nullptr;
		}
		auto imageSet =
		    this->load_image_set(splitString[0] + ":" + splitString[1] + ":" + splitString[2]);
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
				img = imageSet->images[Strings::ToInteger(splitString[3])];
				break;
			}
			case 5:
			{
				sp<PaletteImage> pImg = std::dynamic_pointer_cast<PaletteImage>(this->load_image(
				    "PCK:" + splitString[1] + ":" + splitString[2] + ":" + splitString[3]));
				assert(pImg);
				auto pal = this->load_palette(splitString[4]);
				assert(pal);
				img = pImg->toRGBImage(pal);
				break;
			}
			default:
				LogError("Invalid PCK resource string \"%s\"", path.c_str());
				return nullptr;
		}
	}
	else if (path.substr(0, 9) == "PCKSTRAT:")
	{
		auto splitString = path.split(':');
		if (splitString.size() != 3 && splitString.size() != 4 && splitString.size() != 5)
		{
			LogError("Invalid PCKSTRAT resource string: \"%s\"", path.c_str());
			return nullptr;
		}
		auto imageSet =
		    this->load_image_set(splitString[0] + ":" + splitString[1] + ":" + splitString[2]);
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
				img = imageSet->images[Strings::ToInteger(splitString[3])];
				break;
			}
			case 5:
			{
				sp<PaletteImage> pImg = std::dynamic_pointer_cast<PaletteImage>(this->load_image(
				    "PCKSTRAT:" + splitString[1] + ":" + splitString[2] + ":" + splitString[3]));
				assert(pImg);
				auto pal = this->load_palette(splitString[4]);
				assert(pal);
				img = pImg->toRGBImage(pal);
				break;
			}
			default:
				LogError("Invalid PCKSTRAT resource string \"%s\"", path.c_str());
				return nullptr;
		}
	}
	else if (path.substr(0, 10) == "PCKSHADOW:")
	{
		auto splitString = path.split(':');
		if (splitString.size() != 3 && splitString.size() != 4 && splitString.size() != 5)
		{
			LogError("Invalid PCKSHADOW resource string: \"%s\"", path.c_str());
			return nullptr;
		}
		auto imageSet =
		    this->load_image_set(splitString[0] + ":" + splitString[1] + ":" + splitString[2]);
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
				img = imageSet->images[Strings::ToInteger(splitString[3])];
				break;
			}
			case 5:
			{
				sp<PaletteImage> pImg = std::dynamic_pointer_cast<PaletteImage>(this->load_image(
				    "PCKSHADOW:" + splitString[1] + ":" + splitString[2] + ":" + splitString[3]));
				assert(pImg);
				auto pal = this->load_palette(splitString[4]);
				assert(pal);
				img = pImg->toRGBImage(pal);
				break;
			}
			default:
				LogError("Invalid PCKSHADOW resource string \"%s\"", path.c_str());
				return nullptr;
		}
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
			LogInfo("Failed to load image \"%s\"", path.c_str());
			return nullptr;
		}
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

sp<Palette> Data::load_palette(const UString &path)
{
	auto pal = loadPCXPalette(*this, path);
	if (pal)
	{
		LogInfo("Read \"%s\" as PCX palette", path.c_str());
		return pal;
	}
	pal = loadPNGPalette(*this, path);
	if (pal)
	{
		LogInfo("Read \"%s\" as PNG palette", path.c_str());
		return pal;
	}

	sp<RGBImage> img = std::dynamic_pointer_cast<RGBImage>(this->load_image(path));
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
				p->SetColour(idx, c);
				idx++;
			}
		}
		LogInfo("Read \"%s\" as Image palette", path.c_str());
		return p;
	}

	pal = loadApocPalette(*this, path);
	if (pal)
	{
		LogInfo("Read \"%s\" as RAW palette", path.c_str());
		return pal;
	}
	LogError("Failed to open palette \"%s\"", path.c_str());
	return nullptr;
}

bool Data::write_image(UString systemPath, sp<Image> image, sp<Palette> palette)
{
	std::ofstream outFile(systemPath.str(), std::ios::binary);
	if (!outFile)
	{
		LogWarning("Failed to open \"%s\" for writing", systemPath.c_str());
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
				LogInfo("Loading default palette \"%s\"", defaultPalettePath.c_str());
				palette = this->load_palette(defaultPalettePath);
				if (!palette)
				{
					LogWarning("Failed to write palette image - no palette supplied and failed to "
					           "load default");
					return false;
				}
			}
			if (writer->writeImage(palImg, outFile, palette))
			{
				LogInfo("Successfully wrote palette image \"%s\" using \"%s\"", systemPath.c_str(),
				        writer->getName().c_str());
				return true;
			}
			else
			{
				LogWarning("Failed to write palette image \"%s\" using \"%s\"", systemPath.c_str(),
				           writer->getName().c_str());
				continue;
			}
		}
		else if (rgbImg)
		{
			if (writer->writeImage(rgbImg, outFile))
			{
				LogInfo("Successfully wrote RGB image \"%s\" using \"%s\"", systemPath.c_str(),
				        writer->getName().c_str());
				return true;
			}
			else
			{
				LogWarning("Failed to write RGB image \"%s\" using \"%s\"", systemPath.c_str(),
				           writer->getName().c_str());
				continue;
			}
		}
		else
		{
			LogError("Unknown image type");
			return false;
		}
	}

	LogWarning("No writes succeeded for image \"%s\"", systemPath.c_str());
	return false;
}

}; // namespace OpenApoc

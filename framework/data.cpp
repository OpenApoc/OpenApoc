#include "library/sp.h"
#include "framework/logger.h"
#include "framework/data.h"
#include "game/apocresources/pck.h"
#include "game/apocresources/rawimage.h"
#include "game/apocresources/apocpalette.h"
#include "game/apocresources/loftemps.h"
#include "framework/palette.h"
#include "framework/trace.h"
#include "library/strings.h"

#include "framework/imageloader_interface.h"
#include "framework/musicloader_interface.h"
#include "framework/sampleloader_interface.h"

using namespace OpenApoc;

namespace
{
std::map<UString, std::unique_ptr<OpenApoc::ImageLoaderFactory>> *registeredImageBackends = nullptr;
std::map<UString, std::unique_ptr<OpenApoc::MusicLoaderFactory>> *registeredMusicLoaders = nullptr;
std::map<UString, std::unique_ptr<OpenApoc::SampleLoaderFactory>> *registeredSampleLoaders =
    nullptr;
}; // anonymous namespace

namespace OpenApoc
{

void registerImageLoader(ImageLoaderFactory *factory, UString name)
{
	if (!registeredImageBackends)
	{
		registeredImageBackends =
		    new std::map<UString, std::unique_ptr<OpenApoc::ImageLoaderFactory>>();
	}
	registeredImageBackends->emplace(name, std::unique_ptr<ImageLoaderFactory>(factory));
}
void registerSampleLoader(SampleLoaderFactory *factory, UString name)
{
	if (!registeredSampleLoaders)
	{
		registeredSampleLoaders =
		    new std::map<UString, std::unique_ptr<OpenApoc::SampleLoaderFactory>>();
	}
	registeredSampleLoaders->emplace(name, std::unique_ptr<SampleLoaderFactory>(factory));
}
void registerMusicLoader(MusicLoaderFactory *factory, UString name)
{
	if (!registeredMusicLoaders)
	{
		registeredMusicLoaders =
		    new std::map<UString, std::unique_ptr<OpenApoc::MusicLoaderFactory>>();
	}
	registeredMusicLoaders->emplace(name, std::unique_ptr<MusicLoaderFactory>(factory));
}

Data::Data(std::vector<UString> paths, int imageCacheSize, int imageSetCacheSize,
           int voxelCacheSize)
    : fs(paths)
{
	for (auto &imageBackend : *registeredImageBackends)
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

	for (auto &sampleBackend : *registeredSampleLoaders)
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

	for (auto &musicLoader : *registeredMusicLoaders)
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
			lofTemps = std::make_shared<LOFTemps>(datFile, tabFile);
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
	return slice;
}

sp<ImageSet> Data::load_image_set(const UString &path)
{
	UString cacheKey = path.toUpper();
	sp<ImageSet> imgSet = this->imageSetCache[cacheKey].lock();
	if (imgSet)
	{
		return imgSet;
	}
	TRACE_FN_ARGS1("path", path);
	// PCK resources come in the format:
	//"PCK:PCKFILE:TABFILE[:optional/ignored]"
	if (path.substr(0, 4) == "PCK:")
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
	return imgSet;
}

sp<Sample> Data::load_sample(const UString &path)
{
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
	return sample;
}

sp<MusicTrack> Data::load_music(const UString &path)
{
	TRACE_FN_ARGS1("path", path);
	// No cache for music tracks, just stream of disk
	for (auto &loader : this->musicLoaders)
	{
		auto track = loader->loadMusic(path);
		if (track)
			return track;
	}
	LogInfo("Failed to load music track \"%s\"", path.c_str());
	return nullptr;
}

sp<Image> Data::load_image(const UString &path)
{
	// Use an uppercase version of the path for the cache key
	UString cacheKey = path.toUpper();
	sp<Image> img = this->imageCache[cacheKey].lock();
	if (img)
	{
		return img;
	}

	// Only trace stuff that misses the cache
	TRACE_FN_ARGS1("path", path);
	if (path.substr(0, 4) == "RAW:")
	{
		auto splitString = path.split(':');
		// RAW:PATH:WIDTH:HEIGHT
		if (splitString.size() != 4 && splitString.size() != 5)
		{
			LogError("Invalid RAW resource string: \"%s\"", path.c_str());
			return nullptr;
		}

		auto pImg =
		    RawImage::load(*this, splitString[1], Vec2<int>{Strings::ToInteger(splitString[2]),
		                                                    Strings::ToInteger(splitString[3])});
		if (!pImg)
		{
			LogError("Failed to load RAW image: \"%s\"", path.c_str());
			return nullptr;
		}
		if (splitString.size() == 5)
		{
			auto pal = this->load_palette(splitString[4]);
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
			img = loader->loadImage(fs.getCorrectCaseFilename(path));
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

	this->imageCache[cacheKey] = img;
	img->sourcePath = path;
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

	sp<RGBImage> img = std::dynamic_pointer_cast<RGBImage>(this->load_image(path));
	if (img)
	{
		unsigned int idx = 0;
		auto p = std::make_shared<Palette>(img->size.x * img->size.y);
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

}; // namespace OpenApoc

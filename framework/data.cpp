#include "framework/logger.h"
#include "framework/data.h"
#include "game/apocresources/pck.h"
#include "game/apocresources/apocpalette.h"
#include "framework/palette.h"
#include "framework/ignorecase.h"
#include "library/strings.h"

#include "framework/imageloader_interface.h"
#include "framework/musicloader_interface.h"
#include "framework/sampleloader_interface.h"

using namespace OpenApoc;

namespace {
	std::map<UString, std::unique_ptr<OpenApoc::ImageLoaderFactory>> *registeredImageBackends = nullptr;
	std::map<UString, std::unique_ptr<OpenApoc::MusicLoaderFactory>> *registeredMusicLoaders = nullptr;
	std::map<UString, std::unique_ptr<OpenApoc::SampleLoaderFactory>> *registeredSampleLoaders = nullptr;
}; //anonymous namespace

namespace OpenApoc {

void registerImageLoader(ImageLoaderFactory* factory, UString name)
{
	if (!registeredImageBackends)
	{
		registeredImageBackends = new std::map<UString, std::unique_ptr<OpenApoc::ImageLoaderFactory>>();
	}
	registeredImageBackends->emplace(name, std::unique_ptr<ImageLoaderFactory>(factory));
}
void registerSampleLoader(SampleLoaderFactory *factory, UString name)
{
	if (!registeredSampleLoaders)
	{
		registeredSampleLoaders = new std::map<UString, std::unique_ptr<OpenApoc::SampleLoaderFactory>>();
	}
	registeredSampleLoaders->emplace(name, std::unique_ptr<SampleLoaderFactory>(factory));
}
void registerMusicLoader(MusicLoaderFactory *factory, UString name)
{
	if (!registeredMusicLoaders)
	{
		registeredMusicLoaders = new std::map<UString, std::unique_ptr<OpenApoc::MusicLoaderFactory>>();
	}
	registeredMusicLoaders->emplace(name, std::unique_ptr<MusicLoaderFactory>(factory));
}

Data::Data(Framework &fw, std::vector<UString> paths, int imageCacheSize, int imageSetCacheSize)
{
	for (auto &imageBackend : *registeredImageBackends)
	{
		auto t = imageBackend.first;
		ImageLoader *l = imageBackend.second->create();
		if (l)
		{
			this->imageLoaders.emplace_back(l);
			LogInfo("Initialised image loader %s", t.str().c_str());
		}
		else
			LogWarning("Failed to load image loader %s", t.str().c_str());
	}

	for (auto &sampleBackend : *registeredSampleLoaders)
	{
		auto t = sampleBackend.first;
		SampleLoader *s = sampleBackend.second->create(fw);
		if (s)
		{
			this->sampleLoaders.emplace_back(s);
			LogInfo("Initialised sample loader %s", t.str().c_str());
		}
		else
			LogWarning("Failed to load sample loader %s", t.str().c_str());
	}

	for (auto &musicLoader : *registeredMusicLoaders)
	{
		auto t = musicLoader.first;
		MusicLoader *m = musicLoader.second->create(fw);
		if (m)
		{
			this->musicLoaders.emplace_back(m);
			LogInfo("Initialised music loader %s", t.str().c_str());
		}
		else
			LogWarning("Failed to load music loader %s", t.str().c_str());
	}
	this->writeDir = PHYSFS_getPrefDir(PROGRAM_ORGANISATION, PROGRAM_NAME);
	LogInfo("Setting write directory to \"%s\"", this->writeDir.str().c_str());
	PHYSFS_setWriteDir(this->writeDir.str().c_str());
	for (int i = 0; i < imageCacheSize; i++)
		pinnedImages.push(nullptr);
	for (int i = 0; i < imageSetCacheSize; i++)
		pinnedImageSets.push(nullptr);

	//Paths are supplied in inverse-search order (IE the last in 'paths' should be the first searched)
	for(auto &p : paths)
	{
		if (!PHYSFS_mount(p.str().c_str(), "/", 0))
		{
			LogWarning("Failed to add resource dir \"%s\"", p.str().c_str());
			continue;
		}
		else
			LogInfo("Resource dir \"%s\" mounted to \"%s\"", p.str().c_str(), PHYSFS_getMountPoint(p.str().c_str()));
	}
	//Finally, the write directory trumps all
	PHYSFS_mount(this->writeDir.str().c_str(), "/", 0);
}

Data::~Data()
{

}

std::shared_ptr<ImageSet>
Data::load_image_set(UString path)
{
	UString cacheKey = path.toUpper();
	std::shared_ptr<ImageSet> imgSet = this->imageSetCache[cacheKey].lock();
	if (imgSet)
	{
		return imgSet;
	}
	//PCK resources come in the format:
	//"PCK:PCKFILE:TABFILE[:optional/ignored]"
	if (path.substr(0, 4) == "PCK:")
	{
		auto splitString = path.split(':');
		imgSet = PCKLoader::load(*this, splitString[1], splitString[2]);
	}
	else
	{
		LogError("Unknown image set format \"%s\"", path.str().c_str());
		return nullptr;
	}

	this->pinnedImageSets.push(imgSet);
	this->pinnedImageSets.pop();

	this->imageSetCache[cacheKey] = imgSet;
	return imgSet;
}

std::shared_ptr<Sample>
Data::load_sample(UString path)
{
	UString cacheKey = path.toUpper();
	std::shared_ptr<Sample> sample = this->sampleCache[cacheKey].lock();
	if (sample)
		return sample;

	for (auto &loader : this->sampleLoaders)
	{
		sample = loader->loadSample(path);
		if (sample)
			break;
	}
	if (!sample)
	{
		LogInfo("Failed to load sample \"%s\"", path.str().c_str());
		return nullptr;
	}
	this->sampleCache[cacheKey] = sample;
	return sample;
}

std::shared_ptr<MusicTrack>
Data::load_music(UString path)
{
	//No cache for music tracks, just stream of disk
	for (auto &loader : this->musicLoaders)
	{
		auto track = loader->loadMusic(path);
		if (track)
			return track;
	}
	LogInfo("Failed to load music track \"%s\"", path.str().c_str());
	return nullptr;
}

std::shared_ptr<Image>
Data::load_image(UString path)
{
	//Use an uppercase version of the path for the cache key
	UString cacheKey = path.toUpper();
	std::shared_ptr<Image> img = this->imageCache[cacheKey].lock();
	if (img)
	{
		return img;
	}


	if (path.substr(0,4) == "PCK:")
	{
		auto splitString = path.split(':');
		auto imageSet = this->load_image_set(splitString[0] + ":" + splitString[1] + ":" + splitString[2]);
		if (!imageSet)
		{
			return nullptr;
		}
		//PCK resources come in the format:
		//"PCK:PCKFILE:TABFILE:INDEX"
		//or
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
				std::shared_ptr<PaletteImage> pImg = 
					std::dynamic_pointer_cast<PaletteImage>(
						this->load_image("PCK:" + splitString[1] + ":" + splitString[2] + ":" + splitString[3]));
				assert(pImg);
				auto pal = this->load_palette(splitString[4]);
				assert(pal);
				img = pImg->toRGBImage(pal);
				break;
			}
			default:
				LogError("Invalid PCK resource string \"%s\"", path.str().c_str());
				return nullptr;
		}
	}
	else
	{
		for (auto &loader : imageLoaders)
		{
			img = loader->loadImage(GetCorrectCaseFilename(path));
			if (img)
			{
				break;
			}
		}
		if (!img)
		{
			LogInfo("Failed to load image \"%s\"", path.str().c_str());
			return nullptr;
		}
	}

	this->pinnedImages.push(img);
	this->pinnedImages.pop();

	this->imageCache[cacheKey] = img;
	return img;
}

PHYSFS_file* Data::load_file(UString path, Data::FileMode mode)
{
	//FIXME: read/write/append modes
	if (mode != Data::FileMode::Read)
	{
		LogError("Non-readonly modes not yet supported");
	}
	UString foundPath = GetCorrectCaseFilename(path);
	if (foundPath == "")
	{
		LogInfo("Failed to open file \"%s\" : \"%s\"", path.str().c_str(), PHYSFS_getLastError());
		return nullptr;
	}
	PHYSFS_File *f = PHYSFS_openRead(foundPath.str().c_str());
	if (!f)
	{
		LogError("Failed to open file \"%s\" despite being 'found' at \"%s\"? - error: \"%s\"", path.str().c_str(), foundPath.str().c_str(), PHYSFS_getLastError());
		return nullptr;
	}
	return f;
}

std::shared_ptr<Palette>
Data::load_palette(UString path)
{
	std::shared_ptr<RGBImage> img = std::dynamic_pointer_cast<RGBImage>(this->load_image(path));
	if (img)
	{
		unsigned int idx = 0;
		auto p = std::make_shared<Palette>(img->size.x * img->size.y);
		RGBImageLock src{img, ImageLockUse::Read};
		for (unsigned int y = 0; y < img->size.y; y++)
		{
			for (unsigned int x = 0; x < img->size.x; x++)
			{
				Colour c = src.get(Vec2<int>{x,y});
				p->SetColour(idx, c);
				idx++;
			}
		}
		return p;
	}
	else
	{
		std::shared_ptr<Palette> pal(loadApocPalette(*this, path));
		if (!pal)
		{
			LogError("Failed to open palette \"%s\"", path.str().c_str());
			return nullptr;
		}
		return pal;
	}
}

UString
Data::GetCorrectCaseFilename(UString Filename)
{
	std::string u8Filename = Filename.str();
	std::unique_ptr<char[]> buf(new char[u8Filename.length() + 1]);
	strncpy(buf.get(), u8Filename.c_str(), u8Filename.length());
	buf[Filename.length()] = '\0';
	if (PHYSFSEXT_locateCorrectCase(buf.get()))
	{
		LogInfo("Failed to find file \"%s\"", Filename.str().c_str());
		return "";
	}
	return buf.get();
}

UString
Data::GetActualFilename(UString Filename)
{
	UString foundPath = GetCorrectCaseFilename(Filename);
	if (foundPath == "")
	{
		LogError("Failed to get filename for \"%s\"", Filename.str().c_str());
		return "";
	}
	UString folder = PHYSFS_getRealDir(foundPath.str().c_str());
	return folder + "/" + foundPath;
}

}; //namespace OpenApoc

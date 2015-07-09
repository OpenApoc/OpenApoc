#include "framework/logger.h"
#include "framework/data.h"
#include "game/apocresources/pck.h"
#include "game/apocresources/rawimage.h"
#include "game/apocresources/apocpalette.h"
#include "framework/palette.h"
#include "framework/ignorecase.h"
#include "library/strings.h"

#include "framework/imageloader_interface.h"
#include "framework/musicloader_interface.h"
#include "framework/sampleloader_interface.h"

#include <physfs.h>

#ifndef _WIN32
#include <endian.h>
#else
/* Windows is always little endian? */
static inline uint16_t le16toh(uint16_t val)
{
	return val;
}
static inline uint32_t le32toh(uint32_t val)
{
	return val;
}
#endif

using namespace OpenApoc;

namespace {
	std::map<UString, std::unique_ptr<OpenApoc::ImageLoaderFactory>> *registeredImageBackends = nullptr;
	std::map<UString, std::unique_ptr<OpenApoc::MusicLoaderFactory>> *registeredMusicLoaders = nullptr;
	std::map<UString, std::unique_ptr<OpenApoc::SampleLoaderFactory>> *registeredSampleLoaders = nullptr;

static UString GetCorrectCaseFilename(const UString& Filename)
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

class PhysfsIFileImpl : public std::streambuf, public IFileImpl
{
public:
	size_t bufferSize;
	std::unique_ptr<char[]> buffer;
	UString systemPath;
	UString caseCorrectedPath;
	UString suppliedPath;

	PHYSFS_File *file;

	PhysfsIFileImpl(const UString &path, const UString &suppliedPath, size_t bufferSize = 512)
		: bufferSize(bufferSize), buffer(new char[bufferSize]), suppliedPath(suppliedPath)
	{
		file = PHYSFS_openRead(path.str().c_str());
		if (!file)
		{
			LogError("Failed to open file \"%s\" : \"%s\"", path.str().c_str(), PHYSFS_getLastError());
			return;
		}
		systemPath = PHYSFS_getRealDir(path.str().c_str());
		systemPath += "/" + path;

	}
	virtual ~PhysfsIFileImpl()
	{
		if (file)
			PHYSFS_close(file);
	}

	int_type underflow()
	{
		if (PHYSFS_eof(file))
		{
			return traits_type::eof();
		}
		size_t bytesRead = PHYSFS_readBytes(file, buffer.get(), bufferSize);
		if (bytesRead < 1)
		{
			return traits_type::eof();
		}
		setg(buffer.get(), buffer.get(), buffer.get() + bytesRead);
		return (unsigned char) *gptr();
	}

	pos_type seekoff(off_type pos, std::ios_base::seekdir dir, std::ios_base::openmode mode)
	{
		switch (dir)
		{
			case std::ios_base::beg:
				PHYSFS_seek(file, pos);
				break;
			case std::ios_base::cur:
				PHYSFS_seek(file, (PHYSFS_tell(file) + pos) - (egptr() - gptr()));
				break;
			case std::ios_base::end:
				PHYSFS_seek(file, PHYSFS_fileLength(file) + pos);
				break;
			default:
				LogError("Unknown direction in seekoff (%d)", dir);
				assert(0);
		}

		if (mode & std::ios_base::in)
		{
			setg(egptr(), egptr(), egptr());
		}

		if (mode & std::ios_base::out)
		{
			LogError("ios::out set on read-only IFile \"%s\"", this->suppliedPath.str().c_str());
			assert(0);
			setp(buffer.get(), buffer.get());
		}

		return PHYSFS_tell(file);
	}

	pos_type seekpos(pos_type pos, std::ios_base::openmode mode)
	{
		PHYSFS_seek(file, pos);
		if (mode & std::ios_base::in)
		{
			setg(egptr(), egptr(), egptr());
		}

		if (mode & std::ios_base::out)
		{
			LogError("ios::out set on read-only IFile \"%s\"", this->suppliedPath.str().c_str());
			assert(0);
			setp(buffer.get(), buffer.get());
		}

		return PHYSFS_tell(file);
	}

	int_type overflow(int_type c = traits_type::eof())
	{
		LogError("overflow called on read-only IFile \"%s\"", this->suppliedPath.str().c_str());
		assert(0);
		std::ignore = c;
		return 0;
	}
};

}; //anonymous namespace

namespace OpenApoc {

IFile::IFile()
	: std::istream(nullptr)
{

}
//FIXME: MSVC needs this, GCC fails with it?
#ifdef _WIN32
IFile::IFile(IFile&& other)
	: std::istream(std::move(other))
{
	this->f = std::move(other.f);
	rdbuf(other.rdbuf());
	other.rdbuf(nullptr);
}
#endif

IFileImpl::~IFileImpl()
{
}

bool
IFile::readule16(uint16_t &val)
{
	this->read((char*)&val, sizeof(val));
	val = le16toh(val);
	return !!(*this);
}

bool
IFile::readule32(uint32_t &val)
{
	this->read((char*)&val, sizeof(val));
	val = le32toh(val);
	return !!(*this);
}

size_t
IFile::size() const
{
	if (this->f)
		return PHYSFS_fileLength(dynamic_cast<PhysfsIFileImpl*>(f.get())->file);
	return 0;
}

const UString&
IFile::fileName() const
{
	static const UString emptyString = "";
	if (this->f)
		return dynamic_cast<PhysfsIFileImpl*>(f.get())->suppliedPath;
	return emptyString;
}

const UString&
IFile::systemPath() const
{
	static const UString emptyString = "";
	if (this->f)
		return dynamic_cast<PhysfsIFileImpl*>(f.get())->systemPath;
	return emptyString;
}

IFile::~IFile()
{

}

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
Data::load_image_set(const UString& path)
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
Data::load_sample(const UString& path)
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
Data::load_music(const UString& path)
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
Data::load_image(const UString& path)
{
	//Use an uppercase version of the path for the cache key
	UString cacheKey = path.toUpper();
	std::shared_ptr<Image> img = this->imageCache[cacheKey].lock();
	if (img)
	{
		return img;
	}


	if (path.substr(0,4) == "RAW:")
	{
		auto splitString = path.split(':');
		// RAW:PATH:WIDTH:HEIGHT
		if (splitString.size() != 4 && splitString.size() != 5)
		{
			LogError("Invalid RAW resource string: \"%s\"",
				path.str().c_str());
			return nullptr;
		}

		auto pImg = RawImage::load(*this, splitString[1],
			Vec2<int>{Strings::ToInteger(splitString[2]), Strings::ToInteger(splitString[3])});
		if (!pImg)
		{
			LogError("Failed to load RAW image: \"%s\"",
				path.str().c_str());
			return nullptr;
		}
		if (splitString.size() == 5)
		{
			auto pal = this->load_palette(splitString[4]);
			if (!pal)
			{
				LogError("Failed to load palette for RAW image: \"%s\"",
					path.str().c_str());
				return nullptr;
			}
			img = pImg->toRGBImage(pal);

		}
		else
		{
			img = pImg;
		}

	}
	else if (path.substr(0,4) == "PCK:")
	{
		auto splitString = path.split(':');
		if (splitString.size() != 3 && splitString.size() != 4 && splitString.size() != 5)
		{
			LogError("Invalid PCK resource string: \"%s\"",
				path.str().c_str());
			return nullptr;
		}
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

IFile Data::load_file(const UString& path, Data::FileMode mode)
{
	IFile f;
	if (mode != Data::FileMode::Read)
	{
		LogError("Invalid FileMode set for \"%s\"", path.str().c_str());
		return f;
	}
	UString foundPath = GetCorrectCaseFilename(path);
	if (foundPath == "")
	{
		LogInfo("Failed to find \"%s\"", path.str().c_str());
		assert(!f);
		return f;
	}
	f.f.reset(new PhysfsIFileImpl(foundPath, path));
	f.rdbuf(dynamic_cast<PhysfsIFileImpl*>(f.f.get()));
	LogInfo("Loading \"%s\" from \"%s\"", path.str().c_str(), f.systemPath().str().c_str());
	return f;
}

std::shared_ptr<Palette>
Data::load_palette(const UString& path)
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


}; //namespace OpenApoc

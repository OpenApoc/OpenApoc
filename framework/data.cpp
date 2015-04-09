#include "data.h"
#include "game/apocresources/pck.h"
#include "game/apocresources/apocpalette.h"
#include "palette.h"
#include "ignorecase.h"

namespace OpenApoc {

Data::Data(const char *programName, std::vector<std::string> paths, int imageCacheSize, int imageSetCacheSize) :
	imageLoader(createImageLoader())
{
	PHYSFS_init(programName);
	std::cerr << "PHYSFS_init(" << programName << ")\n";
	this->writeDir = PHYSFS_getPrefDir(PROGRAM_ORGANISATION, PROGRAM_NAME);
	std::cerr << "Setting write dir to \"" << this->writeDir << "\"\n";
	PHYSFS_setWriteDir(this->writeDir.c_str());
	for (int i = 0; i < imageCacheSize; i++)
		pinnedImages.push(nullptr);
	for (int i = 0; i < imageSetCacheSize; i++)
		pinnedImageSets.push(nullptr);
	
	//Paths are supplied in inverse-search order (IE the last in 'paths' should be the first searched)
	for(auto &p : paths)
	{
		std::cerr << "Appending resouce dir \"" << p << "\"\n";
		if (!PHYSFS_mount(p.c_str(), "/", 0))
			std::cerr << "Failed to add resource dir!\n";
		std::cerr << "Resource dir mounted to \"" << PHYSFS_getMountPoint(p.c_str()) << "\"\n";
	}
	//Finally, the write directory trumps all
	PHYSFS_mount(writeDir.c_str(), "/", 0);
}

Data::~Data()
{
	PHYSFS_deinit();

}

std::shared_ptr<ImageSet>
Data::load_image_set(const std::string path)
{
	std::string cacheKey = Strings::ToUpper(path);
	std::shared_ptr<ImageSet> imgSet = this->imageSetCache[cacheKey].lock();
	if (imgSet)
	{
		return imgSet;
	}
	//PCK resources come in the format:
	//"PCK:PCKFILE:TABFILE[:optional/ignored]"
	if (path.substr(0, 4) == "PCK:")
	{
		std::vector<std::string> splitString = Strings::Split(path, ':');
		imgSet = PCKLoader::load(*this, splitString[1], splitString[2]);
	}
	else
	{
		std::cerr << "Unknown image set format \"" << path << "\"\n";
		return nullptr;
	}

	this->pinnedImageSets.push(imgSet);
	this->pinnedImageSets.pop();

	this->imageSetCache[cacheKey] = imgSet;
	return imgSet;
}

std::shared_ptr<Image>
Data::load_image(const std::string path)
{
	//Use an uppercase version of the path for the cache key
	std::string cacheKey = Strings::ToUpper(path);
	std::shared_ptr<Image> img = this->imageCache[cacheKey].lock();
	if (img)
	{
		return img;
	}


	if (path.substr(0,4) == "PCK:")
	{
		std::vector<std::string> splitString = Strings::Split(path, ':');
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
				std::cerr << "Invalid PCK resource string \"" << path << "\"\n";
				return nullptr;
		}
	}
	else
	{
		img = imageLoader->loadImage(GetCorrectCaseFilename(path));
		if (!img)
		{
			std::cerr << "Failed to load image \"" << path << "\"\n";
			return nullptr;
		}
	}

	this->pinnedImages.push(img);
	this->pinnedImages.pop();

	this->imageCache[cacheKey] = img;
	return img;
}

PHYSFS_file* Data::load_file(const std::string path, const char *mode)
{
	//FIXME: read/write/append modes
	std::string foundPath = GetCorrectCaseFilename(path);
	if (foundPath == "")
	{
		std::cerr << "Failed to open file \"" + path +"\"\n";
		std::cerr << PHYSFS_getLastError() << "\n";
		return nullptr;
	}
	return PHYSFS_openRead(foundPath.c_str());
}

std::shared_ptr<Palette>
Data::load_palette(const std::string path)
{
	std::shared_ptr<RGBImage> img = std::dynamic_pointer_cast<RGBImage>(this->load_image(path));
	if (img)
	{
		int idx = 0;
		auto p = std::make_shared<Palette>(img->size.x * img->size.y);
		RGBImageLock src{img, ImageLockUse::Read};
		for (int y = 0; y < img->size.y; y++)
		{
			for (int x = 0; x < img->size.x; x++)
			{
				Colour c = src.get(Vec2<int>{x,y});
				p->SetColour(idx, c);
				idx++;
			}
		}
		return p;
	}
	return std::shared_ptr<Palette>(loadApocPalette(*this, path));
}

std::string
Data::GetCorrectCaseFilename(std::string Filename)
{
	std::unique_ptr<char[]> buf(new char[Filename.length() + 1]);
	strncpy(buf.get(), Filename.c_str(), Filename.length());
	buf[Filename.length()] = '\0';
	if (PHYSFSEXT_locateCorrectCase(buf.get()))
	{
		std::cerr << "Failed to find file \"" << Filename << "\n";
		return "";
	}
	return std::string(buf.get());
}

std::string
Data::GetActualFilename(std::string Filename)
{
	std::string foundPath = GetCorrectCaseFilename(Filename);
	std::string folder = PHYSFS_getRealDir(foundPath.c_str());
	return folder + "/" + foundPath;
}

}; //namespace OpenApoc

#include "data.h"
#include "game/apocresources/pck.h"
#include "game/apocresources/apocpalette.h"
#include "palette.h"

namespace OpenApoc {

Data::Data(const std::string root) :
	root(root), DIR_SEP('/'), imageLoader(createImageLoader())
{
}

Data::~Data()
{

}

std::shared_ptr<Image>
Data::load_image(const std::string path)
{
	//Use an uppercase version of the path for the cache key
	std::string cacheKey = Strings::ToUpper(path);
	std::shared_ptr<Image> img = this->imageCache[cacheKey].lock();
	if (img)
		return img;

	if (path.substr(0,4) == "PCK:")
	{
		//PCK resources come in the format:
		//"PCK:PCKFILE:TABFILE:INDEX"
		//or
		//"PCK:PCKFILE:TABFILE:INDEX:PALETTE" if we want them already in rgb space
		std::vector<std::string> splitString = Strings::Split(path, ':');
		switch (splitString.size())
		{
			case 4:
			{
				img = PCK(*this, splitString[1], splitString[2], Strings::ToInteger(splitString[3])).GetImage(0);
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
		std::string fullPath = this->GetActualFilename(path);
		if (fullPath == "")
		{
			std::cerr << "Failed to find image \"" << path << "\"\n";
			return nullptr;
		}
		Image* bmp = imageLoader->loadImage(fullPath);
		if (!bmp)
		{
			std::cerr << "Failed to load image \"" << fullPath << "\"\n";
			return nullptr;
		}
		img.reset(bmp);
	}

	this->imageCache[cacheKey] = img;
	return img;
}

ALLEGRO_FILE* Data::load_file(const std::string path, const char *mode)
{
	std::string fullPath = this->GetActualFilename(path);
	if (fullPath == "")
	{
		std::cerr << "Failed to find file \"" + path +"\"\n";
		return nullptr;
	}
	ALLEGRO_FILE *file = al_fopen(fullPath.c_str(), mode);
	if (file == nullptr)
	{
		std::cerr << "Failed to open file \"" + fullPath +"\"\n";
		return nullptr;
	}
	return file;
}

#ifdef CASE_SENSITIVE_FILESYSTEM
#include <dirent.h>

class Directory
{
private:
public:
	DIR *d;
	explicit Directory(const std::string &path)
	{
		d = opendir(path.c_str());
	}
	~Directory()
	{
		if (d)
			closedir(d);
	}

	//Disallow copy
	Directory(const Directory&) = delete;
	//Allow move
	Directory(Directory&&) = default;
};

std::string findInDir(std::string parent, std::list<std::string> remainingPath, const char DIR_SEP)
{
	Directory d(parent);
	std::string name = remainingPath.front();
	remainingPath.pop_front();
	//Strip out any excess blank entries (otherwise paths like "./directory//file" would break)
	while (name == "")
	{
		if (remainingPath.empty())
			return parent;
		name = remainingPath.front();
		remainingPath.pop_front();
	}
	struct dirent entry , *result = NULL;
	while (true)
	{
		int err = readdir_r(d.d, &entry, &result);
		if (err)
		{
			std::cerr << "Readdir() failed with \"" << err << "\"\n";
			return "";
		}
		if (!result)
		{
			break;
		}
		std::string entName(entry.d_name);
		if (Strings::CompareCaseInsensitive(name, entName) == 0)
		{
			std::string entPath = parent + DIR_SEP + entName;
			if (remainingPath.empty())
				return entPath;
			else
				return findInDir(entPath, remainingPath, DIR_SEP);
		}
	}
	return "";
}

static std::string GetCaseInsensitiveFilename(const std::string fileName, const char DIR_SEP)
{
	auto splitPaths = Strings::SplitList(fileName, DIR_SEP);
	if (splitPaths.empty())
		return "";
	std::string root = splitPaths.front();
	splitPaths.pop_front();
	std::string path = findInDir(root, Strings::SplitList(fileName, DIR_SEP), DIR_SEP);
	return path;
}
#endif

std::string Data::GetActualFilename( std::string Filename )
{
#ifdef CASE_SENSITIVE_FILESYSTEM
	return GetCaseInsensitiveFilename(this->root + this->DIR_SEP + Filename, this->DIR_SEP);
#else
	return this->root + this->DIR_SEP + Filename;
#endif
}

std::shared_ptr<Palette>
Data::load_palette(const std::string path)
{
	return std::shared_ptr<Palette>(loadApocPalette(*this, path));
}

}; //namespace OpenApoc

#include "data.h"

namespace OpenApoc {

Data *Data::data = nullptr;

Data::Data(const std::string root) :
	root(root), DIR_SEP('/')
{
}

Data::~Data()
{

}

std::shared_ptr<Image>
Data::load_image(const std::string path)
{
	std::string fullPath = this->GetActualFilename(path);
	if (fullPath == "")
	{
		std::cerr << "Failed to find image \"" << path << "\"\n";
		return nullptr;
	}
	//Use an uppercase version of the path for the cache key
	std::string fullPathUpper = Strings::ToUpper(fullPath);
	std::shared_ptr<Image> img = this->imageCache[fullPathUpper].lock();
	if (img)
		return img;

	ALLEGRO_BITMAP *bmp = al_load_bitmap(fullPath.c_str());
	if (!bmp)
	{
		std::cerr << "Failed to load image \"" << fullPath << "\"\n";
		return nullptr;
	}
	img.reset(new Image(bmp));

	this->imageCache[fullPathUpper] = img;
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

}; //namespace OpenApoc

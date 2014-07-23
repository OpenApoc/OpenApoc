#include "data.h"

#include <cassert>
#include <iostream>
#include <algorithm>

Data *Data::data = nullptr;

Data::Data(const std::wstring root) :
	root(root)
{
	DIR_SEP = "/";
}

Data::~Data()
{

}

ALLEGRO_BITMAP* Data::load_bitmap(const std::wstring path)
{
	std::wstring fullpath = std::wstring(this->root + this->DIR_SEP + path);
	ALLEGRO_BITMAP* bitmap = al_load_bitmap(fullpath.c_str());
	if (bitmap != nullptr)
	{
		return bitmap;
	}

	/* Try lowercase version: */
	std::wstring lowerpath = path;
	std::transform(lowerpath.begin(), lowerpath.end(), lowerpath.begin(),  ::tolower);
	fullpath = std::wstring(this->root + this->DIR_SEP + lowerpath);
	std::cerr << "Failed to load \"" + path + "\", trying \"" + lowerpath + "\"\n";
	bitmap = al_load_bitmap(fullpath.c_str());
	if (bitmap != nullptr)
	{
		return bitmap;
	}

	std::cerr << "Failed to load \"" + path + "\"\n";
	assert(0);
	return nullptr;
}

ALLEGRO_FILE* Data::load_file(const std::wstring path, const char *mode)
{
	std::wstring fullpath = std::wstring(this->root + this->DIR_SEP + path);
	ALLEGRO_FILE* file = al_fopen(fullpath.c_str(), mode);
	if (file != nullptr)
	{
		return file;
	}

	/* Try lowercase version: */
	std::wstring lowerpath = path;
	std::transform(lowerpath.begin(), lowerpath.end(), lowerpath.begin(),  ::tolower);
	fullpath = std::wstring(this->root + this->DIR_SEP + lowerpath);
	std::cerr << "Failed to load \"" + path + "\", trying \"" + lowerpath + "\"\n";
	file = al_fopen(fullpath.c_str(), mode);
	if (file != nullptr)
	{
		return file;
	}

	std::cerr << "Failed to load \"" + path + "\"\n";
	assert(0);

	return nullptr;
}

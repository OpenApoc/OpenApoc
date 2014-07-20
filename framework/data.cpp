#include "data.h"

#include <cassert>
#include <iostream>
#include <algorithm>

Data *Data::data = nullptr;

Data::Data(const std::string root) :
	root(root)
{

}

Data::~Data()
{

}

ALLEGRO_BITMAP *
Data::load_bitmap(const std::string path)
{
	std::string fullpath = std::string(this->root + this->DIR_SEP + path);
	auto bitmap = al_load_bitmap(fullpath.c_str());
	if (bitmap)
		return bitmap;

	/* Try lowercase version: */
	auto lowerpath = path;
	std::transform(lowerpath.begin(), lowerpath.end(), lowerpath.begin(),  ::tolower);
	fullpath = std::string(this->root + this->DIR_SEP + lowerpath);
	std::cerr << "Failed to load \"" + path + "\", trying \"" + lowerpath + "\"\n";
	bitmap = al_load_bitmap(fullpath.c_str());
	if (bitmap)
		return bitmap;

	std::cerr << "Failed to load \"" + path + "\"\n";
	assert(0);
	return nullptr;
}

ALLEGRO_FILE *
Data::load_file(const std::string path, const char *mode)
{
	std::string fullpath = std::string(this->root + this->DIR_SEP + path);
	auto file = al_fopen(fullpath.c_str(), mode);
	if (file)
		return file;

	/* Try lowercase version: */
	auto lowerpath = path;
	std::transform(lowerpath.begin(), lowerpath.end(), lowerpath.begin(),  ::tolower);
	fullpath = std::string(this->root + this->DIR_SEP + lowerpath);
	std::cerr << "Failed to load \"" + path + "\", trying \"" + lowerpath + "\"\n";
	file = al_fopen(fullpath.c_str(), mode);
	if (file)
		return file;

	std::cerr << "Failed to load \"" + path + "\"\n";
	assert(0);

	return nullptr;
}

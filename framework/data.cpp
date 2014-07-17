#include "data.h"

#include <cassert>
#include <iostream>

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
	if (!bitmap)
	{
		std::cerr << "Failed to load \"" + fullpath + "\"\n";
		assert(0);
	}
	return bitmap;
}

ALLEGRO_FILE *
Data::load_file(const std::string path, const char *mode)
{
	std::string fullpath = std::string(this->root + this->DIR_SEP + path);
	auto file = al_fopen(fullpath.c_str(), mode);
	if (!file)
	{
		std::cerr << "Failed to load \"" + fullpath + "\"\n";
		assert(0);
	}
	return file;
}

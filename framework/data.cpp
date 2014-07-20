#include "data.h"

#include <cassert>
#include <iostream>
#include <algorithm>

Data *Data::data = 0;

Data::Data(const std::string root) :
	root(root)
{
	DIR_SEP = "/";
}

Data::~Data()
{

}

ALLEGRO_BITMAP* Data::load_bitmap(const std::string path)
{
	std::string fullpath = std::string(this->root + this->DIR_SEP + path);
	ALLEGRO_BITMAP* bitmap = al_load_bitmap(fullpath.c_str());
	if( bitmap != 0 )
	{
		return bitmap;
	}

	/* Try lowercase version: */
	std::string lowerpath = path;
	std::transform(lowerpath.begin(), lowerpath.end(), lowerpath.begin(),  ::tolower);
	fullpath = std::string(this->root + this->DIR_SEP + lowerpath);
	std::cerr << "Failed to load \"" + path + "\", trying \"" + lowerpath + "\"\n";
	bitmap = al_load_bitmap(fullpath.c_str());
	if( bitmap != 0 )
	{
		return bitmap;
	}

	std::cerr << "Failed to load \"" + path + "\"\n";
	assert(0);
	return 0;
}

ALLEGRO_FILE* Data::load_file(const std::string path, const char *mode)
{
	std::string fullpath = std::string(this->root + this->DIR_SEP + path);
	ALLEGRO_FILE* file = al_fopen(fullpath.c_str(), mode);
	if( file != 0 )
	{
		return file;
	}

	/* Try lowercase version: */
	std::string lowerpath = path;
	std::transform(lowerpath.begin(), lowerpath.end(), lowerpath.begin(),  ::tolower);
	fullpath = std::string(this->root + this->DIR_SEP + lowerpath);
	std::cerr << "Failed to load \"" + path + "\", trying \"" + lowerpath + "\"\n";
	file = al_fopen(fullpath.c_str(), mode);
	if( file != 0 )
	{
		return file;
	}

	std::cerr << "Failed to load \"" + path + "\"\n";
	assert(0);

	return 0;
}

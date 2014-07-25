#pragma once

#include "includes.h"

#define DATA Data::data

class Data
{

	private:
		std::string root;
		std::string DIR_SEP;

	public:
		static Data* data;

		Data(const std::string root);
		~Data();

		ALLEGRO_BITMAP* load_bitmap(const std::string path);
		ALLEGRO_FILE* load_file(const std::string path, const char *mode);

		std::string GetActualFilename(std::string Filename);

};

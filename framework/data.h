#pragma once

#include "includes.h"

#define DATA Data::data

class Data
{

	private:
		std::wstring root;
		std::wstring DIR_SEP;

	public:
		static Data* data;

		Data(const std::wstring root);
		~Data();

		ALLEGRO_BITMAP* load_bitmap(const std::wstring path);
		ALLEGRO_FILE* load_file(const std::wstring path, const char *mode);

};

#pragma once

#include "includes.h"
#include "image.h"

namespace OpenApoc {

class Data
{

	private:
		std::string root;
		const char DIR_SEP;

		std::map<std::string, std::weak_ptr<Image> >imageCache;

	public:
		Data(const std::string root);
		~Data();

		std::shared_ptr<Image> load_image(const std::string path);
		ALLEGRO_FILE* load_file(const std::string path, const char *mode);

		std::string GetActualFilename(std::string Filename);
};

}; //namspace OpenApoc

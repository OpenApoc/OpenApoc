#pragma once
#include "image.h"
#include <string>

namespace OpenApoc {

	class ImageLoader
	{
	public:
		virtual ~ImageLoader() {};
		virtual std::shared_ptr<Image> loadImage(std::string path) = 0;
		virtual std::string getName() = 0;
	};

	class ImageLoaderFactory
	{
	public:
		virtual ImageLoader *create() = 0;
		virtual ~ImageLoaderFactory() {};
	};
	
	void registerImageLoader(ImageLoaderFactory* factory, std::string name);
	template <typename T>
	class ImageLoaderRegister
	{
	public:
		ImageLoaderRegister(std::string name)
		{
			registerImageLoader(new T, name);
		}
	};

};

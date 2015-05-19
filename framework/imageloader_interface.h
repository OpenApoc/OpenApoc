#pragma once
#include "image.h"
#include "library/strings.h"

namespace OpenApoc {

	class ImageLoader
	{
	public:
		virtual ~ImageLoader() {};
		virtual std::shared_ptr<Image> loadImage(UString path) = 0;
		virtual UString getName() = 0;
	};

	class ImageLoaderFactory
	{
	public:
		virtual ImageLoader *create() = 0;
		virtual ~ImageLoaderFactory() {};
	};
	
	void registerImageLoader(ImageLoaderFactory* factory, UString name);
	template <typename T>
	class ImageLoaderRegister
	{
	public:
		ImageLoaderRegister(UString name)
		{
			registerImageLoader(new T, name);
		}
	};

};

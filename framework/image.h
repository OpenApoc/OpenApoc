#pragma once

#include "includes.h"

namespace OpenApoc {

class Palette;
class RGBImage;
class RendererImageData;
class ImageSet;

enum class ImageLockUse
{
	Read,
	Write,
	ReadWrite,
};

class Image
{
	protected:
		Image(Vec2<int> size);
	public:
		virtual ~Image();
		Vec2<int> size;

		std::unique_ptr<RendererImageData> rendererPrivateData;
		bool dirty;

		std::weak_ptr<ImageSet> owningSet;
		unsigned indexInSet;
};

class ImageLoader
{
public:
	virtual ~ImageLoader();
	virtual std::shared_ptr<Image> loadImage(std::string path) = 0;
};

ImageLoader* createImageLoader();

//A surface is an image you can render to. No SW locking is allowed!
class Surface : public Image
{
public:
	Surface(Vec2<int> size);
	virtual ~Surface();
};

class PaletteImage : public Image
{
	private:
		friend class PaletteImageLock;
		std::unique_ptr<uint8_t[]> indices;
	public:
		PaletteImage(Vec2<int> size, uint8_t initialIndex = 0);
		~PaletteImage();
		std::shared_ptr<RGBImage> toRGBImage(std::shared_ptr<Palette> p);
};

class PaletteImageLock
{
	private:
		std::shared_ptr<PaletteImage> img;
		//Disallow copy
		PaletteImageLock(const PaletteImageLock &) = delete;
		ImageLockUse use;
	public:
		PaletteImageLock(std::shared_ptr<PaletteImage> img, ImageLockUse use = ImageLockUse::Write);
		~PaletteImageLock();
		uint8_t get(Vec2<int> pos);
		void set(Vec2<int> pos, uint8_t idx);

		//FIXME: Magic backdoor to the index data
		void *getData();
};

class RGBImage : public Image
{
	private:
		friend class RGBImageLock;
		std::unique_ptr<Colour[]> pixels;
	public:
		RGBImage(Vec2<int> size, Colour initialColour = Colour(0,0,0,0));
		~RGBImage();
		void saveBitmap(const std::string &filename);
};

class RGBImageLock
{
	private:
		std::shared_ptr<RGBImage> img;
		//Disallow copy
		RGBImageLock(const RGBImageLock &) = delete;
		ImageLockUse use;
	public:
		RGBImageLock(std::shared_ptr<RGBImage> img, ImageLockUse use = ImageLockUse::Write);
		~RGBImageLock();
		Colour get(Vec2<int> pos);
		void set(Vec2<int> pos, Colour &c);

		//FIXME: Magic backdoor to the RGBA data
		void *getData();
};

class ImageSet
{
public:
	std::vector<std::shared_ptr<Image> > images;
	Vec2<int> maxSize;

	std::shared_ptr<RendererImageData> rendererPrivateData;
};

}; //namespace OpenApoc

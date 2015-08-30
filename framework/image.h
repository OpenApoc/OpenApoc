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
		Image(Vec2<unsigned int> size);
	public:
		virtual ~Image();
		Vec2<unsigned int> size;

		std::unique_ptr<RendererImageData> rendererPrivateData;
		bool dirty;
		Rect<unsigned int> bounds;

		std::weak_ptr<ImageSet> owningSet;
		unsigned indexInSet;
};

//A surface is an image you can render to. No SW locking is allowed!
class Surface : public Image
{
public:
	Surface(Vec2<unsigned int> size);
	virtual ~Surface();
};

class PaletteImage : public Image
{
	private:
		friend class PaletteImageLock;
		std::unique_ptr<uint8_t[]> indices;
	public:
		PaletteImage(Vec2<unsigned int> size, uint8_t initialIndex = 0);
		~PaletteImage();
		std::shared_ptr<RGBImage> toRGBImage(std::shared_ptr<Palette> p);
		static void blit(std::shared_ptr<PaletteImage> src, Vec2<unsigned int> offset, std::shared_ptr<PaletteImage> dst);

		void CalculateBounds();
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
		uint8_t get(Vec2<unsigned int> pos);
		void set(Vec2<unsigned int> pos, uint8_t idx);

		//FIXME: Magic backdoor to the index data
		void *getData();
};

class RGBImage : public Image
{
	private:
		friend class RGBImageLock;
		std::unique_ptr<Colour[]> pixels;
	public:
		RGBImage(Vec2<unsigned int> size, Colour initialColour = Colour(0,0,0,0));
		~RGBImage();
		void saveBitmap(const UString &filename);
		static void blit(std::shared_ptr<RGBImage> src, Vec2<unsigned int> srcOffset, std::shared_ptr<RGBImage> dst, Vec2<unsigned int> dstOffset);
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
		Colour get(Vec2<unsigned int> pos);
		void set(Vec2<unsigned int> pos, Colour &c);

		//FIXME: Magic backdoor to the RGBA data
		void *getData();
};

class ImageSet
{
public:
	std::vector<std::shared_ptr<Image> > images;
	Vec2<unsigned int> maxSize;

	std::shared_ptr<RendererImageData> rendererPrivateData;
};

}; //namespace OpenApoc

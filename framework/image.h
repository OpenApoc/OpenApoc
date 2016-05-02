#pragma once
#include "library/colour.h"
#include "library/rect.h"
#include "library/resource.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

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

class Image : public ResObject
{
  protected:
	Image(Vec2<unsigned int> size);

  public:
	virtual ~Image();
	Vec2<unsigned int> size;

	sp<RendererImageData> rendererPrivateData;
	bool dirty;
	Rect<unsigned int> bounds;

	wp<ImageSet> owningSet;
	unsigned indexInSet;
};

class LazyImage : public Image
{
  private:
	sp<Image> realImage;

  public:
	LazyImage();
	virtual ~LazyImage() = default;
	sp<Image> &getRealImage();
};

// A surface is an image you can render to. No SW locking is allowed!
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
	sp<RGBImage> toRGBImage(sp<Palette> p);
	static void blit(sp<PaletteImage> src, sp<PaletteImage> dst,
	                 Vec2<unsigned int> srcOffset = {0, 0}, Vec2<unsigned int> dstOffset = {0, 0});

	void CalculateBounds();
};

class PaletteImageLock
{
  private:
	sp<PaletteImage> img;
	// Disallow copy
	PaletteImageLock(const PaletteImageLock &) = delete;
	ImageLockUse use;

  public:
	PaletteImageLock(sp<PaletteImage> img, ImageLockUse use = ImageLockUse::Write);
	~PaletteImageLock();
	uint8_t get(const Vec2<unsigned int> &pos) const
	{
		unsigned offset = pos.y * this->img->size.x + pos.x;
		assert(offset < this->img->size.x * this->img->size.y);
		return this->img->indices[offset];
	}
	void set(const Vec2<unsigned int> &pos, const uint8_t idx)
	{
		unsigned offset = pos.y * this->img->size.x + pos.x;
		assert(offset < this->img->size.x * this->img->size.y);
		this->img->indices[offset] = idx;
	}

	// FIXME: Magic backdoor to the index data
	void *getData();
};

class RGBImage : public Image
{
  private:
	friend class RGBImageLock;
	std::unique_ptr<Colour[]> pixels;

  public:
	RGBImage(Vec2<unsigned int> size, Colour initialColour = Colour(0, 0, 0, 0));
	~RGBImage();
	void saveBitmap(const UString &filename);
	static void blit(sp<RGBImage> src, sp<RGBImage> dst, Vec2<unsigned int> srcOffset = {0, 0},
	                 Vec2<unsigned int> dstOffset = {0, 0});
};

class RGBImageLock
{
  private:
	sp<RGBImage> img;
	// Disallow copy
	RGBImageLock(const RGBImageLock &) = delete;
	ImageLockUse use;

  public:
	RGBImageLock(sp<RGBImage> img, ImageLockUse use = ImageLockUse::Write);
	~RGBImageLock();
	Colour get(const Vec2<unsigned int> &pos) const
	{
		unsigned offset = pos.y * this->img->size.x + pos.x;
		assert(offset < this->img->size.x * this->img->size.y);
		return this->img->pixels[offset];
	}
	void set(const Vec2<unsigned int> &pos, const Colour &c)
	{
		unsigned offset = pos.y * this->img->size.x + pos.x;
		assert(offset < this->img->size.x * this->img->size.y);
		this->img->pixels[offset] = c;
	}

	// FIXME: Magic backdoor to the RGBA data
	void *getData();
};

class ImageSet : public ResObject
{
  public:
	std::vector<sp<Image>> images;
	Vec2<unsigned int> maxSize;

	sp<RendererImageData> rendererPrivateData;
};

}; // namespace OpenApoc

#pragma once
#include "library/sp.h"
#include "library/rect.h"
#include "library/vec.h"
#include "framework/resource.h"

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

	std::unique_ptr<RendererImageData> rendererPrivateData;
	bool dirty;
	Rect<unsigned int> bounds;

	wp<ImageSet> owningSet;
	unsigned indexInSet;
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
	static void blit(sp<PaletteImage> src, Vec2<unsigned int> offset, sp<PaletteImage> dst);

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
	uint8_t get(Vec2<unsigned int> pos);
	void set(Vec2<unsigned int> pos, uint8_t idx);

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
	static void blit(sp<RGBImage> src, Vec2<unsigned int> srcOffset, sp<RGBImage> dst,
	                 Vec2<unsigned int> dstOffset);
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
	Colour get(Vec2<unsigned int> pos);
	void set(Vec2<unsigned int> pos, Colour c);

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

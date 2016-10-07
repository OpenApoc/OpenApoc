#include "framework/image.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/palette.h"
#include "library/sp.h"
#include <cstring>

namespace OpenApoc
{

Image::~Image() = default;

Image::Image(Vec2<unsigned int> size)
    : size(size), dirty(true), bounds(0, 0, size.x, size.y), indexInSet(0)
{
}

Surface::Surface(Vec2<unsigned int> size) : Image(size) {}

Surface::~Surface() = default;

PaletteImage::PaletteImage(Vec2<unsigned int> size, uint8_t initialIndex)
    : Image(size), indices(new uint8_t[size.x * size.y])
{
	for (unsigned int i = 0; i < size.x * size.y; i++)
		this->indices[i] = initialIndex;
}

PaletteImage::~PaletteImage() = default;

sp<RGBImage> PaletteImage::toRGBImage(sp<Palette> p)
{
	sp<RGBImage> i = mksp<RGBImage>(size);

	RGBImageLock imgLock{i, ImageLockUse::Write};

	for (unsigned int y = 0; y < this->size.y; y++)
	{
		for (unsigned int x = 0; x < this->size.x; x++)
		{
			uint8_t idx = this->indices[y * this->size.x + x];
			imgLock.set(Vec2<unsigned int>{x, y}, p->getColour(idx));
		}
	}
	return i;
}

void PaletteImage::blit(sp<PaletteImage> src, sp<PaletteImage> dst, Vec2<unsigned int> srcOffset,
                        Vec2<unsigned int> dstOffset)
{
	PaletteImageLock reader(src, ImageLockUse::Read);
	PaletteImageLock writer(dst, ImageLockUse::Write);

	Vec2<unsigned int> size = {std::min(src->size.x - srcOffset.x, dst->size.x - dstOffset.x),
	                           std::min(src->size.y - srcOffset.y, dst->size.y - dstOffset.y)};
	Vec2<unsigned int> pos;
	for (pos.y = 0; pos.y < size.y; pos.y++)
	{
		for (pos.x = 0; pos.x < size.x; pos.x++)
		{
			Vec2<unsigned int> readPos = srcOffset + pos;
			Vec2<unsigned int> writePos = dstOffset + pos;
			writer.set(writePos, reader.get(readPos));
		}
	}
}

RGBImage::RGBImage(Vec2<unsigned int> size, Colour initialColour)
    : Image(size),
      pixels(reinterpret_cast<Colour *>(operator new[](size.x *size.y * sizeof(Colour))))
{
	if (initialColour.r == initialColour.g && initialColour.r == initialColour.b &&
	    initialColour.r == initialColour.a)
	{
		memset(reinterpret_cast<void *>(pixels.get()), initialColour.r,
		       sizeof(Colour) * size.x * size.y);
	}
	else
	{
		for (unsigned int i = 0; i < size.x * size.y; i++)
			this->pixels[i] = initialColour;
	}
}

void RGBImage::blit(sp<RGBImage> src, sp<RGBImage> dst, Vec2<unsigned int> srcOffset,
                    Vec2<unsigned int> dstOffset)
{
	RGBImageLock reader(src, ImageLockUse::Read);
	RGBImageLock writer(dst, ImageLockUse::Write);

	Vec2<unsigned int> size = {std::min(src->size.x - srcOffset.x, dst->size.x - dstOffset.x),
	                           std::min(src->size.y - srcOffset.y, dst->size.y - dstOffset.y)};
	Vec2<unsigned int> pos;
	for (pos.y = 0; pos.y < size.y; pos.y++)
	{
		for (pos.x = 0; pos.x < size.x; pos.x++)
		{
			Vec2<unsigned int> readPos = srcOffset + pos;
			Vec2<unsigned int> writePos = dstOffset + pos;
			writer.set(writePos, reader.get(readPos));
		}
	}
}

RGBImage::~RGBImage() = default;

RGBImageLock::RGBImageLock(sp<RGBImage> img, ImageLockUse use) : img(img), use(use)
{
	// FIXME: Readback from renderer?
	// FIXME: Disallow multiple locks?
}

RGBImageLock::~RGBImageLock() = default;

void *RGBImageLock::getData() { return this->img->pixels.get(); }

PaletteImageLock::PaletteImageLock(sp<PaletteImage> img, ImageLockUse use) : img(img), use(use)
{
	// FIXME: Readback from renderer?
	// FIXME: Disallow multiple locks?
}

PaletteImageLock::~PaletteImageLock() = default;

void *PaletteImageLock::getData() { return this->img->indices.get(); }

void PaletteImage::calculateBounds()
{
	unsigned int minX = this->size.x, minY = this->size.y, maxX = 0, maxY = 0;

	for (unsigned int y = 0; y < this->size.y; y++)
	{
		for (unsigned int x = 0; x < this->size.x; x++)
		{
			unsigned int offset = y * this->size.x + x;
			if (this->indices[offset])
			{
				if (minX > x)
					minX = x;
				if (minY > y)
					minY = y;
				if (maxX < x)
					maxX = x;
				if (maxY < y)
					maxY = y;
			}
		}
	}
	this->bounds = {minX, minY, maxX, maxY};
}

LazyImage::LazyImage() : Image({0, 0}) {}

sp<Image> &LazyImage::getRealImage()
{
	if (!this->realImage)
	{
		this->realImage = fw().data->loadImage(this->path);
		this->size = this->realImage->size;
	}
	return this->realImage;
}

}; // namespace OpenApoc

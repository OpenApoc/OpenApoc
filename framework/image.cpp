#include "framework/image.h"
#include "framework/palette.h"
#include "framework/renderer.h"

namespace OpenApoc {

Image::~Image()
{
}

Image::Image(Vec2<unsigned int> size)
	: size(size), dirty(true)
{}

Surface::Surface(Vec2<unsigned int> size)
	: Image(size)
{}

Surface::~Surface()
{}

PaletteImage::PaletteImage(Vec2<unsigned int> size, uint8_t initialIndex)
	: Image(size), indices(new uint8_t[size.x*size.y])
{
	for (unsigned int i = 0; i < size.x*size.y; i++)
		this->indices[i] = initialIndex;
}

PaletteImage::~PaletteImage()
{}

std::shared_ptr<RGBImage>
PaletteImage::toRGBImage(std::shared_ptr<Palette> p)
{
	std::shared_ptr<RGBImage> i = std::make_shared<RGBImage>(size);

	RGBImageLock imgLock{i, ImageLockUse::Write};

	for (unsigned int y = 0; y < this->size.y; y++)
	{
		for (unsigned int x = 0; x < this->size.x; x++)
		{
			uint8_t idx = this->indices[y*this->size.x + x];
			imgLock.set(Vec2<unsigned int>{x,y}, p->GetColour(idx));
		}
	}
	return i;
}

void
PaletteImage::blit(std::shared_ptr<PaletteImage> src, Vec2<unsigned int> offset, std::shared_ptr<PaletteImage> dst)
{
	PaletteImageLock reader(src, ImageLockUse::Read);
	PaletteImageLock writer(dst, ImageLockUse::Write);

	for (unsigned int y = 0; y < src->size.y; y++)
	{
		for (unsigned int x = 0; x < src->size.x; x++)
		{
			Vec2<unsigned int> readPos{x,y};
			Vec2<unsigned int> writePos{readPos + offset};
			if (writePos.x >= dst->size.x ||
			    writePos.y >= dst->size.y)
				break;
			writer.set(writePos, reader.get(readPos));
		}
	}
}

RGBImage::RGBImage(Vec2<unsigned int> size, Colour initialColour)
: Image(size), pixels(new Colour[size.x*size.y])
{
	for (unsigned int i = 0; i < size.x*size.y; i++)
		this->pixels[i] = initialColour;
}

RGBImage::~RGBImage()
{}

RGBImageLock::RGBImageLock(std::shared_ptr<RGBImage> img, ImageLockUse use)
	: img(img), use(use)
{
	//FIXME: Readback from renderer?
	//FIXME: Disallow multiple locks?
}

RGBImageLock::~RGBImageLock()
{}

Colour
RGBImageLock::get(Vec2<unsigned int> pos)
{
	//FIXME: Check read use
	unsigned offset = pos.y * this->img->size.x + pos.x;
	assert(offset < this->img->size.x * this->img->size.y);
	return this->img->pixels[offset];
}

void
RGBImageLock::set(Vec2<unsigned int> pos, Colour &c)
{
	unsigned offset = pos.y * this->img->size.x + pos.x;
	assert(offset < this->img->size.x * this->img->size.y);
	this->img->pixels[offset] = c;
}

void *
RGBImageLock::getData()
{
	return this->img->pixels.get();
}

PaletteImageLock::PaletteImageLock(std::shared_ptr<PaletteImage> img, ImageLockUse use)
: img(img), use(use)
{
	//FIXME: Readback from renderer?
	//FIXME: Disallow multiple locks?
}

PaletteImageLock::~PaletteImageLock()
{}

uint8_t
PaletteImageLock::get(Vec2<unsigned int> pos)
{
	//FIXME: Check read use
	unsigned offset = pos.y * this->img->size.x + pos.x;
	assert(offset < this->img->size.x * this->img->size.y);
	return this->img->indices[offset];
}

void
PaletteImageLock::set(Vec2<unsigned int> pos, uint8_t idx)
{
	//FIXME: Check write use
	unsigned offset = pos.y * this->img->size.x + pos.x;
	assert(offset < this->img->size.x * this->img->size.y);
	this->img->indices[offset] = idx;
}

void *
PaletteImageLock::getData()
{
	return this->img->indices.get();
}

}; //namespace OpenApoc

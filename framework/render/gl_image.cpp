#include "gl_util.h"
#include "framework/image.h"
#include "framework/includes.h"
#include "game/apocresources/palette.h"

#include <memory>

namespace OpenApoc {

Image::~Image(){}

class RGBImageImpl
{
private:
	friend class RGBImageLock;
	friend class RGBImage;
	friend class PaletteImageImpl;
	bool dirty;
	Vec2<int> size;
	GLuint texHandle;
	bool locked;
	std::vector<Colour> lockedPixels;
	std::shared_ptr<GL::RGBSpriteProgram> program;
	bool isPalette;
public:
	RGBImageImpl(Vec2<int> size, bool isPalette)
		: dirty(true),size(size),texHandle(0),locked(false), isPalette(isPalette)
	{
		glGenTextures(1, &this->texHandle);
		lockedPixels.resize(size.x*size.y);
		this->program = GL::RGBSpriteProgram::get();
	}
	~RGBImageImpl()
	{
		glDeleteTextures(1, &this->texHandle);
	}
	void update()
	{
		if (!this->dirty)
			return;
		if (isPalette)
		{
			glBindTexture(GL_TEXTURE_RECTANGLE, this->texHandle);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)this->lockedPixels.data());
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, this->texHandle);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)this->lockedPixels.data());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		this->dirty = false;
	}
	void draw(GLuint fboTarget, Vec2<int> offset, Vec2<int> size, Vec2<int> screenSize)
	{
		this->update();
		GL::BindTexture t(texHandle, 0);
		this->program->enable(offset, size, screenSize);
		GL::IdentityQuad::draw(this->program->positionLoc);
	}

};

RGBImage::RGBImage(ALLEGRO_BITMAP *bmp, bool isPalette)
{
	this->width = al_get_bitmap_width(bmp);
	this->height = al_get_bitmap_height(bmp);
	pimpl.reset(new RGBImageImpl{Vec2<int>{width, height}, isPalette});

	ALLEGRO_LOCKED_REGION *lr = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READONLY);

	assert(lr);
	assert(lr->format == ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
	assert(lr->pixel_size == 4);


	Colour *dst = pimpl->lockedPixels.data();
	char *src = (char*)lr->data;

	for (int y = 0; y < pimpl->size.y; y++)
	{
		memcpy(dst, src, pimpl->size.x * sizeof(Colour));
		dst += pimpl->size.x;
		src += lr->pitch;
	}
	al_unlock_bitmap(bmp);
	pimpl->dirty = true;
}

RGBImage::RGBImage(int width, int height, Colour initialColour, bool isPalette)
{
	this->width = width;
	this->height = height;
	pimpl.reset(new RGBImageImpl{Vec2<int>{width, height}, isPalette});
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			pimpl->lockedPixels[y*width + x] = initialColour;
		}
	}
	pimpl->dirty = true;
}

RGBImage::~RGBImage()
{
}

void RGBImage::draw(float dx, float dy)
{
	this->drawScaled(0, 0, this->width, this->height, dx, dy, this->width, this->height);
}

void RGBImage::drawRotated(float cx, float cy, float dx, float dy, float angle)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	this->drawScaled(0, 0, pimpl->size.x, pimpl->size.y, dx, dy, pimpl->size.x, pimpl->size.y);
}

void RGBImage::drawScaled(float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
{
	assert(sx == 0);
	assert(sy == 0);
	assert(sw == pimpl->size.x);
	assert(sh == pimpl->size.y);
	ALLEGRO_BITMAP *target = al_get_target_bitmap();
	GLuint fbo = al_get_opengl_fbo(target);

	int screenHeight = al_get_bitmap_height(target);
	int screenWidth = al_get_bitmap_width(target);

	pimpl->draw(fbo, Vec2<int>{dx, dy}, Vec2<int>{dw, dh}, Vec2<int>{screenWidth, screenHeight});
	glUseProgram(0);
}

void RGBImage::saveBitmap(const std::string &fileName)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

RGBImageLock::RGBImageLock(std::shared_ptr<RGBImage> img, ImageLockUse use)
	: img(img), use(use)
{
	assert(img->pimpl->locked == false);
	img->pimpl->locked = true;
	if (use != ImageLockUse::Write)
	{
		std::cerr << __PRETTY_FUNCTION__ << "NO READBACK\n";
	}
	if (use == ImageLockUse::Write || use == ImageLockUse::ReadWrite)
	{
		img->pimpl->dirty = true;
	}
}

RGBImageLock::~RGBImageLock()
{
	assert(img->pimpl->locked);
	img->pimpl->locked = false;
	img->pimpl->update();
}

Colour
RGBImageLock::get(int x, int y)
{
	assert(x >= 0);
	assert(x < img->pimpl->size.x);
	assert(y >= 0);
	assert(y < img->pimpl->size.y);
	return img->pimpl->lockedPixels[y*img->pimpl->size.x + x];
}

void
RGBImageLock::set(int x, int y, Colour &c)
{
	img->pimpl->lockedPixels[y*img->pimpl->size.x + x] = c;
}

class PaletteImageImpl
{
private:
	friend class PaletteImageLock;
	friend class PaletteImage;
	bool dirty;
	Vec2<int> size;
	GLuint texHandle;
	bool locked;
	std::vector<uint8_t> lockedPixels;
	std::shared_ptr<GL::PaletteSpriteProgram> program;
public:
	PaletteImageImpl(Vec2<int> size)
		: dirty(true), size(size), texHandle(0), locked(false)
	{
		glGenTextures(1, &this->texHandle);
		lockedPixels.resize(size.x*size.y);
		this->program = GL::PaletteSpriteProgram::get();
	}
	~PaletteImageImpl()
	{
		glDeleteTextures(1, &this->texHandle);
	}
	void update()
	{
		if (!this->dirty)
			return;
		glBindTexture(GL_TEXTURE_2D, this->texHandle);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, size.x, size.y, 0, GL_RED, GL_UNSIGNED_BYTE, (void*)this->lockedPixels.data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		this->dirty = false;
	}
	void draw(GLuint fboTarget, std::shared_ptr<RGBImage> paletteImage, Vec2<int> offset, Vec2<int> size, Vec2<int> screenSize)
	{
		this->update();
		paletteImage->pimpl->update();
		GL::BindTexture t(texHandle, 0);
		GL::BindTextureRect pal(paletteImage->pimpl->texHandle, 1);
		this->program->enable(offset, size, screenSize);
		GL::IdentityQuad::draw(this->program->positionLoc);
	}
};

PaletteImageLock::PaletteImageLock(std::shared_ptr<PaletteImage> img, ImageLockUse use)
	: img(img), use(use)
{
	assert(img->pimpl->locked == false);
	img->pimpl->locked = true;
	if (use != ImageLockUse::Write)
	{
		std::cerr << __PRETTY_FUNCTION__ << "NO READBACK\n";
	}
	if (use == ImageLockUse::Write || use == ImageLockUse::ReadWrite)
	{
		img->pimpl->dirty = true;
	}
}

PaletteImageLock::~PaletteImageLock()
{
	assert(img->pimpl->locked);
	img->pimpl->locked = false;
	img->pimpl->update();
}

uint8_t
PaletteImageLock::get(int x, int y)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

void
PaletteImageLock::set(int x, int y, uint8_t idx)
{
	assert(x >= 0);
	assert(x < img->pimpl->size.x);
	assert(y >= 0);
	assert(y < img->pimpl->size.y);
	img->pimpl->lockedPixels[y*img->pimpl->size.x + x] = idx;
}

PaletteImage::PaletteImage(int width, int height, uint8_t initialIndex)
{
	this->width = width;
	this->height = height;
	pimpl.reset(new PaletteImageImpl{Vec2<int>{width, height}});
	memset(pimpl->lockedPixels.data(), initialIndex, width*height);
}

PaletteImage::~PaletteImage(){}

//FIXME TESTING-ONLY HACKS

void
PaletteImage::drawRotated(float cx, float cy, float dx, float dy, float angle)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

void
PaletteImage::drawScaled(float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
{
	assert(sx == 0);
	assert(sy == 0);
	assert(sw == pimpl->size.x);
	assert(sh == pimpl->size.y);
	ALLEGRO_BITMAP *target = al_get_target_bitmap();
	GLuint fbo = al_get_opengl_fbo(target);

	int screenHeight = al_get_bitmap_height(target);
	int screenWidth = al_get_bitmap_width(target);

	pimpl->draw(fbo, this->pal->getImage(), Vec2<int>{dx, dy}, Vec2<int>{dw, dh}, Vec2<int>{screenWidth, screenHeight});
	glUseProgram(0);
}

void
PaletteImage::draw(float dx, float dy)
{
	this->drawScaled(0, 0, this->width, this->height, dx, dy, this->width, this->height);
}

void
PaletteImage::saveBitmap(const std::string &filename)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

std::shared_ptr<RGBImage>
PaletteImage::toRGBImage()
{
	return this->toRGBImage(this->pal);
}

std::shared_ptr<RGBImage>
PaletteImage::toRGBImage(std::shared_ptr<Palette> p)
{
	auto img = std::make_shared<RGBImage>(this->width, this->height);
	RGBImageLock dst(img);
	for (int y = 0; y < this->height; y++)
	{
		for (int x = 0; x < this->width; x++)
		{
			uint8_t idx = this->pimpl->lockedPixels[y*width + x];
			dst.set(x, y, p->GetColour(idx));
		}
	}
	return img;
}

void
PaletteImage::setPalette(std::shared_ptr<Palette> newPal)
{
	this->pal = newPal;
}

}; //namespace OpenApoc

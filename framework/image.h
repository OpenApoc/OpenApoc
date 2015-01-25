#pragma once

#include "includes.h"

namespace OpenApoc {

class Palette;
class RGBImage;

enum class ImageLockUse
{
	Read,
	Write,
	ReadWrite,
};

class Image
{
	public:
		virtual ~Image();
		virtual void drawRotated(float cx, float cy, float dx, float dy, float angle) = 0;
		virtual void drawScaled(float sx, float sy, float sw, float sh,
			float dx, float dy, float dw, float dh) = 0;
		virtual void draw(float dx, float dy) = 0;
		virtual void saveBitmap(const std::string &filename) = 0;

		int height, width;
};

class PaletteImageImpl;
class PaletteImage : public Image
{
	private:
		friend class PaletteImageLock;
		std::unique_ptr<PaletteImageImpl> pimpl;
	public:
		PaletteImage(int width, int height, uint8_t initialIndex = 0);
		~PaletteImage();
		virtual void drawRotated(float cx, float cy, float dx, float dy, float angle);
		virtual void drawScaled(float sx, float sy, float sw, float sh,
			float dx, float dy, float dw, float dh);
		virtual void draw(float dx, float dy);
		virtual void saveBitmap(const std::string &filename);

		void setPalette(std::shared_ptr<Palette> newPal);
		std::shared_ptr<RGBImage> toRGBImage();
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
		uint8_t get(int x, int y);
		void set(int x, int y, uint8_t idx);
};

class RGBImageImpl;
class RGBImage : public Image
{
	private:
		friend class RGBImageLock;
		std::unique_ptr<RGBImageImpl> pimpl;
	public:
		RGBImage(ALLEGRO_BITMAP *bmp);
		RGBImage(int width, int height, Colour initialColour = Colour(0,0,0,0));
		~RGBImage();
		virtual void drawRotated(float cx, float cy, float dx, float dy, float angle);
		virtual void drawScaled(float sx, float sy, float sw, float sh,
			float dx, float dy, float dw, float dh);
		virtual void draw(float dx, float dy);
		virtual void saveBitmap(const std::string &filename);
};

class RGBImageLock
{
	private:
		std::shared_ptr<RGBImage> img;
		ALLEGRO_LOCKED_REGION *region;
		//Disallow copy
		RGBImageLock(const RGBImageLock &) = delete;
		ImageLockUse use;
	public:
		RGBImageLock(std::shared_ptr<RGBImage> img, ImageLockUse use = ImageLockUse::Write);
		~RGBImageLock();
		Colour get(int x, int y);
		void set(int x, int y, Colour &c);
};

}; //namespace OpenApoc

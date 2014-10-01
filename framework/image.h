#pragma once

#include "includes.h"
#include "../library/vec.h"
#include "../library/colour.h"
#include <memory>

class Image
{
	private:
		friend class ImageLock;
		ALLEGRO_BITMAP *bmp;
		bool locked;
	public:
		Image(ALLEGRO_BITMAP *bmp);
		Image(int width, int height, Colour initialColour = Colour(0,0,0,0));
		~Image();
		void drawRotated(float cx, float cy, float dx, float dy, float angle);
		void drawScaled(float sx, float sy, float sw, float sh,
			float dx, float dy, float dw, float dh);
		void draw(float dx, float dy);
		void saveBitmap(const std::string &filename);

		int height, width;
};

class ImageLock
{
	private:
		std::shared_ptr<Image> img;
		ALLEGRO_LOCKED_REGION *region;
		//Disallow copy
		ImageLock(const ImageLock &) = delete;
	public:
		ImageLock(std::shared_ptr<Image> img);
		~ImageLock();
		Colour get(int x, int y);
		void set(int x, int y, Colour &c);
};

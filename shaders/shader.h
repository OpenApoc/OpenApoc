
#pragma once

#include "../framework/includes.h"

typedef struct PackedARGB8888
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} PackedARGB8888;

class Shader
{
	public:
		virtual void Apply( ALLEGRO_BITMAP* Target ) = 0;
};

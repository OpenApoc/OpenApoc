
#pragma once

#include "shader.h"

class ShaderGreyscale : public Shader
{
	public:
		virtual void Apply( ALLEGRO_BITMAP* Target );
};
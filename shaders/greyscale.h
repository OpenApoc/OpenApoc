
#pragma once

#include "shader.h"

namespace OpenApoc {

class ShaderGreyscale : public Shader
{
	public:
		virtual void Apply( ALLEGRO_BITMAP* Target );
};

}; //namespace OpenApoc

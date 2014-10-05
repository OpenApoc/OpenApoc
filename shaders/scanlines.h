
#pragma once

#include "shader.h"

namespace OpenApoc {

class ShaderScanlines : public Shader
{
	private:
		int nonScanLineWidth;
		int scanLineWidth;
		int scanDecrease;

	public:
		ShaderScanlines();
		ShaderScanlines(int NonScanWidth, int ScanWidth, int ScanDecrease);
		void Apply( Framework &fw, ALLEGRO_BITMAP* Target );
};

}; //namespace OpenApoc

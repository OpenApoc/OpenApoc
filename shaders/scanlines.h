
#pragma once

#include "shader.h"

class ShaderScanlines : public Shader
{
	private:
		int nonScanLineWidth;
		int scanLineWidth;
		int scanDecrease;

	public:
		ShaderScanlines();
		ShaderScanlines(int NonScanWidth, int ScanWidth, int ScanDecrease);
		void Apply( ALLEGRO_BITMAP* Target );
};
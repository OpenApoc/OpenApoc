
#pragma once

#include "framework/includes.h"
#include "framework/data.h"

namespace OpenApoc {

class RawSound
{

	private:
		Memory* sounddata;
		ALLEGRO_SAMPLE* soundsample;

	public:
		RawSound( std::string Filename );
		~RawSound();

		void PlaySound();

};

}; //namespace OpenApoc


#pragma once

#include "../../framework/includes.h"
#include "../../library/memory.h"

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
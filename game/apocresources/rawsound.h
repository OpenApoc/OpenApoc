
#pragma once

#include "../../framework/includes.h"
#include "../../framework/data.h"
#include "../../library/memory.h"

class RawSound
{

	private:
		Memory* sounddata;
		ALLEGRO_SAMPLE* soundsample;

	public:
		RawSound( std::wstring Filename );
		~RawSound();

		void PlaySound();

};

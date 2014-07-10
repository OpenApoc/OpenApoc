
#pragma once

#include "../../framework/includes.h"
#include "../../library/memory.h"

class Music
{

	private:
		Memory* sounddata;
		ALLEGRO_SAMPLE* soundsample;

	public:
		Music( std::string Filename );
		~Music();

		void Play();

};

#pragma once

#include "../../framework/includes.h"
#include "../../framework/framework.h"
#include "../../library/memory.h"

class Music
{

	private:
		Memory* sounddata;
		ALLEGRO_SAMPLE* soundsample;
		static long starts[];
		static long lengths[];

	public:
		Music( int Track );
		~Music();

		void Play();

};

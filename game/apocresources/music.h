
#pragma once

#include "framework/includes.h"
#include "framework/framework.h"

namespace OpenApoc {

class Music
{

	private:
		Memory sounddata;
		ALLEGRO_SAMPLE* soundsample;
		ALLEGRO_SAMPLE_ID play_id;
		static long starts[];
		static long lengths[];

	public:
		Music( int Track );
		~Music();

		void Play();
		void Stop();

};

}; //namespace OpenApoc

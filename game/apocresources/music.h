
#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Framework;

class Music
{
	private:
		Memory sounddata;
		ALLEGRO_SAMPLE* soundsample;
		ALLEGRO_SAMPLE_ID play_id;
		static long starts[];
		static long lengths[];
		bool playing;

	public:
		Music( Framework &fw, int Track );
		~Music();

		void Play();
		void Stop();

};

}; //namespace OpenApoc

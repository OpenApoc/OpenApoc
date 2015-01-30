
#pragma once

#include "framework/includes.h"
#include <allegro5/allegro_audio.h>

namespace OpenApoc {

class Framework;

class Music
{
	private:
		std::unique_ptr<char[]> data;
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

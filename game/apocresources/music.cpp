
#include "music.h"

Music::Music( std::string Filename )
{
	std::string path = "data/MUSIC";
	ALLEGRO_FILE* f = al_fopen( path.c_str(), "rb" );
	sounddata = new Memory( al_fsize( f ) );
	al_fread( f, sounddata->GetData(), 22050 * 16 * 60 );
	al_fclose( f );

	soundsample = al_create_sample( sounddata->GetData(), sounddata->GetSize(), 22050, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_2, false );
}

Music::~Music()
{
	al_destroy_sample( soundsample );
	delete sounddata;
}

void Music::Play()
{
	al_play_sample( soundsample, 1.0f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, 0 );
}

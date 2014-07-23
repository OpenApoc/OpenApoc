
#include "rawsound.h"

RawSound::RawSound( std::wstring Filename )
{
	std::wstring path = "/RAWSOUND/";
	path.append( Filename );
	ALLEGRO_FILE* f = DATA->load_file( path, "rb" );
	sounddata = new Memory( al_fsize( f ) );
	al_fread( f, sounddata->GetData(), sounddata->GetSize() );
	al_fclose( f );

	soundsample = al_create_sample( sounddata->GetData(), sounddata->GetSize(), 22050, ALLEGRO_AUDIO_DEPTH_UNSIGNED, ALLEGRO_CHANNEL_CONF_1, false );
}

RawSound::~RawSound()
{
	al_destroy_sample( soundsample );
	delete sounddata;
}

void RawSound::PlaySound()
{
	al_play_sample( soundsample, 1.0f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, 0 );
}

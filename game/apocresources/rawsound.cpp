
#include "rawsound.h"
#include "framework/framework.h"

namespace OpenApoc {

RawSound::RawSound( Framework &fw, std::string Filename )
{
	std::string path = "xcom3/RAWSOUND/";
	path.append( Filename );
	PHYSFS_file* f = fw.data->load_file( path, "rb" );
	sounddata = new Memory( PHYSFS_fileLength( f ) );
	PHYSFS_readBytes( f, sounddata->GetData(), sounddata->GetSize() );
	PHYSFS_close( f );

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

}; //namespace OpenApoc

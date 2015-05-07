#include "framework/sound_interface.h"
#include <iostream>
#include <allegro5/allegro_audio.h>
#include <list>

namespace {

static const int max_samples = 10;

using namespace OpenApoc;

class AllegroSampleData : public BackendSampleData
{
public:
	ALLEGRO_SAMPLE *s;	
	virtual ~AllegroSampleData()
	{
		if (s)
			al_destroy_sample(s);
	}
	AllegroSampleData(std::shared_ptr<Sample> sample)
	{
		ALLEGRO_AUDIO_DEPTH depth;
		switch (sample->format.format)
		{
			case AudioFormat::SampleFormat::PCM_SINT16:
				depth = ALLEGRO_AUDIO_DEPTH_INT16;
				break;
			case AudioFormat::SampleFormat::PCM_UINT8:
				depth = ALLEGRO_AUDIO_DEPTH_UINT8;
				break;
			default:
				std::cerr << "Unsupported sample format\n";
				return;
		}
		ALLEGRO_CHANNEL_CONF channels;
		switch (sample->format.channels)
		{
			case 1:
				channels = ALLEGRO_CHANNEL_CONF_1;
				break;
			case 2:
				channels = ALLEGRO_CHANNEL_CONF_2;
				break;
			default:
				std::cerr << "Unsupported channel count\n";
				return;
		}

		this->s = al_create_sample(sample->data.get(), sample->sampleCount,
			sample->format.frequency, depth, channels, false);
		if (!this->s)
			std::cerr << "Failed to create sample\n";
		
	}
};

class AllegroSoundBackend : public SoundBackend
{
	AudioFormat preferredFormat;
	//FIXME: Keep last max_samples live as they may still be playing (no way
	// to tell if allegro is finished without managing the stream outselves?)
	std::list<std::shared_ptr<Sample>> liveSamples;
public:
	virtual void playSample(std::shared_ptr<Sample> sample)
	{
		std::cerr << "Playing sample\n";
		if (!sample->backendData)
			sample->backendData.reset(new AllegroSampleData(sample));
		liveSamples.push_back(sample);
		if (liveSamples.size() > max_samples)
			liveSamples.pop_front();
		AllegroSampleData *sampleData = static_cast<AllegroSampleData*>(sample->backendData.get());

		if (!al_play_sample(sampleData->s, 1.0f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, nullptr))
		{
			std::cerr << "Failed to play sample\n";
		}

	}

	virtual void playMusic(std::shared_ptr<MusicTrack>, std::function<void(void*)> finishedCallback, void *callbackData)
	{

	}

	virtual void stopMusic()
	{

	}

	virtual ~AllegroSoundBackend()
	{

	}

	virtual const AudioFormat& getPreferredFormat()
	{
		return preferredFormat;
	}
};

class AllegroSoundBackendFactory : public SoundBackendFactory
{
	bool initialised;
public:
	AllegroSoundBackendFactory()
		: initialised(false) {}
	virtual SoundBackend *create()
	{
		if (initialised)
		{
			std::cerr << "Trying to load multiple allegro sound backends\n";
			return nullptr;
		}

		if (!al_install_audio())
		{
			std::cerr << "Failed to init allegro sound backend\n";
			return nullptr;
		}

		if (!al_reserve_samples(max_samples))
		{
			std::cerr << "Failed to reserve sample channels\n";
			return nullptr;
		}

		initialised = true;
		return new AllegroSoundBackend();
	};

	virtual ~AllegroSoundBackendFactory()
	{
		if (initialised)
		{
			al_uninstall_audio();
		}
	}
};

SoundBackendRegister<AllegroSoundBackendFactory> load_at_init("allegro");

}; //anonymous namespace

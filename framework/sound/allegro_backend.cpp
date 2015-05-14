#include "framework/sound_interface.h"
#include "framework/logger.h"
#include <iostream>
#include <allegro5/allegro_audio.h>
#include <list>
#include <thread>

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
				LogWarning("Unsupported sample format");
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
				LogWarning("Unsupported sample format");
				return;
		}

		this->s = al_create_sample(sample->data.get(), sample->sampleCount,
			sample->format.frequency, depth, channels, false);
		if (!this->s)
			LogError("Failed to create sample");
	}
};

class AllegroSoundBackend : public SoundBackend
{
	AudioFormat preferredFormat;
	//FIXME: Keep last max_samples live as they may still be playing (no way
	// to tell if allegro is finished without managing the stream outselves?)
	std::list<std::shared_ptr<Sample>> liveSamples;

	bool stopThread;
	std::unique_ptr<std::thread> musicThread;

public:
	virtual void playSample(std::shared_ptr<Sample> sample)
	{
		if (!sample->backendData)
			sample->backendData.reset(new AllegroSampleData(sample));
		liveSamples.push_back(sample);
		if (liveSamples.size() > max_samples)
			liveSamples.pop_front();
		AllegroSampleData *sampleData = static_cast<AllegroSampleData*>(sample->backendData.get());

		if (!al_play_sample(sampleData->s, 1.0f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, nullptr))
		{
			LogError("Failed to play sample");
		}

	}

	static void musicThreadFunction(AllegroSoundBackend &parent, std::shared_ptr<MusicTrack> track, std::function<void(void*)> finishedCallback, void *callbackData)
	{
		ALLEGRO_AUDIO_DEPTH depth;
		switch (track->format.format)
		{
			case AudioFormat::SampleFormat::PCM_SINT16:
				depth = ALLEGRO_AUDIO_DEPTH_INT16;
				break;
			default:
				LogWarning("AllegroSoundBackend: Unsupported music format");
				return;
		}
		ALLEGRO_CHANNEL_CONF channels;
		switch (track->format.channels)
		{
			case 1:
				channels = ALLEGRO_CHANNEL_CONF_1;
				break;
			case 2:
				channels = ALLEGRO_CHANNEL_CONF_2;
				break;
			default:
				LogWarning("Unsupported music channels");
				return;
		}
		ALLEGRO_AUDIO_STREAM *stream = al_create_audio_stream(2, track->requestedSampleBufferSize, track->format.frequency, depth, channels);
		if (!stream)
		{
			LogError("Failed to create music stream");
			return;
		}

		al_attach_audio_stream_to_mixer(stream, al_get_default_mixer());

		ALLEGRO_EVENT_QUEUE *eventQueue = al_create_event_queue();
		al_register_event_source(eventQueue, al_get_audio_stream_event_source(stream));
		while (!parent.stopThread)
		{
			ALLEGRO_EVENT event;
			al_wait_for_event(eventQueue, &event);
			if (event.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT)
			{
				uint8_t *buf = (uint8_t*)al_get_audio_stream_fragment(stream);
				if (!buf)
				{
					/* Sometimes we get an event with no buffer? */
					continue;
				}
				unsigned int returnedSamples;
				auto callbackReturn = track->callback(track, track->requestedSampleBufferSize, buf, &returnedSamples);
				if (returnedSamples != track->requestedSampleBufferSize)
				{
					LogWarning("AllegroSoundBackend: Requested %u samples, got %u - buffer underrun not yet handled correctly",  track->requestedSampleBufferSize, returnedSamples);
				}
				al_set_audio_stream_fragment(stream, buf);

				if (callbackReturn == MusicTrack::MusicCallbackReturn::End)
				{
					al_drain_audio_stream(stream);
					LogError("Unimplement track progression");
					std::ignore = finishedCallback;
					std::ignore = callbackData;
					//FIXME: finishedCallback() in a way that calling playMusic() won't just wedge waiting for the same thread to join
					al_destroy_audio_stream(stream);
					return;
				}

			}
		}
		al_drain_audio_stream(stream);
		al_destroy_audio_stream(stream);

	}

	virtual void playMusic(std::shared_ptr<MusicTrack> track, std::function<void(void*)> finishedCallback, void *callbackData)
	{
		this->stopMusic();
		stopThread = false;
		this->musicThread.reset(new std::thread(musicThreadFunction, std::ref(*this), track, finishedCallback, callbackData));
	}

	virtual void stopMusic()
	{
		if (musicThread)
		{
			stopThread = true;
			musicThread->join();
			musicThread.reset(nullptr);
		}
	}

	virtual ~AllegroSoundBackend()
	{
		this->stopMusic();
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
			LogError("Trying to load multiple allegro sound backends");
			return nullptr;
		}

		if (!al_install_audio())
		{
			LogError("Failed to init allegro sound backend");
			return nullptr;
		}

		if (!al_reserve_samples(max_samples))
		{
			LogError("Failed to reserve sample channels");
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

SoundBackendRegister<AllegroSoundBackendFactory> load_at_init_allegro_sound("allegro");

}; //anonymous namespace

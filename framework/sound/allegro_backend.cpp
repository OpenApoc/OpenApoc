#include "library/sp.h"
#include "framework/sound_interface.h"
#include "framework/logger.h"
#include <iostream>
#include <allegro5/allegro_audio.h>
#include <list>
#include <thread>

namespace
{

static const int max_samples = 1000;

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
	AllegroSampleData(sp<Sample> sample)
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
	// FIXME: Keep last max_samples live as they may still be playing (no way
	// to tell if allegro is finished without managing the stream outselves?)
	std::list<sp<Sample>> liveSamples;

	bool stopThread;
	std::unique_ptr<std::thread> musicThread;

	float globalGain;
	float sampleGain;
	float musicGain;

	sp<MusicTrack> track;

  public:
	AllegroSoundBackend() : stopThread(false), globalGain(1.0), sampleGain(1.0), musicGain(1.0) {}

	virtual float getGain(Gain g) override
	{
		switch (g)
		{
			case Gain::Global:
				return globalGain;
			case Gain::Sample:
				return sampleGain;
			case Gain::Music:
				return musicGain;
			default:
				LogError("Unexpected Gain type");
				return 0.0f;
		}
	}

	virtual void setGain(Gain g, float v) override
	{
		v = std::min(1.0f, v);
		v = std::max(0.0f, v);
		switch (g)
		{
			case Gain::Global:
				globalGain = v;
				break;
			case Gain::Sample:
				sampleGain = v;
				break;
			case Gain::Music:
				musicGain = v;
				break;
			default:
				LogError("Unexpected Gain type");
				break;
		}
	}

	virtual void playSample(sp<Sample> sample, float gain) override
	{
		if (!sample->backendData)
			sample->backendData.reset(new AllegroSampleData(sample));
		liveSamples.push_back(sample);
		if (liveSamples.size() > max_samples)
			liveSamples.pop_front();
		AllegroSampleData *sampleData = static_cast<AllegroSampleData *>(sample->backendData.get());

		LogInfo("Playing sample with gain %f (%f * %f * %f)",
		        this->globalGain * this->sampleGain * gain, this->globalGain, this->sampleGain,
		        gain);
		if (!al_play_sample(sampleData->s, this->globalGain * this->sampleGain * gain, 0.0f, 1.0f,
		                    ALLEGRO_PLAYMODE_ONCE, nullptr))
		{
			LogWarning("Failed to play sample");
		}
	}

	static unsigned int getSampleSize(const AudioFormat &format)
	{
		unsigned int channels;
		switch (format.channels)
		{
			case 1:
				channels = 1;
				break;
			case 2:
				channels = 2;
				break;
			default:
				LogWarning("Unsupported music channels");
				return 0;
		}
		unsigned int bytesPerSample;
		switch (format.format)
		{
			case AudioFormat::SampleFormat::PCM_SINT16:
				bytesPerSample = 2;
				break;
			default:
				LogWarning("Unsupported music format");
				return 0;
		}

		return bytesPerSample * channels;
	}

	static void musicThreadFunction(AllegroSoundBackend &parent,
	                                std::function<void(void *)> finishedCallback,
	                                void *callbackData)
	{
		unsigned int sampleBufferSize;
		ALLEGRO_AUDIO_DEPTH depth;

		auto track = parent.track;
		if (!track)
		{
			LogWarning("Called with no current track");
			return;
		}

		sampleBufferSize = track->requestedSampleBufferSize;

		auto format = track->format;

		switch (format.format)
		{
			case AudioFormat::SampleFormat::PCM_SINT16:
				depth = ALLEGRO_AUDIO_DEPTH_INT16;
				break;
			default:
				LogWarning("AllegroSoundBackend: Unsupported music format");
				return;
		}
		ALLEGRO_CHANNEL_CONF channels;
		switch (format.channels)
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
		ALLEGRO_AUDIO_STREAM *stream =
		    al_create_audio_stream(2, sampleBufferSize, track->format.frequency, depth, channels);
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
				if (!al_set_audio_stream_gain(stream, parent.globalGain * parent.musicGain))
				{
					LogError("Failed to set music stream gain");
				}
				unsigned int samplesLeft = sampleBufferSize;
				uint8_t *buf = reinterpret_cast<uint8_t *>(al_get_audio_stream_fragment(stream));
				if (!buf)
				{
					/* Sometimes we get an event with no buffer? */
					continue;
				}
				while (samplesLeft > 0)
				{
					track = parent.track;
					if (!track)
					{
						LogInfo("No next track, stopping thread ");
						memset(buf, 0, samplesLeft * getSampleSize(format));
						al_set_audio_stream_fragment(stream, buf);
						al_drain_audio_stream(stream);
						al_destroy_audio_stream(stream);
						return;
					}
					if (track->format != format)
					{
						LogError("UNSUPPORTED: Track format changed");
						al_drain_audio_stream(stream);
						al_destroy_audio_stream(stream);
						return;
					}
					unsigned int returnedSamples;
					auto callbackReturn =
					    track->callback(track, samplesLeft, buf, &returnedSamples);
					if (returnedSamples > samplesLeft)
					{
						LogError("Buffer overflow, requested %u samples, got %u", samplesLeft,
						         returnedSamples);
						samplesLeft = 0;
					}
					else
					{
						samplesLeft -= returnedSamples;
					}

					if (callbackReturn == MusicTrack::MusicCallbackReturn::End)
					{
						LogInfo("Progressing track");
						finishedCallback(callbackData);
					}
				}
				al_set_audio_stream_fragment(stream, buf);
			}
		}
		al_drain_audio_stream(stream);
		al_destroy_audio_stream(stream);
	}

	virtual void playMusic(std::function<void(void *)> finishedCallback,
	                       void *callbackData) override
	{
		if (this->musicThread)
		{
			if (stopThread == false)
			{
				LogInfo("Already playing");
				return;
			}
			else
			{
				/* If we happen to catch the thread in the process of shutting down just wait for it
				 * and respawn */
				this->musicThread->join();
			}
		}
		stopThread = false;
		if (!this->track)
		{
			LogWarning("Called with no track");
		}
		this->musicThread.reset(
		    new std::thread(musicThreadFunction, std::ref(*this), finishedCallback, callbackData));
	}

	virtual void stopMusic() override
	{
		this->track = nullptr;
		if (musicThread)
		{
			stopThread = true;
			musicThread->join();
			musicThread.reset(nullptr);
		}
	}

	virtual void setTrack(sp<MusicTrack> track) override
	{
		LogInfo("Setting track to %p", track.get());
		this->track = track;
	}

	virtual ~AllegroSoundBackend() { this->stopMusic(); }

	virtual const AudioFormat &getPreferredFormat() { return preferredFormat; }
};

class AllegroSoundBackendFactory : public SoundBackendFactory
{
	bool initialised;

  public:
	AllegroSoundBackendFactory() : initialised(false) {}
	virtual SoundBackend *create() override
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
	}

	virtual ~AllegroSoundBackendFactory()
	{
		if (initialised)
		{
			al_uninstall_audio();
		}
	}
};

SoundBackendRegister<AllegroSoundBackendFactory> load_at_init_allegro_sound("allegro");

}; // anonymous namespace

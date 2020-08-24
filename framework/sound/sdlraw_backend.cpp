#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound_interface.h"
#include "library/sp.h"
#include "library/vec.h"
#include <SDL.h>
#include <SDL_audio.h>
#include <algorithm>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <vector>

namespace
{

using namespace OpenApoc;

struct SampleData
{
	sp<Sample> sample;
	float gain;
	unsigned int sample_position; // in bytes
	SampleData(sp<Sample> sample, float gain) : sample(sample), gain(gain), sample_position(0) {}
};

struct MusicData
{
	MusicData() : sample_position(0) {}
	std::vector<unsigned char> samples;
	unsigned int sample_position; // in bytes
};

// 'samples' must already be large enough to contain the full output size
static bool ConvertAudio(AudioFormat input_format, SDL_AudioSpec &output_spec, int input_size_bytes,
                         std::vector<unsigned char> &samples)
{
	SDL_AudioCVT cvt;
	SDL_AudioFormat sdl_input_format;
	switch (input_format.format)
	{
		case AudioFormat::SampleFormat::PCM_SINT16:
			sdl_input_format = AUDIO_S16LSB;
			break;
		case AudioFormat::SampleFormat::PCM_UINT8:
			sdl_input_format = AUDIO_U8;
			break;
		default:
			LogWarning("Unknown input sample format");
			return false;
	}

	int ret =
	    SDL_BuildAudioCVT(&cvt, sdl_input_format, input_format.channels, input_format.frequency,
	                      output_spec.format, output_spec.channels, output_spec.freq);
	if (ret < 0)
	{
		LogWarning("Failed to build AudioCVT");
		return false;
	}
	else if (ret == 0)
	{
		// No conversion needed
		return true;
	}
	else
	{
		cvt.len = input_size_bytes;
		size_t neededSize = std::max(1, cvt.len_mult) * cvt.len;
		if (samples.size() < neededSize)
		{
			LogInfo("Expanding sample output buffer from %zu to %zu bytes", samples.size(),
			        neededSize);
			samples.resize(neededSize);
		}
		cvt.buf = (Uint8 *)samples.data();
		SDL_ConvertAudio(&cvt);
		samples.resize(cvt.len_cvt);
		return true;
	}
}

class SDLSampleData : public BackendSampleData
{
  public:
	SDLSampleData(sp<Sample> sample, SDL_AudioSpec &output_spec)
	{
		unsigned long int input_size =
		    sample->format.getSampleSize() * sample->format.channels * sample->sampleCount;
		this->samples.resize(input_size);
		memcpy(this->samples.data(), sample->data.get(), input_size);
		if (!ConvertAudio(sample->format, output_spec, input_size, this->samples))
		{
			LogWarning("Failed to convert sample data");
		}
	}
	~SDLSampleData() override = default;
	std::vector<unsigned char> samples;
};

static void unwrap_callback(void *userdata, Uint8 *stream, int len);

class SDLRawBackend : public SoundBackend
{
	std::recursive_mutex audio_lock;

	float overall_volume;
	float music_volume;
	float sound_volume;

	sp<MusicTrack> track;
	bool music_playing;

	std::function<void(void *)> music_finished_callback;
	void *music_callback_data;

	std::list<SampleData> live_samples;

	SDL_AudioDeviceID devID;
	SDL_AudioSpec output_spec;
	AudioFormat preferred_format;

	sp<MusicData> current_music_data;
	std::queue<sp<MusicData>> music_queue;

	std::shared_future<void> get_music_future;

	unsigned int music_queue_size;

	void getMoreMusic()
	{
		std::lock_guard<std::recursive_mutex> lock(this->audio_lock);
		if (!this->music_playing)
		{
			// Music probably disabled in the time it took us to be scheduled?
			return;
		}
		if (!this->track)
		{
			LogWarning("Music playing but no track?");
			return;
		}
		while (this->music_queue.size() < this->music_queue_size)
		{
			auto data = mksp<MusicData>();

			unsigned int input_size = this->track->format.getSampleSize() *
			                          this->track->format.channels *
			                          this->track->requestedSampleBufferSize;
			unsigned int output_size = (SDL_AUDIO_BITSIZE(this->output_spec.format) / 8) *
			                           this->output_spec.channels *
			                           this->track->requestedSampleBufferSize;

			data->samples.resize(std::max(input_size, output_size));

			unsigned int returned_samples;

			auto ret = this->track->callback(this->track, this->track->requestedSampleBufferSize,
			                                 (void *)data->samples.data(), &returned_samples);

			// Reset the sizes, as we may have fewer returned_samples than asked for
			input_size = this->track->format.getSampleSize() * this->track->format.channels *
			             returned_samples;
			output_size = (SDL_AUDIO_BITSIZE(this->output_spec.format) / 8) *
			              this->output_spec.channels * returned_samples;

			bool convert_ret =
			    ConvertAudio(this->track->format, this->output_spec, input_size, data->samples);
			if (!convert_ret)
			{
				LogWarning("Failed to convert music data");
			}

			if (ret == MusicTrack::MusicCallbackReturn::End)
			{
				this->track = nullptr;
				if (this->music_finished_callback)
				{
					this->music_finished_callback(music_callback_data);
				}
			}
			this->music_queue.push(data);
		}
	}

  public:
	void mixingCallback(Uint8 *stream, int len)
	{
		// initialize stream buffer
		memset(stream, 0, len);
		std::lock_guard<std::recursive_mutex> lock(this->audio_lock);

		if (this->current_music_data || !this->music_queue.empty())
		{
			int int_music_volume =
			    clamp((int)lrint(this->overall_volume * this->music_volume * 128.0f), 0, 128);
			int music_bytes = 0;
			while (music_bytes < len)
			{
				if (!this->current_music_data)
				{
					this->get_music_future =
					    fw().threadPoolEnqueue(std::mem_fn(&SDLRawBackend::getMoreMusic), this);
					if (this->music_queue.empty())
					{
						LogWarning("Music underrun!");
						break;
					}
					this->current_music_data = this->music_queue.front();
					this->music_queue.pop();
				}
				int bytes_from_this_chunk =
				    std::min(len - music_bytes, (int)(this->current_music_data->samples.size() -
				                                      this->current_music_data->sample_position));
				SDL_MixAudioFormat(stream + music_bytes,
				                   this->current_music_data->samples.data() +
				                       this->current_music_data->sample_position,
				                   this->output_spec.format, bytes_from_this_chunk,
				                   int_music_volume);
				music_bytes += bytes_from_this_chunk;
				this->current_music_data->sample_position += bytes_from_this_chunk;
				LogAssert(this->current_music_data->sample_position <=
				          this->current_music_data->samples.size());
				if (this->current_music_data->sample_position ==
				    this->current_music_data->samples.size())
				{
					this->current_music_data = nullptr;
				}
			}
		}

		auto sampleIt = this->live_samples.begin();
		while (sampleIt != this->live_samples.end())
		{
			int int_sample_volume = clamp(
			    (int)lrint(this->overall_volume * this->sound_volume * sampleIt->gain * 128.0f), 0,
			    128);
			auto sampleData =
			    std::dynamic_pointer_cast<SDLSampleData>(sampleIt->sample->backendData);
			if (!sampleData)
			{
				LogWarning("Sample with invalid sample data");
				// Clear it in case we've changed drivers or something
				sampleIt->sample->backendData = nullptr;
				sampleIt = this->live_samples.erase(sampleIt);
				continue;
			}
			if (sampleIt->sample_position == sampleData->samples.size())
			{
				// Reached the end of the sample
				sampleIt = this->live_samples.erase(sampleIt);
				continue;
			}

			unsigned bytes_to_mix =
			    std::min(len, (int)(sampleData->samples.size() - sampleIt->sample_position));
			SDL_MixAudioFormat(stream, sampleData->samples.data() + sampleIt->sample_position,
			                   this->output_spec.format, bytes_to_mix, int_sample_volume);
			sampleIt->sample_position += bytes_to_mix;
			LogAssert(sampleIt->sample_position <= sampleData->samples.size());

			sampleIt++;
		}
	}

	SDLRawBackend()
	    : overall_volume(1.0f), music_volume(1.0f), sound_volume(1.0f), music_playing(false),
	      music_callback_data(nullptr), music_queue_size(2)
	{
		SDL_Init(SDL_INIT_AUDIO);
		preferred_format.channels = 2;
		preferred_format.format = AudioFormat::SampleFormat::PCM_SINT16;
		preferred_format.frequency = 22050;
		LogInfo("Current audio driver: %s", SDL_GetCurrentAudioDriver());
		LogWarning("Changing audio drivers is not currently implemented!");
		int numDevices = SDL_GetNumAudioDevices(0); // Request playback devices only
		LogInfo("Number of audio devices: %d", numDevices);
		for (int i = 0; i < numDevices; ++i)
		{
			LogInfo("Device %d: %s", i, SDL_GetAudioDeviceName(i, 0));
		}
		LogWarning(
		    "Selecting audio devices not currently implemented! Selecting first available device.");
		const char *deviceName = SDL_GetAudioDeviceName(0, 0);
		LogInfo("Using audio device: %s", deviceName);
		SDL_AudioSpec wantFormat;
		wantFormat.channels = 2;
		wantFormat.format = AUDIO_S16LSB;
		wantFormat.freq = 22050;
		wantFormat.samples = 512;
		wantFormat.callback = unwrap_callback;
		wantFormat.userdata = this;
		devID = SDL_OpenAudioDevice(
		    nullptr, // "NULL" here is a 'reasonable default', which uses the platform default when
		             // available
		    0,       // capturing is not supported
		    &wantFormat, &output_spec,
		    SDL_AUDIO_ALLOW_ANY_CHANGE); // hopefully we'll get a sane output format
		SDL_PauseAudioDevice(devID, 0);  // Run at once?

		LogWarning("Audio output format: Channels %d, format: %s %s %s %dbit, freq %d, samples %d",
		           (int)output_spec.channels,
		           SDL_AUDIO_ISSIGNED(output_spec.format) ? "signed" : "unsigned",
		           SDL_AUDIO_ISFLOAT(output_spec.format) ? "float" : "int",
		           SDL_AUDIO_ISBIGENDIAN(output_spec.format) ? "BE" : "LE",
		           SDL_AUDIO_BITSIZE(output_spec.format), (int)output_spec.freq,
		           (int)output_spec.samples);
	}
	void playSample(sp<Sample> sample, float gain) override
	{
		// Clamp to 0..1
		gain = std::min(1.0f, std::max(0.0f, gain));
		if (!sample->backendData)
		{
			sample->backendData.reset(new SDLSampleData(sample, this->output_spec));
		}
		{
			std::lock_guard<std::recursive_mutex> l(this->audio_lock);
			this->live_samples.emplace_back(sample, gain);
		}
		LogInfo("Placed sound %s on queue", sample->path);
	}

	void playMusic(std::function<void(void *)> finishedCallback, void *callbackData) override
	{
		std::lock_guard<std::recursive_mutex> l(this->audio_lock);
		while (!music_queue.empty())
			music_queue.pop();
		music_finished_callback = finishedCallback;
		music_callback_data = callbackData;
		music_playing = true;
		this->get_music_future =
		    fw().threadPoolEnqueue(std::mem_fn(&SDLRawBackend::getMoreMusic), this);
		LogInfo("Playing music on SDL backend");
	}

	void setTrack(sp<MusicTrack> track) override
	{
		std::lock_guard<std::recursive_mutex> l(this->audio_lock);
		LogInfo("Setting track to %s", track->path);
		this->track = track;
		while (!music_queue.empty())
			music_queue.pop();
	}

	void stopMusic() override
	{
		std::shared_future<void> outstanding_get_music;
		{
			std::lock_guard<std::recursive_mutex> l(this->audio_lock);
			this->music_playing = false;
			this->track = nullptr;
			if (this->get_music_future.valid())
				outstanding_get_music = std::move(this->get_music_future);
			while (!music_queue.empty())
				music_queue.pop();
		}
		if (outstanding_get_music.valid())
			outstanding_get_music.wait();
	}

	~SDLRawBackend() override
	{
		// Lock the device and stop any outstanding music threads to ensure everything is dead
		// before destroying the device
		SDL_LockAudioDevice(devID);
		SDL_PauseAudioDevice(devID, 1);
		this->stopMusic();
		SDL_UnlockAudioDevice(devID);
		SDL_CloseAudioDevice(devID);
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}

	virtual const AudioFormat &getPreferredFormat() { return preferred_format; }

	float getGain(Gain g) override
	{
		float reqGain = 0;
		switch (g)
		{
			case Gain::Global:
				reqGain = overall_volume;
				break;
			case Gain::Sample:
				reqGain = sound_volume;
				break;
			case Gain::Music:
				reqGain = music_volume;
				break;
		}
		return reqGain;
	}
	void setGain(Gain g, float f) override
	{
		// Clamp to 0..1
		f = std::min(1.0f, std::max(0.0f, f));
		switch (g)
		{
			case Gain::Global:
				overall_volume = f;
				break;
			case Gain::Sample:
				sound_volume = f;
				break;
			case Gain::Music:
				music_volume = f;
				break;
		}
	}
};

void unwrap_callback(void *userdata, Uint8 *stream, int len)
{
	auto backend = reinterpret_cast<SDLRawBackend *>(userdata);
	backend->mixingCallback(stream, len);
}

class SDLRawBackendFactory : public SoundBackendFactory
{
  public:
	SoundBackend *create() override
	{
		LogWarning("Creating SDLRaw sound backend (Might have issues!)");
		int ret = SDL_InitSubSystem(SDL_INIT_AUDIO);
		if (ret < 0)
		{
			LogWarning("Failed to init SDL_AUDIO (%d) - %s", ret, SDL_GetError());
			return nullptr;
		}
		return new SDLRawBackend();
	}

	~SDLRawBackendFactory() override = default;
};

}; // anonymous namespace

namespace OpenApoc
{
SoundBackendFactory *getSDLSoundBackend() { return new SDLRawBackendFactory(); }
} // namespace OpenApoc

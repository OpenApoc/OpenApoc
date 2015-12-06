#include "library/sp.h"
#include "framework/sound_interface.h"
#include "framework/logger.h"

namespace
{

using namespace OpenApoc;

class NullSoundBackend : public SoundBackend
{
	AudioFormat preferredFormat;

  public:
	NullSoundBackend()
	{
		preferredFormat.channels = 2;
		preferredFormat.format = AudioFormat::SampleFormat::PCM_SINT16;
		preferredFormat.frequency = 22050;
	}
	virtual void playSample(sp<Sample> sample, float gain) override
	{
		std::ignore = sample;
		std::ignore = gain;
		LogWarning("Called on NULL backend");
	}

	virtual void playMusic(std::function<void(void *)> finishedCallback,
	                       void *callbackData) override
	{
		std::ignore = finishedCallback;
		std::ignore = callbackData;
		LogWarning("Called on NULL backend");
	}

	virtual void setTrack(sp<MusicTrack> track) override
	{
		std::ignore = track;
		LogWarning("Called on NULL backend");
	}

	virtual void stopMusic() override { LogWarning("Called on NULL backend"); }

	virtual ~NullSoundBackend() { this->stopMusic(); }

	virtual const AudioFormat &getPreferredFormat() { return preferredFormat; }

	virtual float getGain(Gain g) override
	{
		std::ignore = g;
		return 0.0;
	}
	virtual void setGain(Gain g, float f) override
	{
		std::ignore = g;
		std::ignore = f;
		return;
	}
};

class NullSoundBackendFactory : public SoundBackendFactory
{
  public:
	virtual SoundBackend *create() override
	{
		LogWarning("Creating NULL sound backend (Sound disabled)");
		return new NullSoundBackend();
	}

	virtual ~NullSoundBackendFactory() {}
};

SoundBackendRegister<NullSoundBackendFactory> load_at_init_null_sound("null");

}; // anonymous namespace

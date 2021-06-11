#include "framework/logger.h"
#include "framework/sound_interface.h"
#include "library/sp.h"

namespace
{

using namespace OpenApoc;

class NullSoundBackend : public SoundBackend
{
	AudioFormat preferredFormat;

  public:
	NullSoundBackend(int concurrent_sample_count) : SoundBackend(concurrent_sample_count)
	{
		preferredFormat.channels = 2;
		preferredFormat.format = AudioFormat::SampleFormat::PCM_SINT16;
		preferredFormat.frequency = 22050;
	}
	void playSample(sp<Sample> sample, float gain) override
	{
		std::ignore = sample;
		std::ignore = gain;
		LogInfo("Called on NULL backend");
	}

	void playMusic(std::function<void(void *)> finishedCallback, void *callbackData) override
	{
		std::ignore = finishedCallback;
		std::ignore = callbackData;
		LogInfo("Called on NULL backend");
	}

	void setTrack(sp<MusicTrack> track) override
	{
		std::ignore = track;
		LogInfo("Called on NULL backend");
	}

	void stopMusic() override { LogInfo("Called on NULL backend"); }

	~NullSoundBackend() override { this->stopMusic(); }

	virtual const AudioFormat &getPreferredFormat() { return preferredFormat; }

	float getGain(Gain g) override
	{
		std::ignore = g;
		return 0.0;
	}
	void setGain(Gain g, float f) override
	{
		std::ignore = g;
		std::ignore = f;
	}
};

class NullSoundBackendFactory : public SoundBackendFactory
{
  public:
	SoundBackend *create(int concurrent_sample_count) override
	{
		LogWarning("Creating NULL sound backend (Sound disabled)");
		return new NullSoundBackend(concurrent_sample_count);
	}

	~NullSoundBackendFactory() override = default;
};

}; // anonymous namespace

namespace OpenApoc
{
SoundBackendFactory *getNullSoundBackend() { return new NullSoundBackendFactory(); }
} // namespace OpenApoc

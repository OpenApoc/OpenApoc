#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "library/strings.h"

namespace OpenApoc
{

class AudioFormat
{
  public:
	int frequency;
	int channels;
	enum class SampleFormat {
		PCM_SINT16,
		PCM_UINT8,
	};
	SampleFormat format;
};

class BackendSampleData
{
  public:
	virtual ~BackendSampleData() {}
};

class Sample
{
  public:
	AudioFormat format;
	unsigned int sampleCount;
	std::unique_ptr<uint8_t[]> data;
	std::unique_ptr<BackendSampleData> backendData;

	virtual ~Sample(){};
};

class MusicTrack
{
  public:
	unsigned int sampleCount; // may be estimated? Or 0 if we just don't know?
	unsigned int requestedSampleBufferSize;
	AudioFormat format;

	enum class MusicCallbackReturn {
		End,      // This track ends after *returnedSamples have been played
		Continue, // There is more to come, even if we didn't fill the output buffer
	};

	std::function<MusicCallbackReturn(std::shared_ptr<MusicTrack> thisTrack,
	                                  unsigned int maxSamples, void *sampleBuffer,
	                                  unsigned int *returnedSamples)> callback;

	virtual ~MusicTrack(){};
};

class SoundBackend
{
  public:
	virtual ~SoundBackend(){};
	virtual void playSample(std::shared_ptr<Sample> sample) = 0;
	virtual void playMusic(std::shared_ptr<MusicTrack>,
	                       std::function<void(void *)> finishedCallback,
	                       void *callbackData = nullptr) = 0;
	virtual void stopMusic() = 0;

	/* Gain - a float scale (from 1.0 to 0.0) in 'linear intensity' (IE samples
	 * are simply multiplied by the 'volume')
	 *
	 * The Global volume is applied to both Sample and Music.
	 * IE music is multiplied by (Gain::Music * Gain::Global)
	 * and samples by (Gain::Sample * Gain::Global)
	 *
	 * No claims as to the backend accuracy of this, though.
	 */

	enum class Gain {
		Sample,
		Music,
		Global,
	};

	virtual float getGain(Gain g) = 0;
	/* Any values outside the range 0..1 will be clamped */
	virtual void setGain(Gain g, float v) = 0;
};

class JukeBox
{
  public:
	enum class PlayMode {
		Once,
		Loop,
	};
	virtual ~JukeBox(){};
	virtual void play(std::vector<UString> tracks, PlayMode mode = PlayMode::Loop) = 0;
	virtual void stop() = 0;
};
};

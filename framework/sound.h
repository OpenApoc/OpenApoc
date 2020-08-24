#pragma once

#include "library/resource.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <functional>
#include <vector>

namespace OpenApoc
{

class AudioFormat
{
  public:
	int frequency;
	int channels;
	enum class SampleFormat
	{
		PCM_SINT16,
		PCM_UINT8,
	};
	SampleFormat format;
	bool operator==(const AudioFormat &other) const
	{
		return (this->frequency == other.frequency && this->channels == other.channels &&
		        this->format == other.format);
	}
	bool operator!=(const AudioFormat &other) const { return !(*this == other); }

	int getSampleSize() const
	{
		switch (format)
		{
			case SampleFormat::PCM_SINT16:
				return 2;
			case SampleFormat::PCM_UINT8:
				return 1;
			default:
				// Invalid sample format
				return 0;
		}
	}
};

class BackendSampleData
{
  public:
	virtual ~BackendSampleData() = default;
};

class Sample : public ResObject
{
  public:
	AudioFormat format;
	// The samplecount is for a single channel - e.g. having 2 channels means there's
	// (2*sampleCount) total samples in *data
	unsigned int sampleCount;
	std::unique_ptr<uint8_t[]> data;
	sp<BackendSampleData> backendData;

	virtual ~Sample() = default;
};

class MusicTrack : public ResObject
{
  public:
	// unsigned int sampleCount; // may be estimated? Or 0 if we just don't know?
	unsigned int requestedSampleBufferSize;
	AudioFormat format;

	enum class MusicCallbackReturn
	{
		End,      // This track ends after *returnedSamples have been played
		Continue, // There is more to come, even if we didn't fill the output buffer
	};

	std::function<MusicCallbackReturn(sp<MusicTrack> track, unsigned int maxSamples,
	                                  void *sampleBuffer, unsigned int *returnedSamples)>
	    callback;
	virtual const UString &getName() const = 0;
	virtual ~MusicTrack() = default;
};

class SoundBackend
{
  public:
	virtual ~SoundBackend() = default;
	virtual void playSample(sp<Sample> sample, float gain = 1.0f) = 0;
	virtual void playMusic(std::function<void(void *)> finishedCallback,
	                       void *callbackData = nullptr) = 0;
	virtual void stopMusic() = 0;

	virtual void setTrack(sp<MusicTrack> track) = 0;

	/* Gain - a float scale (from 1.0 to 0.0) in 'linear intensity' (IE samples
	 * are simply multiplied by the 'volume')
	 *
	 * The Global volume is applied to both Sample and Music.
	 * IE music is multiplied by (Gain::Music * Gain::Global)
	 * and samples by (Gain::Sample * Gain::Global)
	 *
	 * No claims as to the backend accuracy of this, though.
	 */

	enum class Gain
	{
		Sample,
		Music,
		Global,
	};

	virtual float getGain(Gain g) = 0;
	/* Any values outside the range 0..1 will be clamped */
	virtual void setGain(Gain g, float v) = 0;

	Vec3<float> listenerPosition;
	/* A quick attempt at 'positional' audio */
	virtual void playSample(sp<Sample> sample, Vec3<float> position, float gainMultiplier = 1.0f);
	virtual void setListenerPosition(Vec3<float> position);
};

}; // namespace OpenApoc

#pragma once
#include <functional>
#include <memory>

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
};

class BackendSampleData
{
public:
	virtual ~BackendSampleData(){}
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
	unsigned int sampleCount; //may be estimated? Or 0 if we just don't know?
	unsigned int requestedSampleBufferSize;
	AudioFormat format;

	enum class MusicCallbackReturn
	{
		End, //This track ends after *returnedSamples have been played
		Continue, //There is more to come, even if we didn't fill the output buffer
	};

	std::function<MusicCallbackReturn (std::shared_ptr<MusicTrack> thisTrack, unsigned int maxSamples, void * sampleBuffer, unsigned int * returnedSamples) > callback;

	virtual ~MusicTrack(){};
};


class SoundBackend
{
public:
	virtual ~SoundBackend() {};
	virtual void playSample(std::shared_ptr<Sample> sample) = 0;
	virtual void playMusic(std::shared_ptr<MusicTrack>, std::function<void(void*)> finishedCallback, void *callbackData = nullptr) = 0;
};

};

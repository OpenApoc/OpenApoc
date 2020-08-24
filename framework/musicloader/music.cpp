#include "framework/data.h"
#include "framework/logger.h"
#include "framework/musicloader_interface.h"
#include "framework/sound.h"
#include "library/sp.h"
#include <algorithm>
#include <memory>
#include <vector>

namespace
{

using namespace OpenApoc;

const int MusicChannels = 2;
const int MusicBytesPerSample = 2;

MusicTrack::MusicCallbackReturn fillMusicData(sp<MusicTrack> thisTrack, unsigned int maxSamples,
                                              void *sampleBuffer, unsigned int *returnedSamples);

class RawMusicTrack : public MusicTrack
{
	IFile file;
	unsigned int samplePosition;
	unsigned int startingPosition;
	unsigned int sampleCount = 0;
	bool valid;
	UString name;

  public:
	RawMusicTrack(Data &data, const UString &name, const UString &fileName, unsigned int fileOffset,
	              unsigned int numSamples)
	    : file(data.fs.open(fileName)), samplePosition(0), startingPosition(fileOffset),
	      valid(false), name(name)
	{
		if (!file)
		{
			LogError("Failed to open file \"%s\"", fileName);
			return;
		}
		if (file.size() < fileOffset + (numSamples * MusicChannels * MusicBytesPerSample))
		{
			LogError("File \"%s\" insufficient size for offset %u + size %u - returned size %zu",
			         fileName, fileOffset, numSamples * MusicChannels * MusicBytesPerSample,
			         file.size());
			return;
		}
		if (!file.seekg(fileOffset))
		{
			LogError("Failed to seek to offset %u", fileOffset);
			return;
		}

		valid = true;
		this->sampleCount = numSamples;
		this->format.frequency = 22050;
		this->format.channels = 2;
		this->format.format = AudioFormat::SampleFormat::PCM_SINT16;
		this->callback = fillMusicData;

		// Ask for 1/10 a second of buffer by default
		// Increase this for
		this->requestedSampleBufferSize = this->format.frequency / 10;
	}
	const UString &getName() const override { return this->name; }
	MusicCallbackReturn fillData(unsigned int maxSamples, void *sampleBuffer,
	                             unsigned int *returnedSamples)
	{
		if (!valid)
		{
			LogError("Playing invalid file \"%s\"", file.fileName());
			*returnedSamples = 0;
			return MusicCallbackReturn::End;
		}

		unsigned int samples = std::min(maxSamples, this->sampleCount - this->samplePosition);

		this->samplePosition += samples;

		if (!file.read(reinterpret_cast<char *>(sampleBuffer),
		               samples * MusicBytesPerSample * MusicChannels))
		{
			LogError("Failed to read sample data in \"%s\"", file.fileName());
			this->valid = false;
			samples = 0;
		}
		*returnedSamples = samples;
		if (samples < maxSamples)
		{
			// Prepare this track to be reused
			if (!file.seekg(startingPosition))
			{
				LogWarning("Could not rewind track %s", name);
			}
			samplePosition = 0;
			return MusicCallbackReturn::End;
		}
		return MusicCallbackReturn::Continue;
	}

	~RawMusicTrack() override = default;
};

MusicTrack::MusicCallbackReturn fillMusicData(sp<MusicTrack> thisTrack, unsigned int maxSamples,
                                              void *sampleBuffer, unsigned int *returnedSamples)
{
	auto track = std::dynamic_pointer_cast<RawMusicTrack>(thisTrack);
	LogAssert(track);
	return track->fillData(maxSamples, sampleBuffer, returnedSamples);
}

class RawMusicLoader : public MusicLoader
{
	Data &data;

  public:
	RawMusicLoader(Data &data) : data(data) {}
	~RawMusicLoader() override = default;

	sp<MusicTrack> loadMusic(UString path) override
	{
		auto strings = split(path, ":");
		// Expected format: "rawmusic:file:start_byte_offset:byte_size"
		if (strings.size() != 4)
		{
			LogInfo("Invalid raw music path string \"%s\"", path);
			return nullptr;
		}
		if (strings[0] != "rawmusic")
		{
			LogInfo("Not rawmusic path: \"%s\"", path);
			return nullptr;
		}
		if (!Strings::isInteger(strings[2]))
		{
			LogInfo("Raw music track \"%s\" start offset \"%s\" doesn't look like a number", path,
			        strings[2]);
			return nullptr;
		}
		if (!Strings::isInteger(strings[3]))
		{
			LogInfo("Raw music track \"%s\" length \"%s\" doesn't look like a number", path,
			        strings[3]);
			return nullptr;
		}

		unsigned int offset = Strings::toInteger(strings[2]);
		unsigned int length = Strings::toInteger(strings[3]);
		return mksp<RawMusicTrack>(data, path, strings[1], offset,
		                           length / (MusicChannels * MusicBytesPerSample));
	}
};

class RawMusicLoaderFactory : public MusicLoaderFactory
{
  public:
	MusicLoader *create(Data &data) override { return new RawMusicLoader(data); }
};

}; // anonymous namespace

namespace OpenApoc
{
MusicLoaderFactory *getRAWMusicLoaderFactory() { return new RawMusicLoaderFactory; }
} // namespace OpenApoc

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

const std::vector<unsigned int> offsets = {
    0,         8413184,   19636224,  35930112,  46612480,  57921536,  72122368,
    84097024,  92616704,  104065024, 107757568, 118507520, 130648064, 142123008,
    154044416, 165922816, 176650240, 187213824, 196755456, 207228928, 218535936,
    231469056, 234115072, 237191168, 239837184, 242477056, 245127168, 247859200,
    250505216, 263354368, 275763200, 287817728, 298854400};
const std::vector<unsigned int> lengths = {
    8413184,  11223040, 16293888, 10682368, 11309056, 14200832, 11974656, 8519680,  11448320,
    3692544,  10749952, 12140544, 11474944, 11921408, 11878400, 10727424, 10563584, 9541632,
    10473472, 11307008, 12933120, 2646016,  3076096,  2646016,  2639872,  2650112,  2732032,
    2646016,  12849152, 12408832, 12054528, 11036672, 12834816};

const int MusicChannels = 2;
const int MusicBytesPerSample = 2;

MusicTrack::MusicCallbackReturn fillMusicData(sp<MusicTrack> thisTrack, unsigned int maxSamples,
                                              void *sampleBuffer, unsigned int *returnedSamples);

class RawMusicTrack : public MusicTrack
{
	IFile file;
	unsigned int samplePosition;
	unsigned int startingPosition;
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
		auto strings = path.split(':');
		if (strings.size() != 2)
		{
			LogInfo("Invalid raw music path string \"%s\"", path);
			return nullptr;
		}

		if (!Strings::isInteger(strings[1]))
		{
			LogInfo("Raw music track \"%s\" doesn't look like a number", strings[1]);
			return nullptr;
		}

		unsigned int track = Strings::toInteger(strings[1]);
		if (track > lengths.size())
		{
			LogInfo("Raw music track %d out of bounds", track);
			return nullptr;
		}
		return mksp<RawMusicTrack>(data, path, strings[0], offsets[track],
		                           lengths[track] / (MusicChannels * MusicBytesPerSample));
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

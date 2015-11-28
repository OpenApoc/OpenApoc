#include "library/sp.h"
#include "framework/logger.h"
#include "framework/musicloader_interface.h"
#include "framework/data.h"
#include <algorithm>
#include <vector>
#include <memory>

namespace
{

using namespace OpenApoc;

static const std::vector<unsigned int> starts = {
    0,         8467200,   19580400,  35897400,  40131000,  46569600,  57947400,
    72147600,  84142800,  92610000,  104076000, 107780400, 118540800, 130712400,
    142090200, 154085400, 165904200, 176664600, 187248600, 196686000, 207270000,
    218559600, 231436800, 234082800, 237169800, 239815800, 242461800, 245107800,
    247842000, 250488000, 263365200, 275801400, 287796600, 298821600, 304819200};
static const std::vector<unsigned int> ends = {
    8202600,   19404000,  35897400,  40131000,  46569600,  57859200,  72147600,
    84054600,  92610000,  103987800, 107780400, 118452600, 130536000, 142090200,
    153997200, 165816000, 176488200, 187072200, 196686000, 207093600, 218383200,
    231348600, 234082800, 237169800, 239815800, 242461800, 245107800, 247842000,
    250488000, 263188800, 275713200, 287708400, 298821600, 304731000, 311434200};

static const int MusicChannels = 2;
static const int MusicBytesPerSample = 2;

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
	    : file(data.fs.open(fileName)), samplePosition(0), valid(false), name(name),
	      startingPosition(fileOffset)
	{
		if (!file)
		{
			LogError("Failed to open file \"%s\"", fileName.c_str());
			return;
		}
		if (file.size() < fileOffset + (numSamples * MusicChannels * MusicBytesPerSample))
		{
			LogError("File \"%s\" insufficient size for offset %u + size %zu - returned size %zu",
			         fileName.c_str(), fileOffset, numSamples * MusicChannels * MusicBytesPerSample,
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
	virtual const UString &getName() const override { return this->name; }
	MusicTrack::MusicCallbackReturn fillData(unsigned int maxSamples, void *sampleBuffer,
	                                         unsigned int *returnedSamples)
	{
		if (!valid)
		{
			LogError("Playing invalid file \"%s\"", file.fileName().c_str());
			*returnedSamples = 0;
			return MusicTrack::MusicCallbackReturn::End;
		}

		unsigned int samples = std::min(maxSamples, this->sampleCount - this->samplePosition);

		this->samplePosition += samples;

		if (!file.read(reinterpret_cast<char *>(sampleBuffer),
		               samples * MusicBytesPerSample * MusicChannels))
		{
			LogError("Failed to read sample data in \"%s\"", file.fileName().c_str());
			this->valid = false;
			samples = 0;
		}
		*returnedSamples = samples;
		if (samples < maxSamples)
		{
			// Prepare this track to be reused
			if (!file.seekg(startingPosition))
			{
				LogWarning("Could not rewind track %s", name.c_str());
			}
			samplePosition = 0;
			return MusicTrack::MusicCallbackReturn::End;
		}
		return MusicTrack::MusicCallbackReturn::Continue;
	}

	virtual ~RawMusicTrack() {}
};

MusicTrack::MusicCallbackReturn fillMusicData(sp<MusicTrack> thisTrack, unsigned int maxSamples,
                                              void *sampleBuffer, unsigned int *returnedSamples)
{
	auto track = std::dynamic_pointer_cast<RawMusicTrack>(thisTrack);
	assert(track);
	return track->fillData(maxSamples, sampleBuffer, returnedSamples);
}

class RawMusicLoader : public MusicLoader
{
	Data &data;

  public:
	RawMusicLoader(Data &data) : data(data) {}
	virtual ~RawMusicLoader() {}

	virtual sp<MusicTrack> loadMusic(UString path) override
	{
		auto strings = path.split(':');
		if (strings.size() != 2)
		{
			LogInfo("Invalid raw music path string \"%s\"", path.c_str());
			return nullptr;
		}

		if (!Strings::IsInteger(strings[1]))
		{
			LogInfo("Raw music track \"%s\" doesn't look like a number", strings[1].c_str());
			return nullptr;
		}

		unsigned int track = Strings::ToInteger(strings[1]);
		if (track > ends.size())
		{
			LogInfo("Raw music track %d out of bounds", track);
			return nullptr;
		}
		return std::make_shared<RawMusicTrack>(data, path, strings[0], starts[track],
		                                       (ends[track] - starts[track]) /
		                                           (MusicChannels * MusicBytesPerSample));
	}
};

class RawMusicLoaderFactory : public MusicLoaderFactory
{
  public:
	virtual MusicLoader *create(Data &data) override { return new RawMusicLoader(data); }
};

MusicLoaderRegister<RawMusicLoaderFactory> load_at_init_raw_music("raw");

}; // anonymous namespace

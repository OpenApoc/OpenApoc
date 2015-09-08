#include "framework/logger.h"
#include "framework/musicloader_interface.h"
#include "framework/framework.h"
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
static const std::vector<unsigned int> lengths = {
    8202600,   19404000,  35897400,  40131000,  46569600,  57859200,  72147600,
    84054600,  92610000,  103987800, 107780400, 118452600, 130536000, 142090200,
    153997200, 165816000, 176488200, 187072200, 196686000, 207093600, 218383200,
    231348600, 234082800, 237169800, 239815800, 242461800, 245107800, 247842000,
    250488000, 263188800, 275713200, 287708400, 298821600, 304731000, 311434200};

static const int MusicChannels = 2;
static const int MusicBytesPerSample = 2;

MusicTrack::MusicCallbackReturn fillMusicData(std::shared_ptr<MusicTrack> thisTrack,
                                              unsigned int maxSamples, void *sampleBuffer,
                                              unsigned int *returnedSamples);

class RawMusicTrack : public MusicTrack
{
	IFile file;
	unsigned int samplePosition;
	bool valid;

  public:
	RawMusicTrack(Data &data, const UString &fileName, unsigned int fileOffset,
	              unsigned int numSamples)
	    : file(data.load_file(fileName)), samplePosition(0), valid(false)
	{
		if (!file)
		{
			LogError("Failed to open file \"%s\"", fileName.str().c_str());
			return;
		}
		if (file.size() < fileOffset + (numSamples * MusicChannels * MusicBytesPerSample))
		{
			LogError("File \"%s\" insufficient size for offset %u + size %zu - returned size %zu",
			         fileName.str().c_str(), fileOffset,
			         numSamples * MusicChannels * MusicBytesPerSample, file.size());
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

		// Ask for 1/2 a second of buffer by default
		this->requestedSampleBufferSize = this->format.frequency / 2;
	}
	MusicTrack::MusicCallbackReturn fillData(unsigned int maxSamples, void *sampleBuffer,
	                                         unsigned int *returnedSamples)
	{
		if (!valid)
		{
			LogError("Playing invalid file \"%s\"", file.fileName().str().c_str());
			*returnedSamples = 0;
			return MusicTrack::MusicCallbackReturn::End;
		}

		unsigned int samples = std::min(maxSamples, this->sampleCount - this->samplePosition);

		this->samplePosition += samples;

		if (!file.read((char *)sampleBuffer, samples * MusicBytesPerSample * MusicChannels))
		{
			LogError("Failed to read sample data in \"%s\"", file.fileName().str().c_str());
			this->valid = false;
			samples = 0;
		}
		*returnedSamples = samples;
		if (samples < maxSamples)
			return MusicTrack::MusicCallbackReturn::End;
		return MusicTrack::MusicCallbackReturn::Continue;
	}

	virtual ~RawMusicTrack() {}
};

MusicTrack::MusicCallbackReturn fillMusicData(std::shared_ptr<MusicTrack> thisTrack,
                                              unsigned int maxSamples, void *sampleBuffer,
                                              unsigned int *returnedSamples)
{
	auto track = std::dynamic_pointer_cast<RawMusicTrack>(thisTrack);
	assert(track);
	return track->fillData(maxSamples, sampleBuffer, returnedSamples);
}

class RawMusicLoader : public MusicLoader
{
	Framework &fw;

  public:
	RawMusicLoader(Framework &fw) : fw(fw) {}
	virtual ~RawMusicLoader() {}

	virtual std::shared_ptr<MusicTrack> loadMusic(UString path) override
	{
		auto strings = path.split(':');
		if (strings.size() != 2)
		{
			LogInfo("Invalid raw music path string \"%s\"", path.str().c_str());
			return nullptr;
		}

		if (!Strings::IsInteger(strings[1]))
		{
			LogInfo("Raw music track \"%s\" doesn't look like a number", strings[1].str().c_str());
			return nullptr;
		}

		unsigned int track = Strings::ToInteger(strings[1]);
		if (track > lengths.size())
		{
			LogInfo("Raw music track %d out of bounds", track);
			return nullptr;
		}
		return std::make_shared<RawMusicTrack>(*fw.data, strings[0], starts[track],
		                                       lengths[track] / MusicChannels /
		                                           MusicBytesPerSample);
	}
};

class RawMusicLoaderFactory : public MusicLoaderFactory
{
  public:
	virtual MusicLoader *create(Framework &fw) override { return new RawMusicLoader(fw); }
};

MusicLoaderRegister<RawMusicLoaderFactory> load_at_init_raw_music("raw");

}; // anonymous namespace

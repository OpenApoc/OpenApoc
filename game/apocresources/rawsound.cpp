#include "library/sp.h"
#include "framework/sampleloader_interface.h"
#include "framework/data.h"
#include "framework/logger.h"

#include <set>

namespace
{

using namespace OpenApoc;

static std::set<int> allowedSampleRates = {
    11025, 22050,
};

class RawSampleLoader : public SampleLoader
{
	Data &data;

  public:
	RawSampleLoader(Data &data) : data(data) {}
	virtual sp<Sample> loadSample(UString path) override
	{
		// Raw sound format strings come in the form:
		// RAWSOUND:FILENAME:SAMPLERATE
		// They are all assumed to be PCK_UINT8 1channel

		auto splitString = path.split(':');
		if (splitString.size() != 3)
		{
			LogInfo("String \"%s\" doesn't look like a rawsample - need 3 elements (got %d)",
			        path.c_str(), splitString.size());
			return nullptr;
		}
		if (splitString[0] != "RAWSOUND")
		{
			LogInfo("String \"%s\" doesn't look like a rawsample - no RAWSOUND prefix",
			        path.c_str());
			return nullptr;
		}
		int frequency = Strings::ToInteger(splitString[2]);
		if (allowedSampleRates.find(frequency) == allowedSampleRates.end())
		{
			LogWarning("Rawsound \"%s\" has invalid sample rate of %d", path.c_str(), frequency);
			return nullptr;
		}
		auto file = data.fs.open(splitString[1]);
		if (!file)
		{
			LogWarning("Rawsound \"%s\" failed to open file \"%s\"", path.c_str(),
			           splitString[1].c_str());
			return nullptr;
		}

		auto sample = std::make_shared<Sample>();

		sample->format.frequency = frequency;
		sample->format.channels = 1;
		sample->format.format = AudioFormat::SampleFormat::PCM_UINT8;
		sample->sampleCount = file.size();
		sample->data.reset(new uint8_t[sample->sampleCount]);
		file.read(reinterpret_cast<char *>(sample->data.get()), sample->sampleCount);
		return sample;
	}
};

class RawSampleLoaderFactory : public SampleLoaderFactory
{
  public:
	virtual SampleLoader *create(Data &data) override { return new RawSampleLoader(data); }
};

SampleLoaderRegister<RawSampleLoaderFactory> load_at_init_raw_sample("raw");
}; // anonymous namespace

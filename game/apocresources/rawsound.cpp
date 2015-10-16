#include "framework/sampleloader_interface.h"
#include "framework/data.h"

namespace
{

using namespace OpenApoc;

class RawSampleLoader : public SampleLoader
{
	Data &data;

  public:
	RawSampleLoader(Data &data) : data(data) {}
	virtual std::shared_ptr<Sample> loadSample(UString path) override
	{
		auto file = data.fs.open(path);
		if (!file)
			return nullptr;

		auto sample = std::make_shared<Sample>();

		sample->format.frequency = 22050;
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

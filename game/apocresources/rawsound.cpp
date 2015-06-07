#include "framework/sampleloader_interface.h"
#include "framework/framework.h"

namespace {

using namespace OpenApoc;

class RawSampleLoader : public SampleLoader
{
	Framework &fw;
public:
	RawSampleLoader(Framework &fw)
		:fw(fw){}
	virtual std::shared_ptr<Sample> loadSample(UString path)
	{
		auto file = fw.data->load_file(path);
		if (!file)
			return nullptr;

		auto sample = std::make_shared<Sample>();

		sample->format.frequency = 22050;
		sample->format.channels = 1;
		sample->format.format = AudioFormat::SampleFormat::PCM_UINT8;
		sample->sampleCount = file.size();
		sample->data.reset(new uint8_t[sample->sampleCount]);
		file.read((char*)sample->data.get(), sample->sampleCount);
		return sample;
	}
};

class RawSampleLoaderFactory : public SampleLoaderFactory
{
public:
	virtual SampleLoader *create(Framework &fw)
	{
		return new RawSampleLoader(fw);
	}
};

SampleLoaderRegister<RawSampleLoaderFactory> load_at_init_raw_sample("raw");
}; //anonymous namespace

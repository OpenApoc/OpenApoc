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
		PHYSFS_file *file = fw.data->load_file(path, Data::FileMode::Read);
		if (!file)
			return nullptr;

		auto sample = std::make_shared<Sample>();

		sample->format.frequency = 22050;
		sample->format.channels = 1;
		sample->format.format = AudioFormat::SampleFormat::PCM_UINT8;
		sample->sampleCount = PHYSFS_fileLength(file);
		sample->data.reset(new uint8_t[sample->sampleCount]);
		PHYSFS_readBytes(file, sample->data.get(), sample->sampleCount);
		PHYSFS_close(file);
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

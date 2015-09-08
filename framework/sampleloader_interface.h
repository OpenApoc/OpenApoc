#pragma once
#include "sound.h"
#include "library/strings.h"

namespace OpenApoc
{

class Framework;

class SampleLoader
{
  public:
	virtual ~SampleLoader(){};
	virtual std::shared_ptr<Sample> loadSample(UString path) = 0;
};

class SampleLoaderFactory
{
  public:
	virtual SampleLoader *create(Framework &fw) = 0;
	virtual ~SampleLoaderFactory(){};
};

void registerSampleLoader(SampleLoaderFactory *factory, UString name);

template <typename T> class SampleLoaderRegister
{
  public:
	SampleLoaderRegister(UString name) { registerSampleLoader(new T, name); }
};

}; // namespace OpenApoc

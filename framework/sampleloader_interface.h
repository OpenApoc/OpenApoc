#pragma once
#include "library/sp.h"
#include "library/strings.h"
#include "sound.h"

namespace OpenApoc
{

class Data;

class SampleLoader
{
  public:
	virtual ~SampleLoader() {}
	virtual sp<Sample> loadSample(UString path) = 0;
};

class SampleLoaderFactory
{
  public:
	virtual SampleLoader *create(Data &data) = 0;
	virtual ~SampleLoaderFactory() {}
};

void registerSampleLoader(SampleLoaderFactory *factory, UString name);

template <typename T> class SampleLoaderRegister
{
  public:
	SampleLoaderRegister(UString name) { registerSampleLoader(new T, name); }
};

}; // namespace OpenApoc

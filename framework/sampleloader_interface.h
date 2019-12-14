#pragma once

#include "library/sp.h"
#include "library/strings.h"

namespace OpenApoc
{

class Data;
class Sample;

class SampleLoader
{
  public:
	virtual ~SampleLoader() = default;
	virtual sp<Sample> loadSample(UString path) = 0;
};

class SampleLoaderFactory
{
  public:
	virtual SampleLoader *create(Data &data) = 0;
	virtual ~SampleLoaderFactory() = default;
};

SampleLoaderFactory *getRAWSampleLoaderFactory();
SampleLoaderFactory *getWAVSampleLoaderFactory();

}; // namespace OpenApoc

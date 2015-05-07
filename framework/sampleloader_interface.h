#pragma once
#include "sound.h"

namespace OpenApoc {

class Framework;

class SampleLoader
{
public:
	virtual ~SampleLoader() {};
	virtual std::shared_ptr<Sample> loadSample(std::string path) = 0;
};

class SampleLoaderFactory
{
public:
	virtual SampleLoader *create(Framework &fw) = 0;
	virtual ~SampleLoaderFactory() {};
};

void registerSampleLoader(SampleLoaderFactory *factory, std::string name);

template <typename T>
class SampleLoaderRegister
{
public:
	SampleLoaderRegister(std::string name)
	{
		registerSampleLoader(new T, name);
	}
};

}; //namespace OpenApoc

#pragma once
#include "sound.h"
#include "library/strings.h"

namespace OpenApoc
{

class Data;
class MusicLoader
{
  public:
	virtual ~MusicLoader() {}
	virtual std::shared_ptr<MusicTrack> loadMusic(UString path) = 0;
};

class MusicLoaderFactory
{
  public:
	virtual MusicLoader *create(Data &data) = 0;
	virtual ~MusicLoaderFactory() {}
};

void registerMusicLoader(MusicLoaderFactory *factory, UString name);

template <typename T> class MusicLoaderRegister
{
  public:
	MusicLoaderRegister(UString name) { registerMusicLoader(new T, name); }
};
}; // namespace OpenApoc

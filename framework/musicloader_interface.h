#pragma once
#include "library/sp.h"
#include "library/strings.h"
#include "sound.h"

namespace OpenApoc
{

class Data;
class MusicLoader
{
  public:
	virtual ~MusicLoader() {}
	virtual sp<MusicTrack> loadMusic(UString path) = 0;
};

class MusicLoaderFactory
{
  public:
	virtual MusicLoader *create(Data &data) = 0;
	virtual ~MusicLoaderFactory() {}
};

MusicLoaderFactory *getRAWMusicLoaderFactory();

}; // namespace OpenApoc

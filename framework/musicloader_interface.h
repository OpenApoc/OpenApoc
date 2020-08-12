#pragma once

#include "library/sp.h"
#include "library/strings.h"

namespace OpenApoc
{

class Data;
class Data;
class MusicTrack;

class MusicLoader
{
  public:
	virtual ~MusicLoader() = default;
	virtual sp<MusicTrack> loadMusic(UString path) = 0;
};

class MusicLoaderFactory
{
  public:
	virtual MusicLoader *create(Data &data) = 0;
	virtual ~MusicLoaderFactory() = default;
};

MusicLoaderFactory *getRAWMusicLoaderFactory();
MusicLoaderFactory *getVorbisMusicLoaderFactory();

}; // namespace OpenApoc

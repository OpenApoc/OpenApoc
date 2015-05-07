#pragma once
#include "sound.h"

namespace OpenApoc {

	class Framework;
	class MusicLoader
	{
	public:
		virtual ~MusicLoader() {};
		virtual std::shared_ptr<MusicTrack> loadMusic(std::string path) = 0;
	};

	class MusicLoaderFactory
	{
	public:
		virtual MusicLoader *create(Framework &fw) = 0;
		virtual ~MusicLoaderFactory() {};
	};

	void registerMusicLoader(MusicLoaderFactory *factory, std::string name);

	template <typename T>
	class MusicLoaderRegister
	{
	public:
		MusicLoaderRegister(std::string name)
		{
			registerMusicLoader(new T, name);
		}
	};
}; //namespace OpenApoc

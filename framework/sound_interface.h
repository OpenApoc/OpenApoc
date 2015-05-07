#pragma once
#include "sound.h"

namespace OpenApoc {
	class SoundBackendFactory
	{
	public:
		virtual SoundBackend *create() = 0;
		virtual ~SoundBackendFactory() {};
	};

	void registerSoundBackend(SoundBackendFactory *factory, std::string name);

	template <typename T>
	class SoundBackendRegister
	{
	public:
		SoundBackendRegister(std::string name)
		{
			registerSoundBackend(new T, name);
		}
	};
}; //namespace OpenApoc

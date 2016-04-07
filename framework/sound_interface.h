#pragma once
#include "sound.h"

namespace OpenApoc
{
class SoundBackendFactory
{
  public:
	virtual SoundBackend *create() = 0;
	virtual ~SoundBackendFactory() {}
};

SoundBackendFactory *getNullSoundBackend();
SoundBackendFactory *getSDLSoundBackend();
}; // namespace OpenApoc

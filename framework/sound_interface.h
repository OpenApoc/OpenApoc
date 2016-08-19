#pragma once
#include "sound.h"

namespace OpenApoc
{
class SoundBackendFactory
{
  public:
	virtual SoundBackend *create() = 0;
	virtual ~SoundBackendFactory() = default;
};

SoundBackendFactory *getNullSoundBackend();
SoundBackendFactory *getSDLSoundBackend();
}; // namespace OpenApoc

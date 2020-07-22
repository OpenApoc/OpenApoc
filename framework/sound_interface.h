#pragma once

#include "framework/sound.h"

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

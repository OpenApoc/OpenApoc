#pragma once

#include "library/sp.h"
#include <list>

namespace OpenApoc
{
class Sample;
class DamageType;

class CityCommonSampleList
{
  public:
	sp<Sample> teleport;
	sp<Sample> vehicleExplosion;
	sp<Sample> sceneryExplosion;
	sp<Sample> shieldHit;
	sp<Sample> dimensionShiftIn;
	sp<Sample> dimensionShiftOut;
	std::list<sp<Sample>> alertSounds;
};
} // namespace OpenApoc

#pragma once
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include <vector>

namespace OpenApoc
{
class Sample;
class DamageType;

class BattleCommonSampleList
{
  public:
	sp<Sample> gravlift;
	sp<Sample> door;
	sp<Sample> brainsuckerHatch;
	sp<Sample> teleport;
	std::vector<sp<std::vector<sp<Sample>>>> walkSounds;
	std::vector<sp<Sample>> objectDropSounds;
	std::list<sp<Sample>> throwSounds;
	std::map<StateRef<DamageType>, std::list<sp<Sample>>> explosionSoundMap;
};
}
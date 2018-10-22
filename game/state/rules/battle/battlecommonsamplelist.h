#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include <list>
#include <map>
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
	sp<Sample> brainsuckerSuck;
	sp<Sample> teleport;
	sp<Sample> burn;
	sp<std::list<sp<Sample>>> genericHitSounds;
	std::vector<sp<std::vector<sp<Sample>>>> walkSounds;
	std::vector<sp<Sample>> objectDropSounds;
	std::list<sp<Sample>> throwSounds;
	sp<std::list<sp<Sample>>> psiSuccessSounds;
	sp<std::list<sp<Sample>>> psiFailSounds;
};
} // namespace OpenApoc

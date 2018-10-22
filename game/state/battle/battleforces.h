#pragma once

#include "library/sp.h"
#include <vector>

namespace OpenApoc
{

class BattleSquad;
class BattleUnit;

class BattleForces
{
  public:
	std::vector<BattleSquad> squads;
	bool insert(unsigned squad, sp<BattleUnit> unit);
	bool insertAt(unsigned squad, unsigned position, sp<BattleUnit> unit);
	void removeAt(unsigned squad, unsigned position);
	BattleForces();
};

class BattleSquad
{
  public:
	std::vector<sp<BattleUnit>> units;
	int getNumUnits();
	BattleSquad();
};
} // namespace OpenApoc

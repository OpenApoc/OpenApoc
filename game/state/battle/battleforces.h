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
	bool insert(int squad, sp<BattleUnit> unit);
	bool insertAt(int squad, int position, sp<BattleUnit> unit);
	BattleForces();
};

class BattleSquad
{
  public:
	std::vector<sp<BattleUnit>> units;
	int getNumUnits();
	BattleSquad();
};
}
#include "game/state/battle/battleforces.h"
#include "game/state/battle/battleunit.h"
#include "library/sp.h"
#include <vector>

namespace OpenApoc
{

BattleForces::BattleForces() : squads(6){};

bool BattleForces::insert(unsigned squad, sp<BattleUnit> unit)
{
	if (squads[squad].getNumUnits() == 6 && unit->squadNumber != (int)squad)
		return false;
	return insertAt(squad, squads[squad].getNumUnits(), unit);
}

bool BattleForces::insertAt(unsigned squad, unsigned position, sp<BattleUnit> unit)
{
	if (squads[squad].getNumUnits() == 6 && unit->squadNumber != (int)squad)
		return false;
	if (unit->squadNumber != -1)
	{
		removeAt(unit->squadNumber, unit->squadPosition);
	}
	if (position > squads[squad].units.size())
	{
		position = (unsigned)squads[squad].units.size();
	}
	squads[squad].units.insert(squads[squad].units.begin() + position, unit);
	unit->squadNumber = squad;
	unit->squadPosition = position;
	return true;
}

void BattleForces::removeAt(unsigned squad, unsigned position)
{
	squads[squad].units[position]->squadNumber = -1;
	squads[squad].units.erase(squads[squad].units.begin() + position);
}

BattleSquad::BattleSquad(){};

int BattleSquad::getNumUnits() { return (int)units.size(); }
}
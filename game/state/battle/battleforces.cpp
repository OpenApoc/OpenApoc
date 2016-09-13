#include "game/state/battle/battleforces.h"
#include "game/state/battle/battleunit.h"
#include "library/sp.h"
#include <vector>

namespace OpenApoc
{

BattleForces::BattleForces() : squads(6) {};

bool BattleForces::insert(int squad, sp<BattleUnit> unit)
{
	if (squads[squad].getNumUnits() == 6 && unit->squadNumber != squad)
		return false;
	return insertAt(squad, squads[squad].getNumUnits(), unit);
}

bool BattleForces::insertAt(int squad, int position, sp<BattleUnit> unit)
{
	if (squads[squad].getNumUnits() == 6 && unit->squadNumber != squad)
		return false;
	if (unit->squadNumber != -1)
	{
		squads[unit->squadNumber].units.erase(squads[unit->squadNumber].units.begin() + unit->squadPosition);
	}
	if (position > squads[squad].units.size())
	{
		position = squads[squad].units.size();
	}
	squads[squad].units.insert(squads[squad].units.begin() + position, unit);
	unit->squadNumber = squad;
	unit->squadPosition = position;
	return true;
}

BattleSquad::BattleSquad() {};

int BattleSquad::getNumUnits() {
	return (int)units.size();
}

}
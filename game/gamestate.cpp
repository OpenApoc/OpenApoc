#include "game/gamestate.h"
#include "game/city/city.h"

#include "framework/includes.h"

namespace OpenApoc {

GameState::GameState(Framework &fw, Rules &rules)
{
	for (auto &orgdef : rules.getOrganisationDefs())
	{
		this->organisations.emplace_back(orgdef);
	}

	this->city.reset(new City(fw));
}


}; //namespace OpenApoc

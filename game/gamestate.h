#pragma once

#include <random>
#include <memory>
#include <vector>
#include "library/strings.h"
#include "game/organisation.h"
#include "game/base/base.h"

namespace OpenApoc
{

class City;
class Framework;
class Rules;

class GameState
{
  public:
	GameState(Framework &fw, Rules &rules);
	std::unique_ptr<City> city;

	std::vector<Organisation> organisations;
	std::vector<std::shared_ptr<Base>> playerBases;

	bool showTileOrigin;
	bool showVehiclePath;
	bool showSelectableBounds;

	std::default_random_engine rng;

	UString getPlayerBalance() const;
};

}; // namespace OpenApoc

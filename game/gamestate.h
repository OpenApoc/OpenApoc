#pragma once

#include <random>
#include <memory>
#include <vector>

namespace OpenApoc
{

class City;
class Framework;
class Rules;
class Organisation;

class GameState
{
  public:
	GameState(Framework &fw, Rules &rules);
	std::unique_ptr<City> city;

	std::vector<Organisation> organisations;

	bool showTileOrigin;
	bool showVehiclePath;
	bool showSelectableBounds;

	std::default_random_engine rng;
};

}; // namespace OpenApoc

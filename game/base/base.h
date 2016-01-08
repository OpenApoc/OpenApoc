#pragma once

#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"

#include <random>

namespace OpenApoc
{

class Building;
class FacilityDef;
class Facility;
class GameState;

class Base
{
  private:
	std::vector<std::vector<bool>> corridors;
	std::vector<sp<Facility>> facilities;

  public:
	static const int SIZE = 8;
	enum class BuildError
	{
		None,
		Occupied,
		OutOfBounds,
		NoMoney
	};

	std::weak_ptr<Building> bld;
	UString name;

	Base(GameState &state, sp<Building> building);

	sp<const Facility> getFacility(Vec2<int> pos) const;
	const std::vector<std::vector<bool>> &getCorridors() const { return corridors; }
	const std::vector<sp<Facility>> &getFacilities() const { return facilities; }

	void startingBase(GameState &state, std::default_random_engine &rng);
	BuildError canBuildFacility(const FacilityDef &def, Vec2<int> pos, bool free = false) const;
	void buildFacility(const FacilityDef &def, Vec2<int> pos, bool free = false);
	BuildError canDestroyFacility(Vec2<int> pos) const;
	void destroyFacility(Vec2<int> pos);

	std::map<UString, unsigned> inventory;
};

}; // namespace OpenApoc

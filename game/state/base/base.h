#pragma once

#include "game/state/city/building.h"
#include "game/state/gamestate.h"
#include "game/state/rules/facility_type.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <vector>

namespace OpenApoc
{

class Building;
class Facility;

class Base : public StateObject<Base>
{
  public:
	std::vector<std::vector<bool>> corridors;
	std::vector<sp<Facility>> facilities;
	std::map<UString, unsigned> inventoryAgentEquipment;
	std::map<UString, unsigned> inventoryVehicleEquipment;
	std::map<UString, unsigned> inventoryVehicleAmmo;
	UString name;
	StateRef<Building> building;

	Base() = default;

	static const int SIZE = 8;
	enum class BuildError
	{
		NoError,
		Occupied,
		OutOfBounds,
		NoMoney,
		Indestructible,
	};

	Base(GameState &state, StateRef<Building> building);

	sp<Facility> getFacility(Vec2<int> pos) const;
	void startingBase(GameState &state);
	BuildError canBuildFacility(StateRef<FacilityType> type, Vec2<int> pos,
	                            bool free = false) const;
	void buildFacility(GameState &state, StateRef<FacilityType> type, Vec2<int> pos,
	                   bool free = false);
	BuildError canDestroyFacility(Vec2<int> pos) const;
	void destroyFacility(GameState &state, Vec2<int> pos);
	int getCapacityUsed(FacilityType::Capacity type) const;
	int getCapacityTotal(FacilityType::Capacity type) const;
	int getUsage(sp<Facility> facility) const;
};

}; // namespace OpenApoc

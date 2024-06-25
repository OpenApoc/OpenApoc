#pragma once

#include "game/state/rules/city/facilitytype.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <map>
#include <vector>

namespace OpenApoc
{

class Building;
class Facility;
class GameState;
class Organisation;

class Base : public StateObject<Base>, public std::enable_shared_from_this<Base>
{
  public:
	std::vector<std::vector<bool>> corridors;
	std::vector<sp<Facility>> facilities;
	// Each base has its own selected lab
	std::weak_ptr<Facility> selectedLab;
	// For ammunition, this is actually the count of bullets, not clips
	std::map<UString, unsigned> inventoryBioEquipment;
	std::map<UString, unsigned> inventoryAgentEquipment;
	std::map<UString, unsigned> inventoryVehicleEquipment;
	std::map<UString, unsigned> inventoryVehicleAmmo;
	UString name;
	StateRef<Building> building;

	Base() = default;

	static constexpr int SIZE = 8;
	enum class BuildError
	{
		NoError,
		Occupied,
		OutOfBounds,
		NoMoney,
		Indestructible,
	};

	Base(GameState &state, StateRef<Building> building);

	void die(GameState &state, bool collapse);

	sp<Facility> getFacility(Vec2<int> pos) const;
	void startingBase(GameState &state);
	BuildError canBuildFacility(StateRef<FacilityType> type, Vec2<int> pos,
	                            bool free = false) const;
	void buildFacility(GameState &state, StateRef<FacilityType> type, Vec2<int> pos,
	                   bool free = false);
	BuildError canDestroyFacility(GameState &state, Vec2<int> pos) const;
	void destroyFacility(GameState &state, Vec2<int> pos);

	// Returns if an Alien Containment module exists at base
	bool alienContainmentExists(GameState &state);

	// Returns if Alien Containment capacity is empty at base
	bool alienContainmentIsEmpty(GameState &state);
	int getCapacityUsed(GameState &state, FacilityType::Capacity type) const;
	int getCapacityTotal(FacilityType::Capacity type) const;
	float getUsage(GameState &state, const sp<Facility> facility, const int delta = 0) const;
	float getUsage(GameState &state, const FacilityType::Capacity type, const int delta = 0) const;
};

}; // namespace OpenApoc

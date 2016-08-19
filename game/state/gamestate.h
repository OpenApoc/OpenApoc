#pragma once
#include "library/sp.h"

#include "game/state/agent.h"
#include "game/state/base/base.h"
#include "game/state/city/city.h"
#include "game/state/gametime.h"
#include "game/state/organisation.h"
#include "game/state/research.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/doodad_type.h"
#include "game/state/rules/facility_type.h"
#include "game/state/rules/ufo_growth.h"
#include "game/state/rules/ufo_incursion.h"
#include "game/state/rules/vammo_type.h"
#include "game/state/rules/vehicle_type.h"
#include "game/state/rules/vequipment_type.h"
#include "game/state/stateobject.h"
#include "game/state/ufopaedia.h"
#include "library/strings.h"
#include "library/xorshift.h"
#include <random>

namespace OpenApoc
{

class SerializationArchive;
class City;
class Base;

static const unsigned ORIGINAL_TICKS = 36;
class GameState : public std::enable_shared_from_this<GameState>
{
  private:
	void update(unsigned int ticks);

  public:
	std::map<UString, sp<VehicleType>> vehicle_types;
	std::map<UString, sp<Organisation>> organisations;
	std::map<UString, sp<FacilityType>> facility_types;
	std::map<UString, sp<DoodadType>> doodad_types;
	std::map<UString, sp<VEquipmentType>> vehicle_equipment;
	std::map<UString, sp<VAmmoType>> vehicle_ammo;
	std::map<UString, sp<AEquipmentType>> agent_equipment;
	std::map<UString, sp<BaseLayout>> base_layouts;
	std::map<UString, sp<UFOGrowth>> ufo_growth_lists;
	std::map<UString, sp<UFOIncursion>> ufo_incursions;
	std::map<UString, sp<Base>> player_bases;
	std::map<UString, sp<City>> cities;
	std::map<UString, sp<Vehicle>> vehicles;
	std::map<UString, sp<UfopaediaCategory>> ufopaedia;
	ResearchState research;

	mutable unsigned lastVehicle = 0;

	std::map<UString, sp<Agent>> agents;
	AgentGenerator agent_generator;

	std::map<Agent::Type, unsigned> initial_agents;
	std::map<UString, unsigned> initial_facilities;

	StateRef<Organisation> player;

	StateRef<City> current_city;
	StateRef<Base> current_base;

	GameState();
	~GameState();

	bool showTileOrigin = false;
	bool showVehiclePath = false;
	bool showSelectableBounds = false;

	Xorshift128Plus<uint32_t> rng;

	UString getPlayerBalance() const;
	StateRef<Organisation> getOrganisation(const UString &orgID);
	const StateRef<Organisation> &getPlayer() const;
	StateRef<Organisation> getPlayer();

	// The time from game start in ticks
	GameTime gameTime;

	// high level api for loading game
	bool loadGame(const UString &path);

	// high level api for saving game
	// WARNING! Does not save metadata
	bool saveGame(const UString &path, bool pack = true);

	// serializes gamestate to archive
	bool serialize(sp<SerializationArchive> archive) const;

	// deserializes gamestate from archive
	bool deserialize(const sp<SerializationArchive> archive);

	// Called on a newly started Game to setup initial state that isn't serialized in (random
	// vehicle positions etc.) - it is not called
	void startGame();
	// Called after serializing in to hook up 'secondary' data (stuff that is derived from stuff
	// that is serialized but not serialized itself). This should also be called on starting a new
	// game after startGame()
	void initState();

	// Returns true if we can go at max speed (IE push all update loops to 5 minute intervals -
	// causes insta-completion of all routes etc.
	// Cannot be done if:
	// - there are any enemy units on the current map
	// - there are any projectiles on the current map
	bool canTurbo() const;

	// Update progresses one 'tick'
	void update();
	// updateTurbo progresses 5 minutes at a time - can only be called if canTurbo() returns true.
	// canTurbo() must be re-tested after each call to see if we should drop down to normal speed
	// (e.g. enemy appeared, other user action required)
	void updateTurbo();

	void updateEndOfDay();
	void updateEndOfWeek();
};

}; // namespace OpenApoc

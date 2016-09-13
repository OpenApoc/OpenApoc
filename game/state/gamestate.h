#pragma once
#include "library/sp.h"

#include "game/state/agent.h"
#include "game/state/base/base.h"
#include "game/state/battle.h"
#include "game/state/battle/aequipment.h"
#include "game/state/battle/battleunitanimation.h"
#include "game/state/battle/battleunitimagepack.h"
#include "game/state/battlemap.h"
#include "game/state/battlemaptileset.h"
#include "game/state/city/city.h"
#include "game/state/gametime.h"
#include "game/state/message.h"
#include "game/state/organisation.h"
#include "game/state/research.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/damage.h"
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
class GameEvent;

static const int MAX_MESSAGES = 50;
static const unsigned ORIGINAL_TICKS = 36;
class GameState : public std::enable_shared_from_this<GameState>
{
  private:
	void update(unsigned int ticks);

  public:
	StateRefMap<VehicleType> vehicle_types;
	StateRefMap<Organisation> organisations;
	StateRefMap<FacilityType> facility_types;
	StateRefMap<DoodadType> doodad_types;
	StateRefMap<VEquipmentType> vehicle_equipment;
	StateRefMap<VAmmoType> vehicle_ammo;
	StateRefMap<BaseLayout> base_layouts;
	StateRefMap<UFOGrowth> ufo_growth_lists;
	StateRefMap<UFOIncursion> ufo_incursions;
	StateRefMap<Base> player_bases;
	StateRefMap<City> cities;
	StateRefMap<Vehicle> vehicles;
	StateRefMap<UfopaediaCategory> ufopaedia;
	ResearchState research;
	StateRefMap<BattleUnitImagePack> battle_unit_image_packs;
	StateRefMap<BattleUnitAnimation> battle_unit_animations;
	std::set<UString> loadedTilesets;
	StateRefMap<BattleMapPartType> battleMapTiles;
	StateRefMap<BattleMap> battle_maps;
	sp<Battle> battle;
	StateRefMap<DamageModifier> damage_modifiers;
	StateRefMap<DamageType> damage_types;
	StateRefMap<AEquipmentType> agent_equipment;
	StateRefMap<EquipmentSet> equipment_sets_by_score;
	StateRefMap<EquipmentSet> equipment_sets_by_level;

	std::list<EventMessage> messages;

	mutable unsigned lastVehicle = 0;

	StateRefMap<AgentType> agent_types;
	StateRefMap<Agent> agents;
	AgentGenerator agent_generator;

	std::map<AgentType::Role, unsigned> initial_agents;
	std::map<UString, unsigned> initial_facilities;

	StateRef<Organisation> player;
	StateRef<Organisation> aliens;

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
	const StateRef<Organisation> &getAliens() const;
	StateRef<Organisation> getAliens();

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

	void logEvent(GameEvent *ev);
};

}; // namespace OpenApoc

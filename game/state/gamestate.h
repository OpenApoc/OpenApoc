#pragma once

#include "game/state/city/economyinfo.h"
#include "game/state/city/research.h"
#include "game/state/gameeventtypes.h"
#include "game/state/gametime.h"
#include "game/state/luagamestate.h"
#include "game/state/shared/agent.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/xorshift.h"
#include <cstdint>
#include <list>
#include <map>
#include <mutex>

namespace OpenApoc
{

class SerializationArchive;
class City;
class Base;
class GameEvent;
class AEquipmentType;
class VehicleType;
class FacilityType;
class DoodadType;
class VEquipmentType;
class VAmmoType;
class BaseLayout;
class HazardType;
class UFOGrowth;
class UFOIncursion;
class Vehicle;
class UfopaediaCategory;
class BattleMap;
class EquipmentSet;
class Battle;
class CityCommonImageList;
class CityCommonSampleList;
class BattleCommonImageList;
class BattleCommonSampleList;
class BattleMapPartType;
class EventMessage;
class DamageType;
class BuildingFunction;
class UFOMissionPreference;

static const int MAX_MESSAGES = 50;
static const unsigned ORIGINAL_TICKS = 36;
static const bool UPDATE_EVERY_TICK = false;

class GameScore
{
  public:
	int tacticalMissions = 0;
	int researchCompleted = 0;
	int alienIncidents = 0;
	int craftShotDownUFO = 0;
	int craftShotDownXCom = 0;
	int incursions = 0;
	int cityDamage = 0;
	int getTotal();
	//	UString getText();
};

class GameState : public std::enable_shared_from_this<GameState>
{
  public:
	StateRefMap<VehicleType> vehicle_types;
	StateRefMap<Organisation> organisations;
	StateRefMap<FacilityType> facility_types;
	StateRefMap<DoodadType> doodad_types;
	StateRefMap<VEquipmentType> vehicle_equipment;
	StateRefMap<VAmmoType> vehicle_ammo;
	StateRefMap<BaseLayout> base_layouts;
	StateRefMap<UFOGrowth> ufo_growth_lists;
	StateRefMap<UFOMissionPreference> ufo_mission_preference;
	StateRefMap<UFOIncursion> ufo_incursions;
	StateRefMap<Base> player_bases;
	StateRefMap<City> cities;
	StateRefMap<Vehicle> vehicles;
	std::set<UString> vehiclesDeathNote;
	StateRefMap<UfopaediaCategory> ufopaedia;
	ResearchState research;
	StateRefMap<BattleMap> battle_maps;
	StateRefMap<HazardType> hazard_types;
	StateRefMap<DamageModifier> damage_modifiers;
	StateRefMap<DamageType> damage_types;
	StateRefMap<AEquipmentType> agent_equipment;
	StateRefMap<EquipmentSet> equipment_sets_by_score;
	StateRefMap<EquipmentSet> equipment_sets_by_level;
	StateRefMap<BuildingFunction> building_functions;
	sp<Battle> current_battle;
	sp<CityCommonImageList> city_common_image_list;
	sp<CityCommonSampleList> city_common_sample_list;
	sp<BattleCommonImageList> battle_common_image_list;
	sp<BattleCommonSampleList> battle_common_sample_list;

	// Loaded temporarily for the duration of the battle
	StateRefMap<BattleUnitImagePack> battle_unit_image_packs;
	StateRefMap<BattleUnitAnimationPack> battle_unit_animation_packs;
	StateRefMap<BattleMapPartType> battleMapTiles;

	std::list<EventMessage> messages;

	int baseIndex = 1;
	int difficulty = 0;
	bool firstDetection = false;
	uint64_t nextInvasion = 0;

	StateRefMap<AgentType> agent_types;
	StateRefMap<AgentBodyType> agent_body_types;
	StateRefMap<AgentEquipmentLayout> agent_equipment_layouts;
	StateRefMap<Agent> agents;
	AgentGenerator agent_generator;

	std::map<AgentType::Role, unsigned> initial_agents;
	std::map<UString, unsigned> initial_facilities;
	std::list<std::list<StateRef<AEquipmentType>>> initial_agent_equipment;
	std::list<std::pair<StateRef<VehicleType>, std::list<StateRef<VEquipmentType>>>>
	    initial_vehicles;
	std::list<std::pair<StateRef<VEquipmentType>, int>> initial_vehicle_equipment;
	std::list<std::pair<StateRef<VAmmoType>, int>> initial_vehicle_ammo;
	std::map<UString, int> initial_base_agent_equipment;
	std::map<int, std::list<std::pair<StateRef<AgentType>, Vec2<int>>>> initial_aliens;

	std::map<UString, EconomyInfo> economy;

	StateRef<Organisation> player;
	StateRef<Organisation> aliens;
	StateRef<Organisation> government;
	StateRef<Organisation> civilian;

	StateRef<City> current_city;
	StateRef<Base> current_base;

	std::vector<EquipmentTemplate> agentEquipmentTemplates;

	GameScore totalScore = {};
	GameScore weekScore = {};
	int micronoidRainChance = 0;

	// Used to move events from battle to city and remember time

	GameTime gameTimeBeforeBattle = GameTime(0);
	UString missionLocationBattle;
	UString eventFromBattleText;
	GameEventType eventFromBattle = GameEventType::None;

	// Used to generate unique names, an incrementing ID for each object type (keyed by StateObject
	// prefix)
	std::mutex objectIdCountLock;
	std::map<UString, uint64_t> objectIdCount;

	GameState();
	~GameState();

	Xorshift128Plus<uint32_t> rng;

	UString getPlayerBalance() const;
	StateRef<Organisation> getOrganisation(const UString &orgID);
	const StateRef<Organisation> &getPlayer() const;
	StateRef<Organisation> getPlayer();
	const StateRef<Organisation> &getAliens() const;
	StateRef<Organisation> getAliens();
	const StateRef<Organisation> &getGovernment() const;
	StateRef<Organisation> getGovernment();
	const StateRef<Organisation> &getCivilian() const;
	StateRef<Organisation> getCivilian();

	// The time from game start in ticks
	GameTime gameTime;

	// high level api for loading game
	bool loadGame(const UString &path);

	// high level api for saving game
	// WARNING! Does not save metadata
	bool saveGame(const UString &path, bool pack = true, bool pretty = false);
	bool saveGameDelta(const UString &path, const GameState &reference, bool pack = true,
	                   bool pretty = false);

	// serializes gamestate to archive
	bool serialize(SerializationArchive *archive) const;
	// Serializes gamestate to archive with a reference (IE values the same as the reference are not
	// saved)
	bool serialize(SerializationArchive *archive, const GameState &reference) const;

	// deserializes gamestate from archive
	bool deserialize(SerializationArchive *archive);

	// Called on a newly started Game to setup initial state that isn't serialized in (random
	// vehicle positions etc.) - it is not called
	void startGame();
	// Called after serializing in to hook up 'secondary' data (stuff that is derived from stuff
	// that is serialized but not serialized itself). This should also be called on starting a new
	// game after startGame()
	void initState();
	// Stub until we have actual mods
	void applyMods();

	void setCurrentCity(StateRef<City> city);

	// Validates gamestate, sanity checks for all the possible fuck-ups
	void validate();
	void validateResearch();
	void validateScenery();
	void validateAgentEquipment();

	void fillOrgStartingProperty();
	// Fills out initial player property
	void fillPlayerStartingProperty();

	void invasion();

	// Returns true if we can go at max speed (IE push all update loops to 5 minute intervals -
	// causes insta-completion of all routes etc.
	// Cannot be done if:
	// - there are any enemy units on the current map
	// - there are any projectiles on the current map
	bool canTurbo() const;

	// Immediately remove all dead objects.
	void cleanUpDeathNote();
	// Update progress
	void update(unsigned int ticks);
	// updateTurbo progresses 5 minutes at a time - can only be called if canTurbo() returns true.
	// canTurbo() must be re-tested after each call to see if we should drop down to normal speed
	// (e.g. enemy appeared, other user action required)
	void updateTurbo();
	// this moves non-aggressive vehicles around for some more ticks so that when time is paused
	// after turbo city appears more alive
	void updateAfterTurbo();

	void updateBeforeBattle();
	void updateAfterBattle();

	void updateEndOfSecond();
	void updateEndOfFiveMinutes();
	void updateEndOfHour();
	void updateEndOfDay();
	void updateEndOfWeek();

	void logEvent(GameEvent *ev);

	// Following members are not serialized
	bool newGame = false;
	bool skipTurboCalculations = false;

	LuaGameState luaGameState;

	// Loads all mods set in the options - note this likely requires the mod data directories to
	// already be added to the filesystem
	void loadMods();
	// appends a GameState package from "submodPath", relative to the currently set data directories
	// Returns true on success, false on failure
	bool appendGameState(const UString &gamestatePath);
};

}; // namespace OpenApoc

#include "game/state/rules/battle/battlemap.h"
#include "framework/configfile.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlescanner.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/facility.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlecommonsamplelist.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/rules/battle/battlemaptileset.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/shared/aequipment.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/organisation.h"
#include <algorithm>
#include <unordered_map>

namespace OpenApoc
{

namespace
{
int getCorridorSectorID(const Base &base, Vec2<int> pos)
{
	// key is North South West East (true = occupied, false = vacant)
	const std::unordered_map<std::vector<bool>, int> TILE_CORRIDORS = {
	    {{false, false, false, false}, 3}, {{true, false, false, false}, 4},
	    {{false, false, false, true}, 5},  {{true, false, false, true}, 6},
	    {{false, true, false, false}, 7},  {{true, true, false, false}, 8},
	    {{false, true, false, true}, 9},   {{true, true, false, true}, 10},
	    {{false, false, true, false}, 11}, {{true, false, true, false}, 12},
	    {{false, false, true, true}, 13},  {{true, false, true, true}, 14},
	    {{false, true, true, false}, 15},  {{true, true, true, false}, 16},
	    {{false, true, true, true}, 17},   {{true, true, true, true}, 18}};

	if (pos.x < 0 || pos.y < 0 || pos.x >= Base::SIZE || pos.y >= Base::SIZE)
	{
		LogError("Going out of bounds for base");
		return 0;
	}
	else if (!base.corridors[pos.x][pos.y])
	{
		// We need to cap any facilities
		// For that we need to find where facilities are
		std::vector<std::vector<bool>> facilities;
		facilities.resize(Base::SIZE);
		for (int i = 0; i < Base::SIZE; i++)
		{
			facilities[i].resize(Base::SIZE);
		}
		for (auto &facility : base.facilities)
		{
			if (facility->buildTime > 0)
			{
				continue;
			}
			for (int x = 0; x < facility->type->size; x++)
			{
				for (int y = 0; y < facility->type->size; y++)
				{
					facilities[facility->pos.x + x][facility->pos.y + y] = true;
				}
			}
		}

		bool north = pos.y > 0 && facilities[pos.x][pos.y - 1];
		bool south = pos.y < Base::SIZE - 1 && facilities[pos.x][pos.y + 1];
		bool west = pos.x > 0 && facilities[pos.x - 1][pos.y];
		bool east = pos.x < Base::SIZE - 1 && facilities[pos.x + 1][pos.y];
		return TILE_CORRIDORS.at({north, south, west, east}) - 3;
		return 0;
	}
	else
	{
		bool north = pos.y > 0 && base.corridors[pos.x][pos.y - 1];
		bool south = pos.y < Base::SIZE - 1 && base.corridors[pos.x][pos.y + 1];
		bool west = pos.x > 0 && base.corridors[pos.x - 1][pos.y];
		bool east = pos.x < Base::SIZE - 1 && base.corridors[pos.x + 1][pos.y];
		return TILE_CORRIDORS.at({north, south, west, east}) - 3 + 15;
	}
}
} // namespace

template <> sp<BattleMap> StateObject<BattleMap>::get(const GameState &state, const UString &id)
{
	auto it = state.battle_maps.find(id);
	if (it == state.battle_maps.end())
	{
		LogError("No battle_map matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}
template <> const UString &StateObject<BattleMap>::getPrefix()
{
	static UString prefix = "BATTLEMAP_";
	return prefix;
}
template <> const UString &StateObject<BattleMap>::getTypeName()
{
	static UString name = "BattleMap";
	return name;
}
template <>
const UString &StateObject<BattleMap>::getId(const GameState &state, const sp<BattleMap> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.battle_maps)
	{
		if (a.second == ptr)
			return a.first;
	}
	LogError("No battle_map matching pointer %p", ptr.get());
	return emptyString;
}

// Create UFO battle
sp<Battle> BattleMap::createBattle(GameState &state, StateRef<Organisation> opponent,
                                   std::list<StateRef<Agent>> &player_agents,
                                   const std::map<StateRef<AgentType>, int> *aliens,
                                   StateRef<Vehicle> player_craft, StateRef<Vehicle> target_craft)
{
	if (!aliens)
	{
		aliens = &target_craft->type->crew_downed;
	}
	auto alienOrg = state.getAliens();

	const float hostilesMultiplier = config().getFloat("OpenApoc.Cheat.HostilesMultiplier");

	int countAliens = 0;
	for (auto &a : *aliens)
	{
		for (int i = 0; i < std::round(a.second * hostilesMultiplier); i++)
		{
			player_agents.push_back(state.agent_generator.createAgent(state, alienOrg, a.first));
			if (++countAliens >= MAX_UNITS_PER_SIDE)
			{
				break;
			}
		}
		if (countAliens >= MAX_UNITS_PER_SIDE)
		{
			break;
		}
	}

	return target_craft->type->battle_map->createBattle(
	    state, target_craft->owner, opponent, player_agents, player_craft,
	    Battle::MissionType::UfoRecovery, target_craft.id);
}

// Create building battle
sp<Battle> BattleMap::createBattle(GameState &state, StateRef<Organisation> opponent,
                                   std::list<StateRef<Agent>> &player_agents,
                                   const std::map<StateRef<AgentType>, int> *aliens,
                                   const int *guards, const int *civilians,
                                   StateRef<Vehicle> player_craft, StateRef<Building> building)
{
	std::list<std::pair<StateRef<Organisation>, StateRef<AgentType>>> otherParticipants;
	auto missionType = Battle::MissionType::AlienExtermination;

	std::map<StateRef<AgentType>, int> current_crew;
	StateRef<BattleMap> map;

	const float hostilesMultiplier = config().getFloat("OpenApoc.Cheat.HostilesMultiplier");

	// Setup mission type and other participants
	if (building->base != nullptr && building->owner == state.getPlayer())
	{
		// Base defense mission
		map = {&state, "BATTLEMAP_37base"};

		if (opponent == state.getAliens() && !aliens)
		{
			// Aliens beamed down or walked into our building
			// They become current_crew!
			aliens = &current_crew;
			current_crew = building->current_crew;
			building->current_crew.clear();
		}
		else
		{
			// Enemy raid, spawn guards for them
			int numGuards = guards ? *guards : opponent->getGuardCount(state);
			numGuards = std::round(numGuards * hostilesMultiplier);
			numGuards = std::min(numGuards, MAX_UNITS_PER_SIDE);
			for (int i = 0; i < numGuards; i++)
			{
				otherParticipants.emplace_back(opponent,
				                               pickRandom(state.rng, opponent->guard_types_human));
			}
		}

		// Find which base is under attack
		StateRef<Base> base = building->base;

		// Add combat personnel
		int playerAgentsCount = 0;
		for (auto &agent : state.agents)
		{
			if (agent.second->homeBuilding->base != base)
			{
				continue;
			}
			if (agent.second->type->role != AgentType::Role::Soldier)
			{
				continue;
			}
			player_agents.emplace_back(&state, agent.first);
			if (++playerAgentsCount >= MAX_UNITS_PER_SIDE)
			{
				break;
			}
		}
		// Add non-combat personnel
		if (playerAgentsCount < MAX_UNITS_PER_SIDE)
		{
			for (auto &agent : state.agents)
			{
				if (agent.second->homeBuilding->base != base)
				{
					continue;
				}
				if (agent.second->type->role == AgentType::Role::Soldier)
				{
					continue;
				}
				player_agents.emplace_back(&state, agent.first);
				if (++playerAgentsCount >= MAX_UNITS_PER_SIDE)
				{
					break;
				}
			}
		}

		missionType = Battle::MissionType::BaseDefense;
	}
	else
	{
		// Raid Aliens / Raid Humans / Alien Extermination mission
		map = building->battle_map;

		if (opponent == state.getAliens())
		{
			if (building->owner == state.getAliens())
			{
				// Raid Aliens mission

				if (!aliens)
				{
					aliens = &building->preset_crew;
				}

				// Civilians will not be actually added if there is no spawn points for them
				int numCivs = civilians ? *civilians : state.getCivilian()->getGuardCount(state);
				numCivs = std::min(numCivs, MAX_UNITS_PER_SIDE);
				for (int i = 0; i < numCivs; i++)
				{
					otherParticipants.emplace_back(
					    state.getCivilian(),
					    pickRandom(state.rng, state.getCivilian()->guard_types_alien));
				}

				missionType = Battle::MissionType::RaidAliens;
			}
			else
			{
				// Alien Extermination mission

				// Add aliens
				if (!aliens)
				{
					aliens = &current_crew;
					current_crew = building->current_crew;
					building->current_crew.clear();
				}

				// Add building security if hostile to player
				int numGuards = guards ? *guards
				                       : (building->owner->isRelatedTo(state.getPlayer()) ==
				                                  Organisation::Relation::Hostile
				                              ? building->owner->getGuardCount(state)
				                              : 0);
				numGuards = std::round(numGuards * hostilesMultiplier);
				numGuards = std::min(numGuards, MAX_UNITS_PER_SIDE);
				for (int i = 0; i < numGuards; i++)
				{
					otherParticipants.emplace_back(
					    building->owner, pickRandom(state.rng, building->owner->guard_types_human));
				}

				// Civilians will not be actually added if there is no spawn points for them
				// Or if building owner is not alien but hostile
				int numCivs = civilians ? *civilians : state.getCivilian()->getGuardCount(state);
				numCivs = std::min(numCivs, MAX_UNITS_PER_SIDE);
				for (int i = 0; i < numCivs; i++)
				{
					otherParticipants.emplace_back(
					    state.getCivilian(),
					    pickRandom(state.rng, state.getCivilian()->guard_types_human));
				}

				missionType = Battle::MissionType::AlienExtermination;
			}
		}
		else
		{
			// Raid humans mission

			// Add aliens
			if (!aliens)
			{
				aliens = &current_crew;
				current_crew = building->current_crew;
				building->current_crew.clear();
			}

			// Add building security always
			{
				int numGuards = guards ? *guards : building->owner->getGuardCount(state);
				numGuards = std::round(numGuards * hostilesMultiplier);
				numGuards = std::min(numGuards, MAX_UNITS_PER_SIDE);
				for (int i = 0; i < numGuards; i++)
				{
					otherParticipants.emplace_back(
					    opponent, pickRandom(state.rng, building->owner->guard_types_human));
				}
			}

			// Never add civilians

			missionType = Battle::MissionType::RaidHumans;
		}
	}

	// Create battle

	if (aliens)
	{
		int countAliens = 0;
		auto alienOrg = state.getAliens();
		for (auto &a : *aliens)
		{
			for (int i = 0; i < std::round(a.second * hostilesMultiplier); i++)
			{
				player_agents.push_back(
				    state.agent_generator.createAgent(state, alienOrg, a.first));
				if (++countAliens >= MAX_UNITS_PER_SIDE)
				{
					break;
				}
			}
			if (countAliens >= MAX_UNITS_PER_SIDE)
			{
				break;
			}
		}
	}

	for (auto &pair : otherParticipants)
	{
		player_agents.push_back(state.agent_generator.createAgent(state, pair.first, pair.second));
	}

	return map->createBattle(state, building->owner, opponent, player_agents, player_craft,
	                         missionType, building.id);
}

namespace
{

// Checks whether two cubes intersect
// s1 = size of sector 1, x1, y1, z1 = coordinate of sector 1
// s2 = size of sector 2, x2, y2, z2 = coordinate of sector 2
bool doTwoSectorsIntersect(int x1, int y1, int z1, Vec3<int> s1, int x2, int y2, int z2,
                           Vec3<int> s2)
{
	return
	    // 1's right > 2's left
	    x1 + s1.x > x2
	    // 2's right > 1's left
	    && x2 + s2.x > x1
	    // 1's top > 2's bottom
	    && z1 + s1.z > z2
	    // 2's top > 1's bottom
	    && z2 + s2.z > z1
	    // 1's front > 2's back
	    && y1 + s1.y > y2
	    // 2's front > 1's back
	    && y2 + s2.y > y1;
}

bool doesCellIntersectSomething(std::vector<sp<BattleMapSector>> &sec_map,
                                const Vec3<int> &map_size, int x1, int y1, int z1)
{
	bool intersects = false;
	static Vec3<int> cell_size = {1, 1, 1};
	for (int x2 = 0; x2 < map_size.x; x2++)
		for (int y2 = 0; y2 < map_size.y; y2++)
			for (int z2 = 0; z2 < map_size.z; z2++)
				intersects =
				    intersects ||
				    (sec_map[x2 + y2 * map_size.x + z2 * map_size.x * map_size.y] &&
				     doTwoSectorsIntersect(
				         x1, y1, z1, cell_size, x2, y2, z2,
				         sec_map[x2 + y2 * map_size.x + z2 * map_size.x * map_size.y]->size));
	return intersects;
}

bool isMapComplete(std::vector<sp<BattleMapSector>> &sec_map, const Vec3<int> &map_size)
{
	// We check if every single cell of a map intersects with at least some sector
	bool complete = true;
	for (int x1 = 0; x1 < map_size.x; x1++)
		for (int y1 = 0; y1 < map_size.y; y1++)
			for (int z1 = 0; z1 < map_size.z; z1++)
				complete = complete && doesCellIntersectSomething(sec_map, map_size, x1, y1, z1);
	return complete;
}

bool placeSector(GameState &state, std::vector<sp<BattleMapSector>> &sec_map,
                 const Vec3<int> &map_size, const sp<BattleMapSector> sector, bool force = false,
                 bool invert_x_packing = false, bool invert_y_packing = false)
{
	bool moved = false;
	int attempt = 0;
	while (++attempt < 2 || (force && moved))
	{
		// If it's not our first attempt, and we are in "force placement" mode, then we need to do
		// some tetris in order to save space
		if (attempt > 1)
		{
			moved = false;

			// Move everything one step back on all axes (even though movement on z axis should be
			// very rare)
			for (int x = 1; x < map_size.x; x++)
			{
				for (int y = 1; y < map_size.y; y++)
				{
					for (int z = 1; z < map_size.z; z++)
					{
						int x1 = invert_x_packing ? map_size.x - 1 - x : x;
						int y1 = invert_y_packing ? map_size.y - 1 - y : y;
						int z1 = z;
						int dx1 = invert_x_packing ? -1 : 1;
						int dy1 = invert_y_packing ? -1 : 1;
						int dz1 = 1;

						if (!sec_map[x1 + y1 * map_size.x + z1 * map_size.x * map_size.y])
							continue;
						bool can_move = false;

						// Try moving back on x
						can_move = true;
						for (int y2 = y1;
						     y2 < y1 + sec_map[x1 + y1 * map_size.x + z1 * map_size.x * map_size.y]
						                   ->size.y;
						     y2++)
							for (int z2 = z1;
							     z2 <
							     z1 + sec_map[x1 + y1 * map_size.x + z1 * map_size.x * map_size.y]
							              ->size.z;
							     z2++)
								can_move = can_move && !doesCellIntersectSomething(
								                           sec_map, map_size, x1 - dx1, y2, z2);
						if (can_move)
						{
							sec_map[x1 - dx1 + y1 * map_size.x + z1 * map_size.x * map_size.y] =
							    sec_map[x1 + y1 * map_size.x + z1 * map_size.x * map_size.y];
							sec_map[x1 + y1 * map_size.x + z1 * map_size.x * map_size.y] = nullptr;
							moved = true;
							continue;
						}

						// Try moving back on y
						can_move = true;
						for (int x2 = x1;
						     x2 < x1 + sec_map[x1 + y1 * map_size.x + z1 * map_size.x * map_size.y]
						                   ->size.x;
						     x2++)
							for (int z2 = z1;
							     z2 <
							     z1 + sec_map[x1 + y1 * map_size.x + z1 * map_size.x * map_size.y]
							              ->size.z;
							     z2++)
								can_move = can_move && !doesCellIntersectSomething(
								                           sec_map, map_size, x2, y1 - dy1, z2);
						if (can_move)
						{
							sec_map[x1 + (y1 - dy1) * map_size.x + z1 * map_size.x * map_size.y] =
							    sec_map[x1 + y1 * map_size.x + z1 * map_size.x * map_size.y];
							sec_map[x1 + y1 * map_size.x + z1 * map_size.x * map_size.y] = nullptr;
							moved = true;
							continue;
						}

						// Try moving back on z
						can_move = true;
						for (int x2 = x1;
						     x2 < x1 + sec_map[x1 + y1 * map_size.x + z1 * map_size.x * map_size.y]
						                   ->size.x;
						     x2++)
							for (int y2 = z1;
							     y2 <
							     y1 + sec_map[x1 + y1 * map_size.x + z1 * map_size.x * map_size.y]
							              ->size.y;
							     y2++)
								can_move = can_move && !doesCellIntersectSomething(
								                           sec_map, map_size, x2, y2, z1 - dz1);
						if (can_move)
						{
							sec_map[x1 + y1 * map_size.x + (z1 - dz1) * map_size.x * map_size.y] =
							    sec_map[x1 + y1 * map_size.x + z1 * map_size.x * map_size.y];
							sec_map[x1 + y1 * map_size.x + z1 * map_size.x * map_size.y] = nullptr;
							moved = true;
							continue;
						}
					}
				}
			}

			// Cannot move anything else, this means placing is impossible
			if (!moved)
				continue;
		}

		// We look for all possible locations to place the sector
		std::vector<Vec3<int>> possible_locations;
		for (int x1 = 0; x1 <= map_size.x - sector->size.x; x1++)
		{
			for (int y1 = 0; y1 <= map_size.y - sector->size.y; y1++)
			{
				for (int z1 = 0; z1 <= map_size.z - sector->size.z; z1++)
				{
					// We check whether sector placed here does not intersect with others
					bool fits = true;
					for (int x2 = 0; x2 < map_size.x; x2++)
						for (int y2 = 0; y2 < map_size.y; y2++)
							for (int z2 = 0; z2 < map_size.z; z2++)
								fits = fits &&
								       (!sec_map[x2 + y2 * map_size.x +
								                 z2 * map_size.x * map_size.y] ||
								        !doTwoSectorsIntersect(x1, y1, z1, sector->size, x2, y2, z2,
								                               sec_map[x2 + y2 * map_size.x +
								                                       z2 * map_size.x * map_size.y]
								                                   ->size));
					if (!fits)
						continue;

					// If we're still here, then it definitely fits
					possible_locations.emplace_back(x1, y1, z1);
				}
			}
		}

		if (possible_locations.empty())
			continue;

		auto location = pickRandom(state.rng, possible_locations);
		sec_map[location.x + location.y * map_size.x + location.z * map_size.x * map_size.y] =
		    sector;

		return true;
	}
	return false;
}
} // namespace

bool BattleMap::generateMap(std::vector<sp<BattleMapSector>> &sec_map, Vec3<int> &size,
                            GameState &state, GenerationSize genSize)
{
	// Vanilla had vertical stacking of sectors planned, but not implemented. I will implement both
	// algorithms because I think that would be great to have. We could make it an extended game
	// option in the future.
	bool allow_vertical_stacking = true;

	// This switch will allow larger maps, +2 in size, which vanilla never did I think, and which is
	// required for some vertical stacking maps to actually spawn because they contain too many
	// mandatory sectors to fit into battle size even when enlarged by 1 on the smaller side
	bool allow_very_large_maps = true;

	// This switch allows maps to spawn only one of the mandatory sectors instead of
	// every single one
	// This provides for vertical stacking maps to be possible without huge sizes, but may
	// theoretically produce maps with no way to the upper layers
	// In practice, however, every vertical-stacking map's sector with more than one vertical chunk
	// I seen provides access to the second level
	// So that should never be a problem
	bool require_only_largest_mandatory_sector = true;

	// Vanilla had some rules that shrunk or extended maps based on squad size.
	// For example, 28SLUMS has max_y_size = 2, but often spawns a single block, because it's 9
	// layers high, big enough to fit everything.
	// OTOH, 14ACNORM has 2x2 max size, but often spawns 3x2 because it's only 2 layers high, and
	// 2nd layer is just air (high ceiling)
	// As we do not know them yet, I will generate maps in 3 modes for now: small, normal, big
	// Small being a -1 on the larger side and Big being +1 or +2 on a random side.
	// +2 is required for some maps with vertical stacking to fit all the mandatory sectors
	// For now, this is random, in the future, this will be tied to the amount of troops
	int size_mod =
	    genSize == GenerationSize::Small ? -1 : (genSize == GenerationSize::Normal ? 0 : 1);
	if (genSize == GenerationSize::VeryLarge)
	{
		if (allow_very_large_maps)
		{
			size_mod++;
		}
		else
		{
			LogWarning("Cannot generate a map %s with gen size %d since generating large maps is "
			           "disabled",
			           id, (int)genSize);
			return false;
		}
	}
	// Vertical stacking is also randomized, and disabled if the map does not allow it
	allow_vertical_stacking =
	    allow_vertical_stacking && max_battle_size.z > 1 && randBool(state.rng);
	// This switch is only relevant if we're vertically stacking
	require_only_largest_mandatory_sector =
	    require_only_largest_mandatory_sector && allow_vertical_stacking;
	// However, there are maps that have too many mandatory sectors
	// For these maps, we must set this switch, otherwise it's not possible to create a map!
	if (!require_only_largest_mandatory_sector)
	{
		int remainingMapSize = max_battle_size.x * max_battle_size.y * max_battle_size.z;
		for (auto &s : sectors)
			remainingMapSize -=
			    s.second->size.x * s.second->size.y * s.second->size.z * s.second->occurrence_min;
		if (remainingMapSize < 0)
			require_only_largest_mandatory_sector = true;
	}

	// Prepare sectors in a more useful form;
	// Need #0 to be unused for convenience
	std::vector<UString> secNames;
	std::vector<sp<BattleMapSector>> secRefs;
	int secCount = 0;
	int secMaxSizeSeen = 0;
	int secMaxZSeen = 0;
	secNames.push_back("");
	secRefs.push_back(nullptr);
	for (auto &s : sectors)
	{
		secCount++;
		secNames.push_back(s.first);
		secRefs.push_back(s.second);
		secMaxSizeSeen =
		    std::max(secMaxSizeSeen, s.second->size.x * s.second->size.y * s.second->size.z);
		secMaxZSeen = std::max(secMaxZSeen, s.second->size.z);
	}

	// Determine size in chunks
	size = this->max_battle_size;

	// Apply vertical stacking flag
	if (!allow_vertical_stacking)
	{
		// However, some maps have "mandatory" vertical stacking
		// That is, they only have sectors that are using more than one z chunk
		// We must check whether or not this map has any sectors that are 1-high on z
		bool foundUnstackedSector = false;
		for (int i = 1; i <= secCount; i++)
			foundUnstackedSector = foundUnstackedSector || (secRefs[i]->size.z == 1);
		if (foundUnstackedSector)
			size.z = 1;
	}

	// Apply size modification
	int size_mod_x = 0;
	int size_mod_y = 0;
	if (size_mod < 0)
	{
		if (size.x >= size.y)
			size_mod_x += size_mod;
		else
			size_mod_y += size_mod;
	}
	else if (size_mod > 0)
	{
		if (randBool(state.rng))
			size_mod_x += size_mod;
		else
			size_mod_y += size_mod;
	}
	size.x = std::max(1, size.x + size_mod_x);
	size.y = std::max(1, size.y + size_mod_y);
	auto modded_size = size;

	// Now let's see if we can actually make a map out of this
	for (int attempt_make_map = 1; attempt_make_map <= 4; attempt_make_map++)
	{
		bool random_generation = false;
		switch (attempt_make_map)
		{
			// If we reached second attempt, it means we failed to create a stacked map with normal
			// size, and should cancel vertical stacking.
			// If there was no vertical stacking, skip this attempt
			case 2:
				if (!allow_vertical_stacking)
					continue;
				size = modded_size;
				size.z = 1;
				break;
			// If we reached third attempt, it means we failed to create a random map and should
			// try a non-random map, filling sectors in big-to-small order, with vertical stacking
			case 3:
				size = modded_size;
				random_generation = false;
				break;
			// If we reached fourth attempt, it means we failed to create a random map and should
			// try a non-random map, filling sectors in big-to-small order, without vert. stacking
			case 4:
				if (!allow_vertical_stacking)
					continue;
				size = modded_size;
				size.z = 1;
				random_generation = false;
				break;
		}

		bool invert_x_packing = randBoundsInclusive(state.rng, 0, 1) == 1;
		bool invert_y_packing = randBoundsInclusive(state.rng, 0, 1) == 1;

		sec_map = {(unsigned)size.x * size.y * size.z, nullptr};
		std::vector<int> sec_num_placed = std::vector<int>(secCount + 1, 0);
		// Fill the list of remaining sectors so that we first attempt to place the mandatory sector
		// that is highest and largest
		std::vector<int> remaining_sectors;
		for (int z = 1; z <= secMaxZSeen; z++)
			for (int s = 1; s <= secMaxSizeSeen; s++)
				for (int i = 1; i <= secCount; i++)
					if (secRefs[i]->size.x * secRefs[i]->size.y * secRefs[i]->size.z == s &&
					    secRefs[i]->size.z == z)
						remaining_sectors.push_back(i);

		// Disable sectors that don't fit
		bool mandatorySectorLost = false;
		bool mandatorySectorRemaining = false;
		for (int i = (int)remaining_sectors.size() - 1; i >= 0; i--)
		{
			sec_num_placed[remaining_sectors[i]] = 0;
			if (secRefs[remaining_sectors[i]]->size.x > size.x ||
			    secRefs[remaining_sectors[i]]->size.y > size.y ||
			    secRefs[remaining_sectors[i]]->size.z > size.z)
			{
				// We only care about losing non-vertically-stacked sectors,
				// as vanilla had some maps where only stacked sectors are mandatory,
				// yet it never actually did stacking!
				// If, however, we are losing a stacked sector while stacking, then we do care!
				if (secRefs[remaining_sectors[i]]->occurrence_min > 0 &&
				    secRefs[remaining_sectors[i]]->size.z <= size.z)
				{
					mandatorySectorLost = true;
				}
				remaining_sectors.erase(remaining_sectors.begin() + i);
			}
			else
			{
				mandatorySectorRemaining =
				    mandatorySectorRemaining || (secRefs[remaining_sectors[i]]->occurrence_min > 0);
			}
		}
		// If we have lost all mandatory sectors due to size,
		// then we cannot create a map of such size
		if (mandatorySectorLost && !mandatorySectorRemaining)
		{
			LogWarning("Failed to place mandatory sectors for map %s with size %d, %d, %d at "
			           "attempt %d",
			           id, size.x, size.y, size.z, attempt_make_map);
			continue;
		}

		// Place all mandatory sectors
		bool failed = false;
		for (int i = (int)remaining_sectors.size() - 1; i >= 0; i--)
		{
			if (failed)
				break;
			for (int j = 0; j < secRefs[remaining_sectors[i]]->occurrence_min; j++)
				failed = failed || !placeSector(state, sec_map, size, secRefs[remaining_sectors[i]],
				                                true, invert_x_packing, invert_y_packing);
			sec_num_placed[remaining_sectors[i]] = secRefs[remaining_sectors[i]]->occurrence_min;
			if (sec_num_placed[remaining_sectors[i]] ==
			    secRefs[remaining_sectors[i]]->occurrence_max)
				remaining_sectors.erase(remaining_sectors.begin() + i);
			if (require_only_largest_mandatory_sector)
				break;
		}
		// If we failed to place at least one mandatory sector,
		// then we cannot create a map of such size
		if (failed)
		{
			LogWarning("Failed to place mandatory sectors for map %s with size %d, %d, %d at "
			           "attempt %d",
			           id, size.x, size.y, size.z, attempt_make_map);
			continue;
		}

		if (random_generation)
		{
			// Random map generator
			// Here we make two attempts to fill a map.
			for (int attempt_fill_map = 1; attempt_fill_map <= 2; attempt_fill_map++)
			{
				if (isMapComplete(sec_map, size))
					break;

				// If map is not complete on first attempt, we try again, this time forcing
				// placement
				// (stacking sectors to make way)
				if (attempt_fill_map == 2)
				{
					// Regenerate list of remaining sectors, returning those that were removed due
					// to
					// being unable to place
					for (int i = 1; i <= secCount; i++)
						remaining_sectors.push_back(i);
					// Remove misfits and those that are already placed up to max occurrence number
					for (int i = (int)remaining_sectors.size() - 1; i >= 0; i--)
					{
						if (sec_num_placed[remaining_sectors[i]] ==
						        secRefs[remaining_sectors[i]]->occurrence_max ||
						    secRefs[remaining_sectors[i]]->size.x > size.x ||
						    secRefs[remaining_sectors[i]]->size.y > size.y ||
						    secRefs[remaining_sectors[i]]->size.z > size.z)
							remaining_sectors.erase(remaining_sectors.begin() + i);
					}
				}

				// Place non-mandatory sectors, abiding the rules of max occurrence, until either
				// map is
				// complete or we cannot place another sector
				while (!isMapComplete(sec_map, size) && remaining_sectors.size() > 0)
				{
					int i = randBoundsExclusive(state.rng, (int)0, (int)remaining_sectors.size());
					if (placeSector(state, sec_map, size, secRefs[remaining_sectors[i]],
					                attempt_fill_map == 2, invert_x_packing, invert_y_packing))
					{
						if (++sec_num_placed[remaining_sectors[i]] ==
						    secRefs[remaining_sectors[i]]->occurrence_max)
							remaining_sectors.erase(remaining_sectors.begin() + i);
					}
					else
					{
						remaining_sectors.erase(remaining_sectors.begin() + i);
					}
				}
			}
		}
		else
		{
			// Non-random map generator
			// Place sectors, biggest to smallest, 0 to max on all axes

			for (int i = (int)remaining_sectors.size() - 1; i >= 0; i--)
			{
				while (sec_num_placed[remaining_sectors[i]] <
				       secRefs[remaining_sectors[i]]->occurrence_max)
				{
					bool placed = false;
					for (int x = 0; x <= size.x - secRefs[remaining_sectors[i]]->size.x; x++)
					{
						for (int y = 0; y <= size.y - secRefs[remaining_sectors[i]]->size.z; y++)
						{
							for (int z = 0; z <= size.z - secRefs[remaining_sectors[i]]->size.z;
							     z++)
							{
								placed = placeSector(state, sec_map, size,
								                     secRefs[remaining_sectors[i]]);
								if (placed)
									break;
							}
							if (placed)
								break;
						}
						if (placed)
							break;
					}
					if (!placed)
						break;
					sec_num_placed[remaining_sectors[i]]++;
				}
				remaining_sectors.erase(remaining_sectors.begin() + i);
				if (isMapComplete(sec_map, size))
					break;
			}
		}

		// If we failed at filling a map at this point, then there's nothing else we can do
		if (!isMapComplete(sec_map, size))
		{
			LogWarning("Failed to complete map %s with size %d, %d, %d at attempt %d", id, size.x,
			           size.y, size.z, attempt_make_map);
			continue;
		}

		LogWarning("Successfully completed map %s with size %d, %d, %d at attempt %d", id, size.x,
		           size.y, size.z, attempt_make_map);
		return true;
	}

	LogWarning("Failed (totally) to generate a map %s with gen size %d", id, (int)genSize);
	return false;
}

bool BattleMap::generateBase(std::vector<sp<BattleMapSector>> &sec_map, Vec3<int> &size,
                             GameState &state, UString mission_location_id)
{
	StateRef<Building> building = {&state, mission_location_id};
	StateRef<Base> base;
	for (auto &b : state.player_bases)
	{
		if (b.second->building == building)
		{
			base = {&state, b.first};
			break;
		}
	}
	if (!base)
	{
		LogError("Failed to find base in building %s", mission_location_id);
		return false;
	}

	std::vector<sp<BattleMapSector>> secRefs;
	for (auto &s : sectors)
	{
		secRefs.push_back(s.second);
	}

	size = {Base::SIZE, Base::SIZE, 1};
	sec_map = {(unsigned)size.x * size.y * size.z, nullptr};

	// Fill corridors and earth
	Vec2<int> i;
	for (i.x = 0; i.x < Base::SIZE; i.x++)
	{
		for (i.y = 0; i.y < Base::SIZE; i.y++)
		{
			sec_map[i.x + i.y * size.x] = secRefs[getCorridorSectorID(*base, i)];
		}
	}

	// Replace with facilities
	for (auto &facility : base->facilities)
	{
		if (facility->buildTime > 0)
		{
			continue;
		}
		// Clean ground under facility (for 2x2's)
		for (i.x = facility->pos.x; i.x < facility->pos.x + facility->type->size; i.x++)
		{
			for (i.y = facility->pos.y; i.y < facility->pos.y + facility->type->size; i.y++)
			{
				sec_map[i.x + i.y * size.x] = nullptr;
			}
		}
		// Facility itself
		sec_map[facility->pos.x + facility->pos.y * size.x] = secRefs[facility->type->sector];
	}

	return true;
}

sp<Battle>
BattleMap::fillMap(std::vector<std::list<std::pair<Vec3<int>, sp<BattleMapPart>>>> &doors,
                   bool &spawnCivilians, std::vector<sp<BattleMapSector>> sec_map, Vec3<int> size,
                   GameState &state, StateRef<Organisation> propertyOwner,
                   StateRef<Organisation> target_organisation, std::list<StateRef<Agent>> &agents,
                   StateRef<Vehicle> player_craft, Battle::MissionType mission_type,
                   UString mission_location_id)
{

	auto b = mksp<Battle>();

	b->currentPlayer = state.getPlayer();
	b->currentActiveOrganisation = state.getPlayer();
	b->currentTurn = 1;
	b->size = {chunk_size.x * size.x, chunk_size.y * size.y, chunk_size.z * size.z};
	b->battle_map = {&state, id};
	b->mission_type = mission_type;
	b->mission_location_id = mission_location_id;
	b->player_craft = player_craft;
	b->loadResources(state);
	b->reinforcementsInterval = reinforcementsInterval * TICKS_PER_SECOND;
	b->ticksUntilNextReinforcement = reinforcementsInterval * TICKS_PER_SECOND;

	for (int x = 0; x < size.x; x++)
	{
		for (int y = 0; y < size.y; y++)
		{
			for (int z = 0; z < size.z; z++)
			{
				auto sec = sec_map[x + y * size.x + z * size.x * size.y];
				if (!sec)
					continue;
				if (!sec->tiles)
				{
					LogInfo("Loading sector tiles \"%s\"", sec->sectorTilesName);
					sec->tiles.reset(new BattleMapSectorTiles());
					if (!sec->tiles->loadSector(state, BattleMapSectorTiles::getMapSectorPath() +
					                                       "/" + sec->sectorTilesName))
					{
						LogError("Failed to load sector tiles \"%s\"", sec->sectorTilesName);
					}
				}
				else
				{
					LogInfo("Using already-loaded sector tiles \"%s\"", sec->sectorTilesName);
				}
				auto &tiles = *sec->tiles;
				Vec3<int> shift = {x * chunk_size.x, y * chunk_size.y, z * chunk_size.z};

				for (auto &pair : tiles.initial_grounds)
				{
					auto s = mksp<BattleMapPart>();

					auto initialPosition = pair.first + shift;
					s->owner = propertyOwner;
					s->position = initialPosition;
					s->position += Vec3<float>(0.5f, 0.5f, 0.0f);

					// Check whether this is an exit location, and if so,
					// replace the ground map part with an appropriate exit
					bool canExit =
					    s->position.z >= exit_level_min && s->position.z <= exit_level_max;
					canExit = canExit && (s->position.x > 0 || allow_exit[MapDirection::West]) &&
					          (s->position.y > 0 || allow_exit[MapDirection::North]) &&
					          (s->position.x < size.x * chunk_size.x - 1 ||
					           allow_exit[MapDirection::East]) &&
					          (s->position.y < size.y * chunk_size.y - 1 ||
					           allow_exit[MapDirection::South]);
					if (canExit)
					{
						Vec3<int> exitLocX = s->position;
						Vec3<int> exitLocY = s->position;
						bool exitFarSideX = false;
						bool exitFarSideY = false;
						if (exitLocX.y == 0 || exitLocX.y == size.y * chunk_size.y - 1)
						{
							exitFarSideX = exitLocX.y != 0;
							exitLocX.y = 0;
							exitLocX.x = exitLocX.x % chunk_size.x;
						}
						if (exitLocY.x == 0 || exitLocY.x == size.x * chunk_size.x - 1)
						{
							exitFarSideY = exitLocY.x != 0;
							exitLocY.x = 0;
							exitLocY.y = exitLocY.y % chunk_size.y;
						}
						if (exitsX.find(exitLocX) != exitsX.end())
							s->type = exit_grounds[exitFarSideX ? 2 : 0];
						else if (exitsY.find(exitLocY) != exitsY.end())
							s->type = exit_grounds[exitFarSideY ? 1 : 3];
						else
							s->type = pair.second;
					}

					b->map_parts.push_back(s);
				}
				for (auto &pair : tiles.initial_left_walls)
				{
					auto s = mksp<BattleMapPart>();

					auto initialPosition = pair.first + shift;
					s->owner = propertyOwner;
					s->position = initialPosition;
					s->position += Vec3<float>(0.5f, 0.5f, 0.0f);
					s->type = pair.second;

					if (pair.second->door)
					{
						doors[0].emplace_back(initialPosition, s);
					}

					b->map_parts.push_back(s);
				}
				for (auto &pair : tiles.initial_right_walls)
				{
					auto s = mksp<BattleMapPart>();

					auto initialPosition = pair.first + shift;
					s->owner = propertyOwner;
					s->position = initialPosition;
					s->position += Vec3<float>(0.5f, 0.5f, 0.0f);
					s->type = pair.second;

					if (pair.second->door)
					{
						doors[1].emplace_back(initialPosition, s);
					}

					b->map_parts.push_back(s);
				}
				for (auto &pair : tiles.initial_features)
				{
					switch (pair.second->autoConvert)
					{
						case BattleMapPartType::AutoConvert::Fire:
						{
							StateRef<DamageType> dt = {&state, "DAMAGETYPE_INCENDIARY"};
							b->placeHazard(state, propertyOwner, nullptr, dt, pair.first + shift,
							               // Make it already hot
							               dt->hazardType->getLifetime(state) * 2, 0, 1, false);
							break;
						}
						case BattleMapPartType::AutoConvert::Smoke:
						{
							StateRef<DamageType> dt = {&state, "DAMAGETYPE_SMOKE"};
							b->placeHazard(state, propertyOwner, nullptr, dt, pair.first + shift,
							               dt->hazardType->getLifetime(state), 1, 2, false);
							break;
						}
						case BattleMapPartType::AutoConvert::None:
						{
							auto s = mksp<BattleMapPart>();

							auto initialPosition = pair.first + shift;
							s->owner = propertyOwner;
							s->position = initialPosition;
							s->position += Vec3<float>(0.5f, 0.5f, 0.0f);
							s->type = pair.second;

							b->map_parts.push_back(s);
							break;
						}
					}
				}
				for (auto &pair : tiles.loot_locations)
				{
					if (target_organisation->loot[pair.second].empty())
						continue;
					auto l = pickRandom(state.rng, target_organisation->loot[pair.second]);
					if (!l)
						continue;
					auto i = mksp<AEquipment>();
					i->type = l;
					i->ownerOrganisation = target_organisation;

					auto bi = b->placeItem(state, i, pair.first + shift);
				}
				for (auto &tlb : tiles.losBlocks)
				{
					auto lb = tlb->clone(shift);
					b->losBlocks.push_back(lb);
					if (mission_type == Battle::MissionType::BaseDefense)
					{
						// Base defenses never spawn civilians, so don't raise the flag here
						// They also never prevent entrance or exit from any vector,
						// so don't worry about that too
					}
					else
					{
						// At least one civilian spawner required for map to spawn civilians
						if (lb->spawn_type == SpawnType::Civilian || lb->also_allow_civilians)
						{
							spawnCivilians = true;
						}
						// Los block must touch map edge, and it's lowest z must be within spawn
						// allowance in order for it to qualify for spawning X-Com agents
						else if (lb->spawn_priority > 0 && lb->spawn_type == SpawnType::Player)
						{
							bool canSpawn = lb->start.z >= entrance_level_min &&
							                lb->start.z < entrance_level_max;
							if (canSpawn)
							{
								canSpawn =
								    (allow_entrance[MapDirection::West] && lb->start.x == 0) ||
								    (allow_entrance[MapDirection::North] && lb->start.y == 0) ||
								    (allow_entrance[MapDirection::East] &&
								     lb->end.x == size.x * chunk_size.x) ||
								    (allow_entrance[MapDirection::South] &&
								     lb->end.y == size.y * chunk_size.y);
							}
							if (!canSpawn)
							{
								lb->spawn_priority = 0;
							}
						}
					}
				}
				for (auto &entry : tiles.guardianLocations)
				{
					for (auto &pos : entry.second)
					{
						b->spawnLocations[entry.first].push_back(pos + shift);
						agents.emplace_back(
						    state.agent_generator.createAgent(state, propertyOwner, entry.first));
						agents.back()->destroyAfterBattle = true;
					}
				}
				for (auto &entry : sec->spawnLocations)
				{
					for (auto &pos : entry.second)
					{
						b->spawnLocations[entry.first].push_back(pos + shift);
					}
				}
			}
		}
	}
	// If org is hostile to player don't spawn civilians
	if (propertyOwner->isRelatedTo(state.getPlayer()) == Organisation::Relation::Hostile &&
	    propertyOwner != state.getAliens())
	{
		spawnCivilians = false;
	}

	return b;
}

void BattleMap::linkDoors(sp<Battle> b,
                          std::vector<std::list<std::pair<Vec3<int>, sp<BattleMapPart>>>> doors,
                          GameState &state)
{
	for (int i = 0; i < 2; i++)
	{
		while (doors[i].size() > 0)
		{
			std::list<Vec3<int>> locationsToCheck;
			std::set<Vec3<int>> locationsVisited;

			auto d = b->addDoor(state);
			d->right = i == 1;
			d->operational = true;

			// Recursively search for doors in a pattern spreading around start location
			locationsToCheck.push_back(doors[i].front().first);
			while (locationsToCheck.size() > 0)
			{
				auto loc = locationsToCheck.front();
				locationsToCheck.pop_front();

				if (locationsVisited.find(loc) != locationsVisited.end())
					continue;
				locationsVisited.insert(loc);

				auto it = doors[i].begin();
				while (it != doors[i].end() &&
				       (it->first.x != loc.x || it->first.y != loc.y || it->first.z != loc.z))
				{
					it++;
				}
				if (it == doors[i].end())
				{
					continue;
				}

				it->second->door = {&state, d->id};
				d->animationFrameCount = std::max(d->animationFrameCount,
				                                  (int)it->second->type->animation_frames.size());
				// d->mapParts.push_back(it->second); // <- no need to do it here, we do it in
				// initBattle

				// 0 = left, 1 = right
				// left search on Y axis, right on X
				// therefore for i=0 X=0, for i=1 Y=0
				locationsToCheck.push_back(it->first + Vec3<int>{i * 1, (1 - i) * 1, 0});
				locationsToCheck.push_back(it->first + Vec3<int>{i * -1, (1 - i) * -1, 0});
				locationsToCheck.push_back(it->first + Vec3<int>{0, 0, 1});
				locationsToCheck.push_back(it->first + Vec3<int>{0, 0, -1});
				doors[i].erase(it);
			}
		}
	}
}

void BattleMap::fillSquads(sp<Battle> b, bool spawnCivilians, GameState &state,
                           std::list<StateRef<Agent>> &agents)
{
	// Ensure player goes first
	b->participants.push_back(state.getPlayer());

	// Agents are just added to squads in a default way here.
	// Player will be allowed a chance to equip them and assign to squads how they prefer
	// We will assign their positions and "spawn" them in "BeginBattle" function
	for (auto &a : agents)
	{
		if (!spawnCivilians && a->owner == state.getCivilian())
		{
			// Delete agent
			state.agents.erase(a.id);
			a->destroy();
			continue;
		}
		if (std::find(b->participants.begin(), b->participants.end(), a->owner) ==
		    b->participants.end())
		{
			b->participants.push_back(a->owner);
		}

		for (auto &e : a->equipment)
		{
			e->ownerOrganisation = a->owner;
		}

		auto u = b->placeUnit(state, a);

		u->updateDisplayedItem(state);
	}

	// Find out number of agents in each org
	std::map<StateRef<Organisation>, int> agentCount;
	for (auto &o : b->participants)
	{
		b->forces[o];
		agentCount[o] = 0;
	}
	for (auto &u : b->units)
	{
		if (u.second->retreated || !u.second->agent->type->allowsDirectControl)
		{
			continue;
		}
		agentCount[u.second->owner]++;
	}

	// Distribute agents into squads
	for (auto &o : b->participants)
	{
		// If amount of agents is small, first add agents in groups of threes
		if (agentCount[o] <= 18)
		{
			// If only 2 remain, add them still
			// If only 1 remains, do not add them
			for (int s = 0; s < 6; s++)
			{
				if (agentCount[o] == 0)
				{
					break;
				}
				if (agentCount[o] > 1)
				{
					for (auto &u : b->units)
					{
						if (b->forces[o].squads[s].getNumUnits() >= 3)
						{
							break;
						}
						if (u.second->owner != o || u.second->squadNumber != -1 ||
						    u.second->retreated || !u.second->agent->type->allowsDirectControl)
						{
							continue;
						}
						u.second->assignToSquad(*b, s);
						agentCount[o]--;
						if (agentCount[o] == 0)
						{
							break;
						}
					}
				}
			}
		}

		// Now fill squads with remaining agents
		for (int s = 0; s < 6; s++)
		{
			if (agentCount[o] == 0)
			{
				break;
			}
			for (auto &u : b->units)
			{
				if (b->forces[o].squads[s].getNumUnits() == 6)
				{
					break;
				}
				if (u.second->owner != o || u.second->squadNumber != -1 || u.second->retreated ||
				    !u.second->agent->type->allowsDirectControl)
				{
					continue;
				}
				u.second->assignToSquad(*b, s);
				agentCount[o]--;
				if (agentCount[o] == 0)
				{
					break;
				}
			}
		}
	}
}

void BattleMap::initNewMap(sp<Battle> b)
{
	// Init visibility
	for (auto &o : b->participants)
	{
		b->visibleTiles[o] = std::vector<bool>(b->size.x * b->size.y * b->size.z, false);
		b->visibleBlocks[o] = std::vector<bool>(b->losBlocks.size(), false);
		b->visibleUnits[o] = {};
	}

	// Init los block pathfinding

	// Vars
	int size = b->losBlocks.size();
	auto &losBlocks = b->losBlocks;
	auto &linkAvailable = b->linkAvailable;

	// Mark all blocks for update
	b->linkNeedsUpdate = std::vector<bool>(size * size, false);
	b->blockNeedsUpdate = std::vector<bool>(size, true);

	// Init which blocks are adjacent (this never changes)
	linkAvailable = std::vector<bool>(size * size, false);
	for (int i = 0; i < size - 1; i++)
	{
		auto &b1 = *losBlocks[i];
		for (int j = i + 1; j < size; j++)
		{
			auto &b2 = *losBlocks[j];
			if (doTwoSectorsIntersect(
			        b1.start.x, b1.start.y, b1.start.z, b1.end - b1.start + Vec3<int>{1, 1, 1},
			        b2.start.x, b2.start.y, b2.start.z, b2.end - b2.start + Vec3<int>{1, 1, 1}))
			{
				linkAvailable[i + j * size] = true;
				linkAvailable[j + i * size] = true;
			}
		}
	}

	// Init arrays for further use
	for (auto &type : BattleUnitTypeList)
	{
		b->blockAvailable[type] = std::vector<bool>(size, false);
		b->blockCenterPos[type] = std::vector<Vec3<int>>(size, Vec3<int>());
		b->linkCost[type] = std::vector<int>(size * size, -1);
	}
}

void BattleMap::unloadTiles()
{
	for (auto &s : sectors)
		s.second->tiles = nullptr;
	LogInfo("Unloaded sector tiles.");
}

sp<Battle> BattleMap::createBattle(GameState &state, StateRef<Organisation> propertyOwner,
                                   StateRef<Organisation> target_organisation,
                                   std::list<StateRef<Agent>> &agents,
                                   StateRef<Vehicle> player_craft, Battle::MissionType mission_type,
                                   UString mission_location_id)
{
	std::vector<sp<BattleMapSector>> sec_map;
	Vec3<int> size;
	auto doors = std::vector<std::list<std::pair<Vec3<int>, sp<BattleMapPart>>>>(2);
	bool spawnCivilians = false;

	// Step 01 - 03 : Generate Map
	sp<Battle> b;
	sp<Battle> lastB;
	int genSize = 0;
	GenerationSize genSizeEnum;
	do
	{
		// Step 01: Generate map layout
		genSizeEnum = (GenerationSize)genSize;
		if (mission_type == Battle::MissionType::BaseDefense)
		{
			generateBase(sec_map, size, state, mission_location_id);
		}
		else
		{
			if (!generateMap(sec_map, size, state, genSizeEnum))
			{
				continue;
			}
		}

		// Step 02: Fill map with map parts
		b = fillMap(doors, spawnCivilians, sec_map, size, state, propertyOwner, target_organisation,
		            agents, player_craft, mission_type, mission_location_id);

		// Step 03: Ensure enough space exists
		if (!b->initialMapCheck(state, agents))
		{
			lastB = b;
			b = nullptr;
		}
	} while (!b && genSizeEnum != GenerationSize::VeryLarge && ++genSize);
	if (!b)
	{
		if (lastB)
		{
			b = lastB;
		}
		else
		{
			return nullptr;
		}
	}

	// Step 04: Link adjacent doors
	linkDoors(b, doors, state);

	// Step 05: Fill squads initially (and remove civilians if there's no spawns for them)
	fillSquads(b, spawnCivilians, state, agents);

	// Step 06: Fill up initial map parameters
	initNewMap(b);

	// Step 07: Unload sector tiles
	unloadTiles();

	// Step 08: Make target hostile
	state.getPlayer()->current_relations[target_organisation] = -100.0f;

	return b;
}

void BattleMap::loadTilesets(GameState &state) const
{
	if (state.battleMapTiles.size() > 0)
	{
		LogInfo("Tilesets are already loaded.");
		return;
	}

	// Load all tilesets used by the map
	for (auto &tilesetName : this->tilesets)
	{
		unsigned count = 0;
		auto tilesetPath = BattleMapTileset::getTilesetPath() + "/" + tilesetName;
		LogInfo("Loading tileset \"%s\" from \"%s\"", tilesetName, tilesetPath);
		BattleMapTileset tileset;
		if (!tileset.loadTileset(state, tilesetPath))
		{
			LogError("Failed to load tileset \"%s\" from \"%s\"", tilesetName, tilesetPath);
			continue;
		}

		for (auto &tilePair : tileset.map_part_types)
		{
			auto &tileName = tilePair.first;
			auto &tile = tilePair.second;
			// Assign sounds
			if (tile->sfxIndex != -1)
			{
				tile->walkSounds = state.battle_common_sample_list->walkSounds.at(tile->sfxIndex);
				tile->objectDropSound =
				    state.battle_common_sample_list->objectDropSounds.at(tile->sfxIndex);
			}
			// Assign map marts
			switch (tile->type)
			{
				case BattleMapPartType::Type::Ground:
					tile->rubble = rubble_feature;
					tile->destroyed_ground_tile = destroyed_ground_tile;
					break;
				case BattleMapPartType::Type::LeftWall:
					tile->rubble = rubble_left_wall;
					break;
				case BattleMapPartType::Type::RightWall:
					tile->rubble = rubble_right_wall;
					break;
				case BattleMapPartType::Type::Feature:
					tile->rubble = rubble_feature;
					break;
			}
			tile->damageModifier = {&state, "DAMAGEMODIFIER_TERRAIN_1_"};
			// Sanity check
			if (state.battleMapTiles.find(tileName) != state.battleMapTiles.end())
			{
				LogError("Duplicate tile with ID \"%s\"", tileName);
				continue;
			}
			state.battleMapTiles.emplace(tileName, tile);
			count++;
		}
		LogInfo("Loaded %u tiles from tileset \"%s\"", count, tilesetName);
	}
}

void BattleMap::unloadTilesets(GameState &state)
{
	state.battleMapTiles.clear();
	LogInfo("Unloaded all tilesets.");
}
} // namespace OpenApoc

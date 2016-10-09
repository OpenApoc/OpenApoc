#include "game/state/battle/battlemap.h"
#include "game/state/aequipment.h"
#include "game/state/agent.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlecommonsamplelist.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/battle/battlemaptileset.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/organisation.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/vehicle_type.h"
#include <algorithm>

namespace OpenApoc
{
BattleMap::BattleMap() {}

template <> sp<BattleMap> StateObject<BattleMap>::get(const GameState &state, const UString &id)
{
	auto it = state.battle_maps.find(id);
	if (it == state.battle_maps.end())
	{
		LogError("No battle_map matching ID \"%s\"", id.cStr());
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

sp<Battle> BattleMap::createBattle(GameState &state, StateRef<Organisation> organisation,
                                   std::list<StateRef<Agent>> &player_agents,
                                   StateRef<Vehicle> craft, StateRef<Vehicle> ufo)
{
	// FIXME: Generate list of agent types for enemies (and civs) properly
	std::list<std::pair<StateRef<Organisation>, StateRef<AgentType>>> otherParticipants;
	otherParticipants.emplace_back(organisation,
	                               StateRef<AgentType>{&state, "AGENTTYPE_ANTHROPOD"});
	otherParticipants.emplace_back(organisation,
	                               StateRef<AgentType>{&state, "AGENTTYPE_ANTHROPOD"});
	otherParticipants.emplace_back(organisation,
	                               StateRef<AgentType>{&state, "AGENTTYPE_ANTHROPOD"});
	otherParticipants.emplace_back(organisation,
	                               StateRef<AgentType>{&state, "AGENTTYPE_SKELETOID"});
	otherParticipants.emplace_back(organisation,
	                               StateRef<AgentType>{&state, "AGENTTYPE_SKELETOID"});
	otherParticipants.emplace_back(organisation,
	                               StateRef<AgentType>{&state, "AGENTTYPE_SKELETOID"});
	otherParticipants.emplace_back(organisation,
	                               StateRef<AgentType>{&state, "AGENTTYPE_ANTHROPOD"});
	otherParticipants.emplace_back(organisation,
	                               StateRef<AgentType>{&state, "AGENTTYPE_ANTHROPOD"});
	otherParticipants.emplace_back(organisation,
	                               StateRef<AgentType>{&state, "AGENTTYPE_ANTHROPOD"});

	for (auto &pair : otherParticipants)
	{
		player_agents.push_back(state.agent_generator.createAgent(state, pair.first, pair.second));
	}
	return ufo->type->battle_map->createBattle(state, organisation, player_agents, craft,
	                                           Battle::MissionType::UfoRecovery, ufo.id);
}

sp<Battle> BattleMap::createBattle(GameState &state, StateRef<Organisation> organisation,
                                   std::list<StateRef<Agent>> &player_agents,
                                   StateRef<Vehicle> craft, StateRef<Building> building)
{
	std::list<std::pair<StateRef<Organisation>, StateRef<AgentType>>> otherParticipants;
	auto missionType = Battle::MissionType::AlienExtermination;

	// Setup mission type and other participants
	if (building->owner == state.getPlayer())
	{
		// Base defense mission

		// FIXME: Generate list of agent types for enemies properly
		// otherParticipants.emplace_back(organisation, StateRef<AgentType>{ &state,
		// "AGENTTYPE_ANTHROPOD" });
		// Also add non-combat personell

		missionType = Battle::MissionType::BaseDefense;
	}
	else
	{
		// FIXME: Generate list of aliens in the building properly
		otherParticipants.emplace_back(state.getAliens(),
		                               StateRef<AgentType>{&state, "AGENTTYPE_ANTHROPOD"});
		otherParticipants.emplace_back(state.getAliens(),
		                               StateRef<AgentType>{&state, "AGENTTYPE_ANTHROPOD"});
		otherParticipants.emplace_back(state.getAliens(),
		                               StateRef<AgentType>{&state, "AGENTTYPE_ANTHROPOD"});
		otherParticipants.emplace_back(state.getAliens(),
		                               StateRef<AgentType>{&state, "AGENTTYPE_SKELETOID"});
		otherParticipants.emplace_back(state.getAliens(),
		                               StateRef<AgentType>{&state, "AGENTTYPE_SKELETOID"});
		otherParticipants.emplace_back(state.getAliens(),
		                               StateRef<AgentType>{&state, "AGENTTYPE_SKELETOID"});
		otherParticipants.emplace_back(state.getAliens(),
		                               StateRef<AgentType>{&state, "AGENTTYPE_ANTHROPOD"});
		otherParticipants.emplace_back(state.getAliens(),
		                               StateRef<AgentType>{&state, "AGENTTYPE_ANTHROPOD"});
		otherParticipants.emplace_back(state.getAliens(),
		                               StateRef<AgentType>{&state, "AGENTTYPE_ANTHROPOD"});

		if (organisation == state.getAliens())
		{
			if (building->owner == state.getAliens())
			{
				// Raid Aliens mission

				missionType = Battle::MissionType::RaidAliens;
			}
			else
			{
				// Alien Extermination mission

				// Add building security if hostile to player
				if (building->owner->isRelatedTo(state.getPlayer()) ==
				    Organisation::Relation::Hostile)
				{
					int numGuards = std::min(
					    20,
					    randBoundsInclusive(state.rng, building->owner->average_guards * 75 / 100,
					                        building->owner->average_guards * 125 / 100));

					for (int i = 0; i < numGuards; i++)
					{
						otherParticipants.emplace_back(
						    organisation, listRandomiser(state.rng, building->owner->guard_types));
					}
				}

				// Civilains will not be actually added if there is no spawn points for them
				{
					int numGuards =
					    std::min(20, randBoundsInclusive(
					                     state.rng, state.getCivilian()->average_guards * 75 / 100,
					                     state.getCivilian()->average_guards * 125 / 100));

					for (int i = 0; i < numGuards; i++)
					{
						otherParticipants.emplace_back(
						    state.getCivilian(),
						    listRandomiser(state.rng, state.getCivilian()->guard_types));
					}
				}

				missionType = Battle::MissionType::AlienExtermination;
			}
		}
		else
		{
			// Raid humans mission

			// Add building security
			{
				int numGuards = std::min(
				    20, randBoundsInclusive(state.rng, building->owner->average_guards * 75 / 100,
				                            building->owner->average_guards * 125 / 100));

				for (int i = 0; i < numGuards; i++)
				{
					otherParticipants.emplace_back(
					    organisation, listRandomiser(state.rng, building->owner->guard_types));
				}
			}

			missionType = Battle::MissionType::RaidHumans;
		}
	}

	// Create battle

	for (auto &pair : otherParticipants)
	{
		player_agents.push_back(state.agent_generator.createAgent(state, pair.first, pair.second));
	}

	return building->battle_map->createBattle(state, organisation, player_agents, craft,
	                                          missionType, building.id);
}

namespace
{

// Checks wether two cubes intersect
bool doTwoSectorsIntersect(int x1, int y1, int z1, const Vec3<int> &s1, int x2, int y2, int z2,
                           const Vec3<int> &s2)
{
	return x1 + s1.x > x2 && x2 + s2.x > x1 && z1 + s1.z > z2 && z2 + s2.z > z1 && y1 + s1.y > y2 &&
	       y2 + s2.y > y1;
}

bool doesCellIntersectSomething(std::vector<std::vector<std::vector<sp<BattleMapSector>>>> &sec_map,
                                const Vec3<int> &map_size, int x1, int y1, int z1)
{
	bool intersects = false;
	static Vec3<int> cell_size = {1, 1, 1};
	for (int x2 = 0; x2 < map_size.x; x2++)
		for (int y2 = 0; y2 < map_size.y; y2++)
			for (int z2 = 0; z2 < map_size.z; z2++)
				intersects = intersects || (sec_map[x2][y2][z2] &&
				                            doTwoSectorsIntersect(x1, y1, z1, cell_size, x2, y2, z2,
				                                                  sec_map[x2][y2][z2]->size));
	return intersects;
}

bool isMapComplete(std::vector<std::vector<std::vector<sp<BattleMapSector>>>> &sec_map,
                   const Vec3<int> &map_size)
{
	// We check if every single cell of a map intersects with at least some sector
	bool complete = true;
	for (int x1 = 0; x1 < map_size.x; x1++)
		for (int y1 = 0; y1 < map_size.y; y1++)
			for (int z1 = 0; z1 < map_size.z; z1++)
				complete = complete && doesCellIntersectSomething(sec_map, map_size, x1, y1, z1);
	return complete;
}

bool placeSector(GameState &state,
                 std::vector<std::vector<std::vector<sp<BattleMapSector>>>> &sec_map,
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

						if (!sec_map[x1][y1][z1])
							continue;
						bool can_move = false;

						// Try moving back on x
						can_move = true;
						for (int y2 = y1; y2 < y1 + sec_map[x1][y1][z1]->size.y; y2++)
							for (int z2 = z1; z2 < z1 + sec_map[x1][y1][z1]->size.z; z2++)
								can_move = can_move &&
								           !doesCellIntersectSomething(sec_map, map_size, x1 - dx1,
								                                       y2, z2);
						if (can_move)
						{
							sec_map[x1 - dx1][y1][z1] = sec_map[x1][y1][z1];
							sec_map[x1][y1][z1] = nullptr;
							moved = true;
							continue;
						}

						// Try moving back on y
						can_move = true;
						for (int x2 = x1; x2 < x1 + sec_map[x1][y1][z1]->size.x; x2++)
							for (int z2 = z1; z2 < z1 + sec_map[x1][y1][z1]->size.z; z2++)
								can_move = can_move &&
								           !doesCellIntersectSomething(sec_map, map_size, x2,
								                                       y1 - dy1, z2);
						if (can_move)
						{
							sec_map[x1][y1 - dy1][z1] = sec_map[x1][y1][z1];
							sec_map[x1][y1][z1] = nullptr;
							moved = true;
							continue;
						}

						// Try moving back on z
						can_move = true;
						for (int x2 = x1; x2 < x1 + sec_map[x1][y1][z1]->size.x; x2++)
							for (int y2 = z1; y2 < y1 + sec_map[x1][y1][z1]->size.y; y2++)
								can_move = can_move &&
								           !doesCellIntersectSomething(sec_map, map_size, x2, y2,
								                                       z1 - dz1);
						if (can_move)
						{
							sec_map[x1][y1][z1 - dz1] = sec_map[x1][y1][z1];
							sec_map[x1][y1][z1] = nullptr;
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
					// We check wether sector placed here does not intersect with others
					bool fits = true;
					for (int x2 = 0; x2 < map_size.x; x2++)
						for (int y2 = 0; y2 < map_size.y; y2++)
							for (int z2 = 0; z2 < map_size.z; z2++)
								fits = fits &&
								       (!sec_map[x2][y2][z2] ||
								        !doTwoSectorsIntersect(x1, y1, z1, sector->size, x2, y2, z2,
								                               sec_map[x2][y2][z2]->size));
					if (!fits)
						continue;

					// If we're still here, then it definetly fits
					possible_locations.emplace_back(x1, y1, z1);
				}
			}
		}

		if (possible_locations.size() == 0)
			continue;

		auto location = vectorRandomizer(state.rng, possible_locations);
		sec_map[location.x][location.y][location.z] = sector;

		return true;
	}
	return false;
}
} // anonymous-namespace

sp<Battle> BattleMap::createBattle(GameState &state, StateRef<Organisation> target_organisation,
                                   std::list<StateRef<Agent>> &agents,
                                   StateRef<Vehicle> player_craft, Battle::MissionType mission_type,
                                   UString mission_location_id)
{
	// Vanilla had vertical stacking of sectors planned, but not implemented. I will implement both
	// algorithms because I think that would be great to have. We could make it an extended game
	// option in the future.
	bool allow_vertical_stacking = true;

	// This switch will allow larger maps, +2 in size, which vanilla never did I think, and which is
	// required for some vertical stacking maps to actually spawn because they contain too many
	// mandatory sectors to fit into battle size even when enlarged by 1 on the smaller side
	bool allow_larger_maps = true;

	// This switch allows maps to spawn only one of the mandatory sectos instead of every single one
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
	// 2nd layer is just air (high ceiliing)
	// As we do not know them yet, I will generate maps in 3 modes for now: small, normal, big
	// Small being a -1 on the larger side and Big being +1 or +2 on the larger side.
	// +2 is required for some maps with vertical stacking to fit all the mandatory sectors
	// For now, this is random, in the future, this will be tied to the amount of troops
	int size_mod = randBoundsInclusive(state.rng, -1, 1);
	if (allow_larger_maps && size_mod == 1)
		size_mod += randBoundsInclusive(state.rng, 0, 1);
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
	auto size = this->max_battle_size;

	// Apply vertical stacking flag
	if (!allow_vertical_stacking)
	{
		// However, some maps have "mandatory" vertical stacking
		// That is, they only have sectors that are using more than one z chunk
		// We must check wether or not this map has any sectors that are 1-high on z
		bool foundUnstackedSector = false;
		for (int i = 1; i <= secCount; i++)
			foundUnstackedSector = foundUnstackedSector || (secRefs[i]->size.z == 1);
		if (foundUnstackedSector)
			size.z = 1;
	}
	auto normal_size = size;

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
	for (int attempt_make_map = 1; attempt_make_map <= 6; attempt_make_map++)
	{
		bool random_generation = false;
		switch (attempt_make_map)
		{
			// If we reached second attempt, it means we failed to create a map with modded size,
			// and must revert to normal
			case 2:
				size = normal_size;
				break;
			// If we reached third attempt, it means we failed to create a stacked map with normal
			// size, and should cancel vertical stacking.
			// If there was no vertical stacking, skip this attempt
			case 3:
				if (!allow_vertical_stacking)
					continue;
				size = modded_size;
				size.z = 1;
				break;
			// If we reached fourth attempt, it means we failed to create a map with modded size
			// without vertical stacking, and should try normal without stacking
			// If there was no vertical stacking, skip this attempt
			case 4:
				if (!allow_vertical_stacking)
					continue;
				size = normal_size;
				size.z = 1;
				break;
			// If we reached fifth attempt, it means we failed to create a random map and should
			// try a non-random map, filling sectors in big-to-small order, with vertical stacking
			case 5:
				size = normal_size;
				random_generation = false;
				break;
			// If we reached sixth attempt, it means we failed to create a random map and should
			// try a non-random map, filling sectors in big-to-small order, without vert. stacking
			case 6:
				if (!allow_vertical_stacking)
					continue;
				size = normal_size;
				size.z = 1;
				random_generation = false;
				break;
		}

		bool invert_x_packing = randBoundsInclusive(state.rng, 0, 1) == 1;
		bool invert_y_packing = randBoundsInclusive(state.rng, 0, 1) == 1;

		std::vector<std::vector<std::vector<sp<BattleMapSector>>>> sec_map = {
		    (unsigned)size.x, {(unsigned)size.y, {(unsigned)size.z, nullptr}}};
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

		// Disable sectors that don'pair fit
		bool mandatorySectorLost = false;
		bool mandatorySectorRemaining = false;
		for (int i = (int)remaining_sectors.size() - 1; i >= 0; i--)
		{
			sec_num_placed[remaining_sectors[i]] = 0;
			if (secRefs[remaining_sectors[i]]->size.x > size.x ||
			    secRefs[remaining_sectors[i]]->size.y > size.y ||
			    secRefs[remaining_sectors[i]]->size.z > size.z)
			{
				mandatorySectorLost =
				    mandatorySectorLost || (secRefs[remaining_sectors[i]]->occurrence_min > 0);
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
			           id.cStr(), size.x, size.y, size.z, attempt_make_map);
			continue;
		}

		// Place all mandatory sectors
		bool failed = false;
		for (int i = (int)remaining_sectors.size() - 1; i >= 0; i--)
		{
			if (failed)
				break;
			for (int j = 0; j < secRefs[remaining_sectors[i]]->occurrence_min; j++)
				failed = failed ||
				         !placeSector(state, sec_map, size, secRefs[remaining_sectors[i]], true,
				                      invert_x_packing, invert_y_packing);
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
			           id.cStr(), size.x, size.y, size.z, attempt_make_map);
			continue;
		}

		if (random_generation)
		{
			// Random map generator
			// Here we make two attemps to fill a map.
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
			LogWarning("Failed to complete map %s with size %d, %d, %d at attempt %d", id.cStr(),
			           size.x, size.y, size.z, attempt_make_map);
			continue;
		}

		LogWarning("Successfully completed map %s with size %d, %d, %d at attempt %d", id.cStr(),
		           size.x, size.y, size.z, attempt_make_map);

		// If we succeeded, time to actually create a battle map
		auto b = mksp<Battle>();

		b->currentPlayer = state.getPlayer();
		b->currentActiveOrganisation = state.getPlayer();
		b->currentTurn = 1;
		b->size = {chunk_size.x * size.x, chunk_size.y * size.y, chunk_size.z * size.z};
		b->spawnMap = {(unsigned)b->size.x,
		               {(unsigned)b->size.y, std::vector<int>((unsigned)b->size.z, 0)}};
		b->battle_map = {&state, id};
		b->mission_type = mission_type;
		b->mission_location_id = mission_location_id;
		b->player_craft = player_craft;
		b->loadResources(state);

		bool spawnCivilians = false;

		auto doors = std::vector<std::list<std::pair<Vec3<int>, sp<BattleMapPart>>>>(2);

		for (int x = 0; x < size.x; x++)
		{
			for (int y = 0; y < size.y; y++)
			{
				for (int z = 0; z < size.z; z++)
				{
					auto sec = sec_map[x][y][z];
					if (!sec)
						continue;
					if (!sec->tiles)
					{
						LogInfo("Loading sector tiles \"%s\"", sec->sectorTilesName.cStr());
						sec->tiles.reset(new BattleMapSectorTiles());
						if (!sec->tiles->loadSector(state, BattleMapSectorTiles::mapSectorPath +
						                                       "/" + sec->sectorTilesName))
						{
							LogError("Failed to load sector tiles \"%s\"",
							         sec->sectorTilesName.cStr());
						}
					}
					else
					{
						LogInfo("Using already-loaded sector tiles \"%s\"",
						        sec->sectorTilesName.cStr());
					}
					auto &tiles = *sec->tiles;
					Vec3<int> shift = {x * chunk_size.x, y * chunk_size.y, z * chunk_size.z};

					for (auto &pair : tiles.initial_grounds)
					{
						auto s = mksp<BattleMapPart>();

						s->initialPosition = pair.first + shift;
						s->currentPosition = s->initialPosition;
						s->currentPosition += Vec3<float>(0.5f, 0.5f, 0.0f);

						// Check wether this is an exit location, and if so,
						// replace the ground map part with an appropriate exit
						bool canExit = s->currentPosition.z >= exit_level_min &&
						               s->currentPosition.z <= exit_level_max;
						canExit =
						    canExit &&
						    (s->currentPosition.x > 0 || allow_exit[Battle::MapBorder::West]) &&
						    (s->currentPosition.y > 0 || allow_exit[Battle::MapBorder::North]) &&
						    (s->currentPosition.x < size.x * chunk_size.x - 1 ||
						     allow_exit[Battle::MapBorder::East]) &&
						    (s->currentPosition.y < size.y * chunk_size.y - 1 ||
						     allow_exit[Battle::MapBorder::South]);
						if (canExit)
						{
							Vec3<int> exitLocX = s->currentPosition;
							Vec3<int> exitLocY = s->currentPosition;
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

						// Set spawnability and height
						if (s->type->movement_cost == 255 || s->type->height == 39 ||
						    b->spawnMap[s->initialPosition.x][s->initialPosition.y]
						               [s->initialPosition.z] == -1)
						{
							b->spawnMap[s->initialPosition.x][s->initialPosition.y]
							           [s->initialPosition.z] = -1;
						}
						else
						{
							b->spawnMap[s->initialPosition.x][s->initialPosition.y]
							           [s->initialPosition.z] =
							    std::max(b->spawnMap[s->initialPosition.x][s->initialPosition.y]
							                        [s->initialPosition.z],
							             s->type->height);
						}

						b->map_parts.push_back(s);
					}
					for (auto &pair : tiles.initial_left_walls)
					{
						auto s = mksp<BattleMapPart>();

						s->initialPosition = pair.first + shift;
						s->currentPosition = s->initialPosition;
						s->currentPosition += Vec3<float>(0.5f, 0.5f, 0.0f);
						s->type = pair.second;

						if (pair.second->door)
						{
							doors[0].emplace_back(s->initialPosition, s);
						}

						b->map_parts.push_back(s);
					}
					for (auto &pair : tiles.initial_right_walls)
					{
						auto s = mksp<BattleMapPart>();

						s->initialPosition = pair.first + shift;
						s->currentPosition = s->initialPosition;
						s->currentPosition += Vec3<float>(0.5f, 0.5f, 0.0f);
						s->type = pair.second;

						if (pair.second->door)
						{
							doors[1].emplace_back(s->initialPosition, s);
						}

						b->map_parts.push_back(s);
					}
					for (auto &pair : tiles.initial_features)
					{
						auto s = mksp<BattleMapPart>();

						s->initialPosition = pair.first + shift;
						s->currentPosition = s->initialPosition;
						s->currentPosition += Vec3<float>(0.5f, 0.5f, 0.0f);
						s->type = pair.second;

						// Set spawnability and height
						if (s->type->movement_cost == 255 || s->type->height == 39 ||
						    b->spawnMap[s->initialPosition.x][s->initialPosition.y]
						               [s->initialPosition.z] == -1)
						{
							b->spawnMap[s->initialPosition.x][s->initialPosition.y]
							           [s->initialPosition.z] = -1;
						}
						else
						{
							b->spawnMap[s->initialPosition.x][s->initialPosition.y]
							           [s->initialPosition.z] =
							    std::max(b->spawnMap[s->initialPosition.x][s->initialPosition.y]
							                        [s->initialPosition.z],
							             s->type->height);
						}

						b->map_parts.push_back(s);
					}
					for (auto &pair : tiles.loot_locations)
					{
						if (target_organisation->loot[pair.second].size() == 0)
							continue;
						auto l =
						    vectorRandomizer(state.rng, target_organisation->loot[pair.second]);
						if (!l)
							continue;
						auto i = mksp<AEquipment>();
						i->type = l;

						auto bi = b->addItem(state);
						bi->item = i;
						bi->position = pair.first + shift;
						bi->falling = false;
					}
					for (auto &tlb : tiles.los_blocks)
					{
						auto lb = tlb->clone(shift);
						b->los_blocks.push_back(lb);
						// At least one civilian spawner required for map to spawn civilians
						if (lb->spawn_type ==
						    BattleMapSector::LineOfSightBlock::SpawnType::Civilian)
						{
							spawnCivilians = true;
						}
						// Los block must touch map edge, and it's lowest z must be within spawn
						// allowance
						// in order for it to qualify for spawning X-Com agents
						else if (lb->spawn_priority > 0 &&
						         lb->spawn_type ==
						             BattleMapSector::LineOfSightBlock::SpawnType::Player)
						{
							bool canSpawn = lb->start.z >= entrance_level_min &&
							                lb->start.z < entrance_level_max;
							canSpawn = canSpawn &&
							           (lb->start.x == 0 || lb->end.x == size.x * chunk_size.x ||
							            lb->start.y == 0 || lb->end.y == size.y * chunk_size.y);
							if (!canSpawn)
							{
								lb->spawn_priority = 0;
							}
						}
					}
				}
			}
		}

		// Link doors up
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
					d->animationFrameCount = std::max(
					    d->animationFrameCount, (int)it->second->type->animation_frames.size());
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

		// Agents are just added to squads in default way here.
		// Player will be allowed a chance to equip them and assign to squads how they prefer
		// We will assign their positions and "spawn" them in "BeginBattle" function
		for (auto &a : agents)
		{
			if (b->participants.find(a->owner) == b->participants.end())
			{
				b->participants.insert(a->owner);
			}

			auto u = b->addUnit(state);

			u->agent = a;
			u->agent->unit = {&state, u->id};
			u->owner = a->owner;
			u->squadNumber = -1;
			u->updateDisplayedItem();
			if (!spawnCivilians && a->owner == state.getCivilian())
			{
				u->retreated = true;
			}
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
			if (u.second->retreated)
			{
				continue;
			}
			agentCount[u.second->owner]++;
		}

		// Distribute agents into squads
		for (auto &o : b->participants)
		{
			// First add agents in groups of threes
			// If only 2 remain, add them still
			// If only 1 remains, do not add them
			for (int s = 0; s < 6; s++)
			{
				if (agentCount[o] == 0)
					break;
				if (agentCount[o] > 1)
				{
					for (auto &u : b->units)
					{
						if (b->forces[o].squads[s].getNumUnits() >= 3)
							break;
						if (u.second->owner != o || u.second->squadNumber != -1 ||
						    u.second->retreated)
							continue;
						u.second->assignToSquad(*b, s);
						agentCount[o]--;
						if (agentCount[o] == 0)
							break;
					}
				}
			}
			// Now fill squads with remaining agents
			for (int s = 0; s < 6; s++)
			{
				if (agentCount[o] == 0)
					break;
				for (auto &u : b->units)
				{
					if (b->forces[o].squads[s].getNumUnits() == 6)
						break;
					if (u.second->owner != o || u.second->squadNumber != -1)
						continue;
					u.second->assignToSquad(*b, s);
					agentCount[o]--;
					if (agentCount[o] == 0)
						break;
				}
			}
		}

		// Unload sector tiles
		for (auto &s : sectors)
			s.second->tiles = nullptr;

		LogInfo("Unloaded sector tiles.");

		return b;
	}
	LogError("Failed to create map %s", id.cStr());
	return nullptr;
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
		auto tilesetPath = BattleMapTileset::tilesetPath + "/" + tilesetName;
		LogInfo("Loading tileset \"%s\" from \"%s\"", tilesetName.cStr(), tilesetPath.cStr());
		BattleMapTileset tileset;
		if (!tileset.loadTileset(state, tilesetPath))
		{
			LogError("Failed to load tileset \"%s\" from \"%s\"", tilesetName.cStr(),
			         tilesetPath.cStr());
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
					tile->destroyed_map_parts.push_back(destroyed_ground_tile);
					break;
				case BattleMapPartType::Type::LeftWall:
					tile->destroyed_map_parts = rubble_left_wall;
					break;
				case BattleMapPartType::Type::RightWall:
					tile->destroyed_map_parts = rubble_right_wall;
					break;
				case BattleMapPartType::Type::Feature:
					tile->destroyed_map_parts = rubble_feature;
					break;
			}
			tile->damageModifier = {&state, "DAMAGEMODIFIER_TERRAIN_1_"};
			// Sanity check
			if (state.battleMapTiles.find(tileName) != state.battleMapTiles.end())
			{
				LogError("Duplicate tile with ID \"%s\"", tileName.cStr());
				continue;
			}
			state.battleMapTiles.emplace(tileName, tile);
			count++;
		}
		LogInfo("Loaded %u tiles from tileset \"%s\"", count, tilesetName.cStr());
	}
}

void BattleMap::unloadTilesets(GameState &state)
{
	state.battleMapTiles.clear();
	LogInfo("Unloaded all tilesets.");
}
}

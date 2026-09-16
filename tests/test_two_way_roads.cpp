#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/city/city.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/tilemap/tile.h"
#include "game/state/tilemap/tilemap.h"
#include <glm/glm.hpp>
#include <iostream>

using namespace OpenApoc;

namespace
{

UString commonName;
UString gamestateName;
int failures = 0;

bool check(bool condition, const UString &what)
{
	if (condition)
	{
		LogWarning("PASS: {0}", what);
	}
	else
	{
		LogWarning("FAIL: {0}", what);
		failures++;
	}
	return condition;
}

// Each case places vehicles and runs the clock, so every case gets its own state rather than
// inheriting the previous case's traffic and game time.
sp<GameState> loadState()
{
	auto state = mksp<GameState>();
	if (!state->loadGame(commonName) || !state->loadGame(gamestateName))
	{
		LogError("Failed to load gamestate");
		return nullptr;
	}
	state->startGame();
	state->initState();
	state->fillPlayerStartingProperty();
	return state;
}

struct StraightRun
{
	std::vector<Vec3<int>> tiles;
	Vec3<int> dir;
};

// Uses the game's own passability helper rather than reimplementing road connectivity, so the
// run found here is one real pathfinding would also accept.
bool findStraightRoadRun(GameState &state, int minLength, StraightRun &out,
                         bool straightTilesOnly = false)
{
	auto &map = *state.current_city->map;
	GroundVehicleTileHelper helper(map, VehicleType::Type::Road);
	std::vector<Vec3<int>> axes = {{1, 0, 0}, {0, 1, 0}};

	for (int z = 0; z < map.size.z; z++)
	{
		for (int y = 0; y < map.size.y; y++)
		{
			for (int x = 0; x < map.size.x; x++)
			{
				Vec3<int> start = {x, y, z};
				auto startTile = map.getTile(start);
				if (!startTile->presentScenery ||
				    startTile->presentScenery->type->tile_type != SceneryTileType::TileType::Road)
				{
					continue;
				}
				if (straightTilesOnly && startTile->presentScenery->type->road_type !=
				                             SceneryTileType::RoadType::StraightBend)
				{
					continue;
				}
				for (const auto &dir : axes)
				{
					std::vector<Vec3<int>> run = {start};
					Vec3<int> cur = start;
					while ((int)run.size() < minLength)
					{
						Vec3<int> next = cur + dir;
						if (!map.tileIsValid(next))
						{
							break;
						}
						if (!helper.canEnterTile(map.getTile(cur), map.getTile(next)))
						{
							break;
						}
						auto nextTile = map.getTile(next);
						if (straightTilesOnly && (!nextTile->presentScenery ||
						                          nextTile->presentScenery->type->road_type !=
						                              SceneryTileType::RoadType::StraightBend))
						{
							break;
						}
						run.push_back(next);
						cur = next;
					}
					if ((int)run.size() >= minLength)
					{
						out.tiles = run;
						out.dir = dir;
						return true;
					}
				}
			}
		}
	}
	return false;
}

float lateralOffset(const Vec3<float> &position, const Vec3<int> &tile, const Vec3<int> &dir)
{
	if (dir.x != 0)
	{
		return position.y - (tile.y + 0.5f);
	}
	return position.x - (tile.x + 0.5f);
}

// Two vehicles driving in opposite directions down the same straight road must not share a
// centre line.
void testLaneSeparation()
{
	auto state = loadState();
	if (!check(state != nullptr, "lane separation: gamestate loaded"))
	{
		return;
	}

	StraightRun run;
	if (!check(findStraightRoadRun(*state, 6, run), "lane separation: found a straight road run"))
	{
		return;
	}

	StateRef<VehicleType> vType{state.get(), UString("VEHICLETYPE_AUTOTRANS")};
	if (!check((bool)vType, "lane separation: VEHICLETYPE_AUTOTRANS exists"))
	{
		return;
	}

	auto &map = *state->current_city->map;
	Vec3<int> first = run.tiles.front();
	Vec3<int> last = run.tiles.back();

	auto forward = state->current_city->placeVehicle(*state, vType, state->getPlayer(),
	                                                 map.getTile(first)->getRestingPosition());
	auto backward = state->current_city->placeVehicle(*state, vType, state->getPlayer(),
	                                                  map.getTile(last)->getRestingPosition());
	if (!check(forward && backward, "lane separation: test vehicles placed"))
	{
		return;
	}

	forward->setMission(*state, VehicleMission::gotoLocation(*state, *forward, last));
	backward->setMission(*state, VehicleMission::gotoLocation(*state, *backward, first));

	Vec3<int> midTile = run.tiles[run.tiles.size() / 2];
	bool sawForward = false;
	bool sawBackward = false;
	float forwardLateral = 0.0f;
	float backwardLateral = 0.0f;

	for (int i = 0; i < 400 && !(sawForward && sawBackward); i++)
	{
		state->update(TICKS_PER_SECOND / 4);

		if (!sawForward && (Vec3<int>)forward->position == midTile)
		{
			sawForward = true;
			forwardLateral = lateralOffset(forward->position, midTile, run.dir);
		}
		if (!sawBackward && (Vec3<int>)backward->position == midTile)
		{
			sawBackward = true;
			backwardLateral = lateralOffset(backward->position, midTile, run.dir);
		}
	}

	if (!check(sawForward && sawBackward, "lane separation: both vehicles crossed the mid tile"))
	{
		return;
	}

	LogWarning("lane separation: forward lateral = {0}, backward lateral = {1}", forwardLateral,
	           backwardLateral);

	check(std::abs(forwardLateral) > 0.1f && std::abs(backwardLateral) > 0.1f,
	      "lane separation: both vehicles are off the tile centre line");
	check((forwardLateral > 0.0f) != (backwardLateral > 0.0f),
	      "lane separation: opposing vehicles use opposite lanes");
}

struct FollowerRun
{
	bool valid = false;
	// Updates on which the follower sat in the leader's tile on the leader's own side of the road
	// while the leader was still moving.
	int sameLaneOverlaps = 0;
	// Updates on which the follower drove on the side of the road opposite to the leader.
	int opposingLaneSamples = 0;
	// Updates on which the two were close enough for the follower to be queueing at all.
	int closeSamples = 0;
	// The follower went back to the leader's side of the road after using the other one.
	bool mergedBack = false;
	bool followerAhead = false;
};

// Puts a slow leader one tile in front of a faster follower on a straight road and sends both to
// the far end of it, recording how the follower behaves while it is caught up behind the leader.
FollowerRun runFollowerScenario(bool twoWayRoads, int runLength, int updates,
                                bool straightTilesOnly = false)
{
	FollowerRun result;

	config().set("OpenApoc.NewFeature.TwoWayRoads", twoWayRoads);

	auto state = loadState();
	if (!state)
	{
		return result;
	}

	StraightRun run;
	if (!findStraightRoadRun(*state, runLength, run, straightTilesOnly))
	{
		LogError("follower scenario: no straight road run of {0} tiles", runLength);
		return result;
	}

	// Road vehicle types carry no top_speed of their own - all of it comes from the engine the
	// vehicle is built with - so the pair is chosen by name and the speed gap checked once placed.
	StateRef<VehicleType> slowType{state.get(), UString("VEHICLETYPE_AUTOTRANS")};
	StateRef<VehicleType> fastType{state.get(), UString("VEHICLETYPE_BLAZER_TURBO_BIKE")};
	if (!slowType || !fastType)
	{
		LogError("follower scenario: expected road vehicle types missing");
		return result;
	}

	auto &map = *state->current_city->map;
	Vec3<int> target = run.tiles.back();

	auto follower = state->current_city->placeVehicle(
	    *state, fastType, state->getPlayer(), map.getTile(run.tiles[0])->getRestingPosition());
	auto leader = state->current_city->placeVehicle(
	    *state, slowType, state->getPlayer(), map.getTile(run.tiles[1])->getRestingPosition());
	if (!follower || !leader)
	{
		LogError("follower scenario: failed to place vehicles");
		return result;
	}

	if (follower->getSpeed() <= leader->getSpeed())
	{
		LogError("follower scenario: follower speed {0} does not exceed leader speed {1}",
		         follower->getSpeed(), leader->getSpeed());
		return result;
	}
	LogWarning("follower scenario: leader speed {0}, follower speed {1}", leader->getSpeed(),
	           follower->getSpeed());

	leader->setMission(*state, VehicleMission::gotoLocation(*state, *leader, target));
	follower->setMission(*state, VehicleMission::gotoLocation(*state, *follower, target));

	Vec3<float> axis = {(float)run.dir.x, (float)run.dir.y, 0.0f};

	for (int i = 0; i < updates; i++)
	{
		state->update(TICKS_PER_SECOND / 4);
		// Once either vehicle parks on the shared destination the other simply arrives on top of
		// it, which says nothing about how traffic behaves en route.
		if (!follower->tileObject || !leader->tileObject || follower->missions.empty() ||
		    leader->missions.empty())
		{
			break;
		}

		Vec3<int> followerTile = follower->position;
		Vec3<int> leaderTile = leader->position;
		float followerLateral = lateralOffset(follower->position, followerTile, run.dir);
		float leaderLateral = lateralOffset(leader->position, leaderTile, run.dir);
		bool opposingLanes = followerLateral * leaderLateral < -0.01f;
		bool leaderMoving = glm::length(leader->velocity) > 0.001f;

		Vec3<float> gap = follower->position - leader->position;
		if (std::abs(gap.x * axis.x + gap.y * axis.y) < 2.0f)
		{
			result.closeSamples++;
		}
		if (opposingLanes)
		{
			result.opposingLaneSamples++;
		}
		else if (result.opposingLaneSamples > 0 && followerLateral * leaderLateral > 0.01f)
		{
			result.mergedBack = true;
		}
		if (followerTile == leaderTile && leaderMoving && !opposingLanes)
		{
			result.sameLaneOverlaps++;
		}
		if (gap.x * axis.x + gap.y * axis.y > 0.5f)
		{
			result.followerAhead = true;
		}
	}

	result.valid = true;
	config().set("OpenApoc.NewFeature.TwoWayRoads", true);
	return result;
}

// A faster vehicle must queue behind a slower one in its own lane instead of driving through it.
void testEnRouteBlocking()
{
	auto on = runFollowerScenario(true, 12, 600);
	if (!check(on.valid, "en-route blocking: scenario ran with TwoWayRoads on"))
	{
		return;
	}
	LogWarning("en-route blocking: TwoWayRoads on -> sameLaneOverlaps = {0}, closeSamples = {1}",
	           on.sameLaneOverlaps, on.closeSamples);
	check(on.closeSamples > 0, "en-route blocking: the follower actually caught the leader");
	check(on.sameLaneOverlaps == 0,
	      "en-route blocking: the follower never shares the moving leader's tile in its lane");

	auto off = runFollowerScenario(false, 12, 600);
	if (!check(off.valid, "en-route blocking: scenario ran with TwoWayRoads off"))
	{
		return;
	}
	LogWarning("en-route blocking: TwoWayRoads off -> sameLaneOverlaps = {0}, closeSamples = {1}",
	           off.sameLaneOverlaps, off.closeSamples);
	check(off.sameLaneOverlaps > 0,
	      "en-route blocking: with the option off the follower drives through the leader");
}

// A faster vehicle stuck behind a slower one on a clear straight must pass it on the other side
// of the road and then pull back in.
void testOvertaking()
{
	auto on = runFollowerScenario(true, 16, 900, true);
	if (!check(on.valid, "overtaking: scenario ran with TwoWayRoads on"))
	{
		return;
	}
	LogWarning("overtaking: TwoWayRoads on -> opposingLaneSamples = {0}, mergedBack = {1}, "
	           "followerAhead = {2}",
	           on.opposingLaneSamples, on.mergedBack, on.followerAhead);
	check(on.opposingLaneSamples > 0, "overtaking: the follower used the opposing lane");
	check(on.mergedBack, "overtaking: the follower merged back into its own lane");
	check(on.followerAhead, "overtaking: the follower finished the pass in front of the leader");
}

struct HeadOnRun
{
	bool valid = false;
	int forwardHits = 0;
	int backwardHits = 0;
};

// Drives two vehicles into each other on one centre line with lanes turned off, which is the
// simplest way to make two ground vehicles actually touch.
HeadOnRun runHeadOnScenario(bool collisions)
{
	HeadOnRun result;

	config().set("OpenApoc.NewFeature.TwoWayRoads", false);
	config().set("OpenApoc.NewFeature.VehicleCollisions", collisions);

	auto state = loadState();
	StraightRun run;
	if (!state || !findStraightRoadRun(*state, 8, run))
	{
		LogError("head-on scenario: no straight road run");
		config().set("OpenApoc.NewFeature.TwoWayRoads", true);
		config().set("OpenApoc.NewFeature.VehicleCollisions", false);
		return result;
	}

	StateRef<VehicleType> vType{state.get(), UString("VEHICLETYPE_AUTOTRANS")};
	auto &map = *state->current_city->map;
	Vec3<int> first = run.tiles.front();
	Vec3<int> last = run.tiles.back();

	auto forward = state->current_city->placeVehicle(*state, vType, state->getPlayer(),
	                                                 map.getTile(first)->getRestingPosition());
	auto backward = state->current_city->placeVehicle(*state, vType, state->getPlayer(),
	                                                  map.getTile(last)->getRestingPosition());
	if (!forward || !backward)
	{
		LogError("head-on scenario: failed to place vehicles");
		config().set("OpenApoc.NewFeature.TwoWayRoads", true);
		config().set("OpenApoc.NewFeature.VehicleCollisions", false);
		return result;
	}

	forward->setMission(*state, VehicleMission::gotoLocation(*state, *forward, last));
	backward->setMission(*state, VehicleMission::gotoLocation(*state, *backward, first));

	int forwardHealth = forward->getHealth();
	int backwardHealth = backward->getHealth();

	for (int i = 0; i < 400; i++)
	{
		state->update(TICKS_PER_SECOND / 4);
		if (!forward->tileObject || !backward->tileObject)
		{
			break;
		}
		if (forward->getHealth() < forwardHealth)
		{
			result.forwardHits++;
			forwardHealth = forward->getHealth();
		}
		if (backward->getHealth() < backwardHealth)
		{
			result.backwardHits++;
			backwardHealth = backward->getHealth();
		}
	}

	result.valid = true;
	config().set("OpenApoc.NewFeature.TwoWayRoads", true);
	config().set("OpenApoc.NewFeature.VehicleCollisions", false);
	return result;
}

// Vehicles that drive into each other take damage once per contact, and only with the option on.
void testVehicleCollisions()
{
	auto on = runHeadOnScenario(true);
	if (!check(on.valid, "collisions: scenario ran with VehicleCollisions on"))
	{
		return;
	}
	LogWarning("collisions: VehicleCollisions on -> forwardHits = {0}, backwardHits = {1}",
	           on.forwardHits, on.backwardHits);
	check(on.forwardHits == 1 && on.backwardHits == 1,
	      "collisions: both vehicles are damaged exactly once by the one contact");

	auto off = runHeadOnScenario(false);
	if (!check(off.valid, "collisions: scenario ran with VehicleCollisions off"))
	{
		return;
	}
	LogWarning("collisions: VehicleCollisions off -> forwardHits = {0}, backwardHits = {1}",
	           off.forwardHits, off.backwardHits);
	check(off.forwardHits == 0 && off.backwardHits == 0,
	      "collisions: with the option off neither vehicle is damaged");
}

} // namespace

int main(int argc, char **argv)
{
	config().addPositionalArgument("common", "Common gamestate to load");
	config().addPositionalArgument("gamestate", "Gamestate to load");

	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	gamestateName = config().getString("gamestate");
	commonName = config().getString("common");
	if (gamestateName.empty() || commonName.empty())
	{
		std::cerr << "Must provide common and gamestate\n";
		config().showHelp();
		return EXIT_FAILURE;
	}

	Framework fw("OpenApoc", false);

	testLaneSeparation();
	testEnRouteBlocking();
	testOvertaking();
	testVehicleCollisions();

	if (failures > 0)
	{
		LogWarning("test_two_way_roads: {0} failure(s)", failures);
		return failures;
	}
	LogWarning("test_two_way_roads success - all cases passed");
	return EXIT_SUCCESS;
}

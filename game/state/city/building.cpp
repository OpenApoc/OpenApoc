#include "game/state/city/building.h"
#include "framework/framework.h"
#include "game/state/base/base.h"
#include "game/state/city/city.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/organisation.h"

namespace OpenApoc
{

sp<BuildingFunction> BuildingFunction::get(const GameState &state, const UString &id)
{
	auto it = state.building_functions.find(id);
	if (it == state.building_functions.end())
	{
		LogError("No building_function matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &BuildingFunction::getPrefix()
{
	static UString prefix = "BUILDINGFUNCTION_";
	return prefix;
}
const UString &BuildingFunction::getTypeName()
{
	static UString name = "AgentType";
	return name;
}

sp<Building> Building::get(const GameState &state, const UString &id)
{
	for (auto &city : state.cities)
	{
		auto it = city.second->buildings.find(id);
		if (it != city.second->buildings.end())
			return it->second;
	}

	LogError("No building type matching ID \"%s\"", id);
	return nullptr;
}

const UString &Building::getPrefix()
{
	static UString prefix = "BUILDING_";
	return prefix;
}
const UString &Building::getTypeName()
{
	static UString name = "Building";
	return name;
}

const UString &Building::getId(const GameState &state, const sp<Building> ptr)
{
	static const UString emptyString = "";
	for (auto &c : state.cities)
	{
		for (auto &b : c.second->buildings)
		{
			if (b.second == ptr)
				return b.first;
		}
	}
	LogError("No building matching pointer %p", ptr.get());
	return emptyString;
}

bool Building::hasAliens() const
{
	if (!preset_crew.empty())
	{
		return true;
	}
	for (auto &pair : current_crew)
	{
		if (pair.second > 0)
		{
			return true;
		}
	}
	return false;
}

void Building::updateDetection(GameState &state, unsigned int ticks)
{
	if (ticksDetectionTimeOut > 0)
	{
		if (ticksDetectionTimeOut < ticks)
		{
			ticksDetectionTimeOut = 0;
		}
		else
		{
			ticksDetectionTimeOut -= ticks;
		}
		return;
	}
	detected = false;
	ticksDetectionAttemptAccumulated += ticks;
	while (ticksDetectionAttemptAccumulated >= TICKS_PER_DETECTION_ATTEMPT[state.difficulty])
	{
		ticksDetectionAttemptAccumulated -= TICKS_PER_DETECTION_ATTEMPT[state.difficulty];
		detect(state, state.firstDetection || owner == state.getPlayer());
	}
}

void Building::detect(GameState &state, bool forced)
{
	if (ticksDetectionTimeOut > 0)
	{
		return;
	}
	if (!forced)
	{
		bool alien_spotted = false;
		int detectionValue = 0;
		for (auto &pair : current_crew)
		{
			if (pair.second == 0)
			{
				continue;
			}
			if (randBoundsExclusive(state.rng, 0, 100) < 100 / (pair.second + 1))
			{
				alien_spotted = true;
			}
			detectionValue += pair.first->detectionWeight * pair.second;
		}
		if (!alien_spotted)
		{
			return;
		}
		detectionValue *= function->detectionWeight - 2 * state.difficulty;
		detectionValue /= 100;
		if (owner->isRelatedTo(state.getAliens()) == Organisation::Relation::Allied)
		{
			detectionValue /= 2;
		}
		if (owner->isRelatedTo(state.getPlayer()) == Organisation::Relation::Hostile)
		{
			detectionValue /= 2;
		}
		if (owner->takenOver)
		{
			detectionValue /= 2;
		}
		if (randBoundsExclusive(state.rng, 0, 100) >= detectionValue - 10)
		{
			return;
		}
	}
	else
	{
		bool alien_spotted = false;
		for (auto &pair : current_crew)
		{
			if (pair.second == 0)
			{
				continue;
			}
			alien_spotted = true;
			break;
		}
		if (!alien_spotted)
		{
			return;
		}
	}
	state.firstDetection = false;
	ticksDetectionTimeOut = TICKS_DETECTION_TIMEOUT;
	StateRef<Base> base;
	if (owner == state.getPlayer())
	{
		auto thisSP = shared_from_this();
		for (auto &b : state.player_bases)
		{
			if (b.second->building == thisSP)
			{
				base = {&state, b.first};
				break;
			}
		}
	}
	if (base)
	{
		auto event = new GameDefenseEvent(GameEventType::DefendTheBase, base, state.getAliens());
		fw().pushEvent(event);
	}
	else
	{
		detected = true;

		auto event = new GameBuildingEvent(GameEventType::AlienSpotted,
		                                   {&state, Building::getId(state, shared_from_this())});
		fw().pushEvent(event);
	}
}

void Building::alienGrowth(GameState &state)
{
	// Calculate changes to building's crew
	std::map<StateRef<AgentType>, int> change_crew;
	for (auto &pair : current_crew)
	{
		int growth = 0;
		for (int i = 0; i < pair.second; i++)
		{
			if (randBoundsInclusive(state.rng, 0, 100) < pair.first->growthChance)
			{
				growth++;
			}
		}
		if (growth == 0)
		{
			continue;
		}
		// Growing aliens die
		change_crew[pair.first] -= growth;
		// And are replaced with a random one from available options
		// (or simply die if they don't have any)
		int rand = randBoundsExclusive(state.rng, 0, 100);
		for (auto &g : pair.first->growthOptions)
		{
			if (rand < g.first)
			{
				change_crew[g.second.first] += g.second.second * growth;
				break;
			}
		}
		// Additionally, suckers apply infiltration here
		if (pair.first->growthInfiltration > 0)
		{
			owner->infiltrationValue += growth * pair.first->growthInfiltration *
			                            function->infiltrationSpeed * owner->infiltrationSpeed;
			if (owner->infiltrationValue > 200)
			{
				owner->infiltrationValue = 200;
			}
		}
	}
	// Apply changes
	for (auto &pair : change_crew)
	{
		current_crew[pair.first] += pair.second;
	}
	// Disable detection if no aliens are there
	detected = detected && hasAliens();
}

} // namespace OpenApoc

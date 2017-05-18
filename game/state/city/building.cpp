#include "game/state/city/building.h"
#include "framework/framework.h"
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
	ticksDetectionAttemptAccumulated += ticks;
	while (ticksDetectionAttemptAccumulated >= TICKS_PER_DETECTION_ATTEMPT[state.difficulty])
	{
		ticksDetectionAttemptAccumulated -= TICKS_PER_DETECTION_ATTEMPT[state.difficulty];
		detect(state, state.firstDetection);
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
	state.firstDetection = false;
	ticksDetectionTimeOut = TICKS_DETECTION_TIMEOUT;

	auto event = new GameBuildingEvent(GameEventType::AlienSpotted,
	                                   {&state, Building::getId(state, shared_from_this())});
	fw().pushEvent(event);
}

} // namespace OpenApoc

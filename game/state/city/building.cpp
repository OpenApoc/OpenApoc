#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"

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

} // namespace OpenApoc

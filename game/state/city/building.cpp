#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

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

#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

template <> sp<Building> StateObject<Building>::get(const GameState &state, const UString &id)
{
	for (auto &city : state.cities)
	{
		auto it = city.second->buildings.find(id);
		if (it != city.second->buildings.end())
			return it->second;
	}

	LogError("No building type matching ID \"%s\"", id.cStr());
	return nullptr;
}

template <> const UString &StateObject<Building>::getPrefix()
{
	static UString prefix = "BUILDING_";
	return prefix;
}
template <> const UString &StateObject<Building>::getTypeName()
{
	static UString name = "Building";
	return name;
}

template <>
const UString &StateObject<Building>::getId(const GameState &state, const sp<Building> ptr)
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

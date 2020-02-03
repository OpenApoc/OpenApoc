#include "game/state/rules/city/ufogrowth.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

template <> sp<UFOGrowth> StateObject<UFOGrowth>::get(const GameState &state, const UString &id)
{
	auto it = state.ufo_growth_lists.find(id);
	if (it == state.ufo_growth_lists.end())
	{
		LogError("No ufo growth matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<UFOGrowth>::getPrefix()
{
	static UString prefix = "UFO_GROWTH_";
	return prefix;
}
template <> const UString &StateObject<UFOGrowth>::getTypeName()
{
	static UString name = "UFOGrowth";
	return name;
}
}; // namespace OpenApoc

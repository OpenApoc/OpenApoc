#include "game/state/rules/city/ufomissionpreference.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

template <>
sp<UFOMissionPreference> StateObject<UFOMissionPreference>::get(const GameState &state,
                                                                const UString &id)
{
	auto it = state.ufo_mission_preference.find(id);
	if (it == state.ufo_mission_preference.end())
	{
		LogError("No UFOMissionPreference matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<UFOMissionPreference>::getPrefix()
{
	static UString prefix = "UFO_MISSION_PREFERENCE_";
	return prefix;
}
template <> const UString &StateObject<UFOMissionPreference>::getTypeName()
{
	static UString name = "UFOMissionPreference";
	return name;
}
}; // namespace OpenApoc

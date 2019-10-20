#include "game/state/rules/city/ufomissionpreference.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

sp<UFOMissionPreference> UFOMissionPreference::get(const GameState &state, const UString &id)
{
	auto it = state.ufo_mission_preference.find(id);
	if (it == state.ufo_mission_preference.end())
	{
		LogError("No UFOMissionPreference matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &UFOMissionPreference::getPrefix()
{
	static UString prefix = "UFO_MISSION_PREFERENCE_";
	return prefix;
}
const UString &UFOMissionPreference::getTypeName()
{
	static UString name = "UFOMissionPreference";
	return name;
}
}; // namespace OpenApoc

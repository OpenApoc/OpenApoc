#include "game/state/rules/city/ufoincursion.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

sp<UFOIncursion> UFOIncursion::get(const GameState &state, const UString &id)
{
	auto it = state.ufo_incursions.find(id);
	if (it == state.ufo_incursions.end())
	{
		LogError("No incursion rule matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &UFOIncursion::getPrefix()
{
	static UString prefix = "UFO_INCURSION_";
	return prefix;
}
const UString &UFOIncursion::getTypeName()
{
	static UString name = "UFOIncursion";
	return name;
}
}; // namespace OpenApoc

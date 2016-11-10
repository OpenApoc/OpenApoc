#include "game/state/rules/ufo_incursion.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

const std::map<UFOIncursion::PrimaryMission, UString> UFOIncursion::primaryMissionMap = {
    {PrimaryMission::Infiltration, "infiltration"},
    {PrimaryMission::Subversion, "subversion"},
    {PrimaryMission::Attack, "attack"},
    {PrimaryMission::Overspawn, "overspawn"}};

template <>
sp<UFOIncursion> StateObject<UFOIncursion>::get(const GameState &state, const UString &id)
{
	auto it = state.ufo_incursions.find(id);
	if (it == state.ufo_incursions.end())
	{
		LogError("No incursion rule matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<UFOIncursion>::getPrefix()
{
	static UString prefix = "UFO_INCURSION_";
	return prefix;
}
template <> const UString &StateObject<UFOIncursion>::getTypeName()
{
	static UString name = "UFOIncursion";
	return name;
}
}; // namespace OpenApoc

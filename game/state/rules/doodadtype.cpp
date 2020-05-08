#include "game/state/rules/doodadtype.h"
#include "game/state/gamestate.h"
#include "library/strings.h"

namespace OpenApoc
{
template <>

sp<DoodadType> StateObject<DoodadType>::get(const GameState &state, const UString &id)
{
	auto it = state.doodad_types.find(id);
	if (it == state.doodad_types.end())
	{
		LogError("No doodad type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<DoodadType>::getPrefix()
{
	static UString prefix = "DOODAD_";
	return prefix;
}
template <> const UString &StateObject<DoodadType>::getTypeName()
{
	static UString name = "DoodadType";
	return name;
}
}; // namespace OpenApoc

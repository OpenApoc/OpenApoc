#include "game/state/rules/city/baselayout.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

template <> sp<BaseLayout> StateObject<BaseLayout>::get(const GameState &state, const UString &id)
{
	auto it = state.base_layouts.find(id);
	if (it == state.base_layouts.end())
	{
		LogError("No base layout type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<BaseLayout>::getPrefix()
{
	static UString prefix = "BASELAYOUT_";
	return prefix;
}
template <> const UString &StateObject<BaseLayout>::getTypeName()
{
	static UString name = "BaseLayout";
	return name;
}

} // namespace OpenApoc

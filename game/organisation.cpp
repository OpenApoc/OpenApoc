#include "game/organisation.h"
#include "game/gamestate.h"
#include "library/strings.h"

namespace OpenApoc
{

Organisation::Organisation(const UString &name, int balance, int income)
    : name(name), balance(balance), income(income)
{
}

bool Organisation::isHostileTo(const Organisation &other) const
{
	// FIXME: Everyone is hostile!
	return (this != &other);
}

template <>
sp<Organisation> StateObject<Organisation>::get(const GameState &state, const UString &id)
{
	auto it = state.organisations.find(id);
	if (it == state.organisations.end())
	{
		LogError("No organisation matching ID \"%s\"", id.c_str());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<Organisation>::getPrefix()
{
	static UString prefix = "ORG_";
	return prefix;
}
template <> const UString &StateObject<Organisation>::getTypeName()
{
	static UString name = "Organisation";
	return name;
}
}; // namespace OpenApoc

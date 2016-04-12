#include "game/state/organisation.h"
#include "game/state/gamestate.h"
#include "library/strings.h"

namespace OpenApoc
{

Organisation::Organisation(const UString &name, int balance, int income)
    : name(name), balance(balance), income(income)
{
}

bool Organisation::isHostileTo(StateRef<Organisation> other)
{
	if (other == this)
	{
		// Not hostile to yourself
		return false;
	}
	// FIXME: Make the hostile threshold read from serialized GameState?
	if (this->current_relations[other] <= -50)
	{
		return true;
	}
	return false;
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

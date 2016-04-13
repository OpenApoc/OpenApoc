#include "game/state/organisation.h"
#include "game/state/gamestate.h"
#include "library/strings.h"

namespace OpenApoc
{

Organisation::Organisation(const UString &name, int balance, int income)
    : name(name), balance(balance), income(income)
{
}

Organisation::Relation Organisation::isRelatedTo(StateRef<Organisation> other)
{
	if (other == this)
	{
		// Assume maximum relations
		return Relation::Allied;
	}

	float x = this->current_relations[other];
	// FIXME: Make the thresholds read from serialized GameState?
	if (x <= -50)
	{
		return Relation::Hostile;
	}
	else if (x <= -25)
	{
		return Relation::Unfriendly;
	}
	else if (x >= 25)
	{
		return Relation::Friendly;
	}
	else if (x >= 75)
	{
		return Relation::Allied;
	}
	return Relation::Neutral;
}

bool Organisation::isPositiveTo(StateRef<Organisation> other)
{
	if (other == this)
	{
		// No pessimism
		return true;
	}

	return this->current_relations[other] >= 0;
}

bool Organisation::isNegativeTo(StateRef<Organisation> other)
{
	if (other == this)
	{
		// No pessimism
		return false;
	}

	return this->current_relations[other] < 0;
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

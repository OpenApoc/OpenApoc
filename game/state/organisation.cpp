#include "game/state/organisation.h"
#include "game/state/gamestate.h"
#include "library/strings.h"

namespace OpenApoc
{

Organisation::Organisation() : balance(0), income(0), tech_level(1), average_guards(1) {}

float Organisation::getRelationTo(const StateRef<Organisation> &other) const
{
	if (other == this)
	{
		// Assume maximum relations
		return 100.0f;
	}
	if (this->name == "Alien" || other->name == "Alien")
	{
		// Everybody hates Aliens and vice-versa
		// FIXME: extract initital standings
		return -100.0f;
	}
	float x;

	auto it = this->current_relations.find(other);
	if (it == this->current_relations.end())
	{
		x = 0;
	}
	else
	{
		x = it->second;
	}
	return x;
}

Organisation::Relation Organisation::isRelatedTo(const StateRef<Organisation> &other) const
{
	float x = this->getRelationTo(other);
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

bool Organisation::isPositiveTo(const StateRef<Organisation> &other) const
{
	float x = this->getRelationTo(other);
	return x >= 0;
}

bool Organisation::isNegativeTo(const StateRef<Organisation> &other) const
{
	float x = this->getRelationTo(other);
	return x < 0;
}

template <>
sp<Organisation> StateObject<Organisation>::get(const GameState &state, const UString &id)
{
	auto it = state.organisations.find(id);
	if (it == state.organisations.end())
	{
		LogError("No organisation matching ID \"%s\"", id.cStr());
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

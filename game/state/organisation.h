#pragma once

#include "game/state/stateobject.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{

class Vehicle;
template <typename T> class StateObject;

class Organisation : public StateObject<Organisation>
{
  public:
	enum class Relation
	{
		Allied,
		Friendly,
		Neutral,
		Unfriendly,
		Hostile
	};
	UString name;
	int balance;
	int income;

	Organisation(const UString &name = "", int balance = 0, int income = 0);
	Relation isRelatedTo(StateRef<Organisation> other);
	bool isPositiveTo(StateRef<Organisation> other);
	bool isNegativeTo(StateRef<Organisation> other);
	std::map<StateRef<Organisation>, float> current_relations;
};

}; // namespace OpenApoc

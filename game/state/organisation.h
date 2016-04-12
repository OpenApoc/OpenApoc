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
	UString name;
	int balance;
	int income;

	Organisation(const UString &name = "", int balance = 0, int income = 0);
	bool isHostileTo(StateRef<Organisation> other);
	std::map<StateRef<Organisation>, float> current_relations;
};

}; // namespace OpenApoc

#pragma once

#include "game/state/stateobject.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{

class Vehicle;
class AEquipmentType;
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
	enum class LootPriority
	{
		A,
		B,
		C
	};
	UString name;
	int balance;
	int income;

	int tech_level;
	int average_guards;

	std::map<LootPriority, std::vector<StateRef<AEquipmentType>>> loot;

	Organisation();
	Relation isRelatedTo(const StateRef<Organisation> &other) const;
	bool isPositiveTo(const StateRef<Organisation> &other) const;
	bool isNegativeTo(const StateRef<Organisation> &other) const;
	float getRelationTo(const StateRef<Organisation> &other) const;
	std::map<StateRef<Organisation>, float> current_relations;
};

}; // namespace OpenApoc

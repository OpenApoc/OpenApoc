#pragma once

#include "game/state/stateobject.h"
#include "library/strings.h"
#include <list>
#include <map>
#include <vector>

namespace OpenApoc
{

class Vehicle;
class AgentType;
class AEquipmentType;
class GameState;

class Organisation : public StateObject
{
	STATE_OBJECT(Organisation)
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
	// What guard types can spawn, supports duplicates to provide variable probability
	std::list<StateRef<AgentType>> guard_types;

	std::map<LootPriority, std::vector<StateRef<AEquipmentType>>> loot;

	Organisation();
	int getGuardCount(GameState &state) const;
	Relation isRelatedTo(const StateRef<Organisation> &other) const;
	bool isPositiveTo(const StateRef<Organisation> &other) const;
	bool isNegativeTo(const StateRef<Organisation> &other) const;
	float getRelationTo(const StateRef<Organisation> &other) const;
	std::map<StateRef<Organisation>, float> current_relations;
};

}; // namespace OpenApoc

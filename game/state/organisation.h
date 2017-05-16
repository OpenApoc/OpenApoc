#pragma once

#include "game/state/gametime.h"
#include "game/state/stateobject.h"
#include "library/strings.h"
#include <list>
#include <map>
#include <vector>

namespace OpenApoc
{

// Original game checked one org every 5 minutes, which resulted in 125 minutes per check
// We are going to randomize accumulated ticks instead and check every 125 minutes,
// this would provide the same frequency (and thus the same takeover chance) regardless
// of how many orgs there are
static const unsigned TICKS_PER_TAKEOVER_ATTEMPT = TICKS_PER_MINUTE * 125;

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
	UString id;
	UString name;
	int balance = 0;
	int income = 0;
	int infiltrationValue = 0;
	// Modified for all infiltration attempts at this org
	int infiltrationSpeed = 0;
	bool takenOver = false;
	unsigned int ticksTakeOverAttemptAccumulated = 0;

	int tech_level = 1;
	int average_guards = 1;
	// What guard types can spawn, supports duplicates to provide variable probability
	std::list<StateRef<AgentType>> guard_types;

	std::map<LootPriority, std::vector<StateRef<AEquipmentType>>> loot;

	Organisation() = default;
	int getGuardCount(GameState &state) const;
	void takeOver(GameState &state, bool forced = false);
	void updateInfiltration(GameState &state);
	void update(GameState &state, unsigned int ticks);
	Relation isRelatedTo(const StateRef<Organisation> &other) const;
	bool isPositiveTo(const StateRef<Organisation> &other) const;
	bool isNegativeTo(const StateRef<Organisation> &other) const;
	float getRelationTo(const StateRef<Organisation> &other) const;
	std::map<StateRef<Organisation>, float> current_relations;
};

}; // namespace OpenApoc

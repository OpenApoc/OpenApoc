#pragma once

#include "game/state/gamestate.h"
#include "game/state/gametime.h"
#include "game/state/shared/organisation.h"
#include "library/strings.h"
#include "library/sp.h"

namespace OpenApoc
{

class PlayerStateSnapshot : public std::enable_shared_from_this<PlayerStateSnapshot>
{
  public:
	PlayerStateSnapshot(sp<GameState> stateToSnap);
	~PlayerStateSnapshot();

	bool fundingTerminated = false;
	const int fundingModifier = 0;

	GameTime gameTime;
	GameScore totalScore;
	GameScore weekScore;

	const int playerIncome = 0;
	int playerBalance = 0;

	const UString getPlayerBalance();

	const int governmentBalance = 0;
	Organisation::Relation government_player_relationship;

	std::vector<std::pair<int, int>> weekly_rating_rules;

	std::map<UString, int> playerBasesWeeklyCosts;
	std::map<AgentType::Role, int> agent_salary;
	std::map<AgentType::Role, int> agent_qty;
};

}; // namespace OpenApoc

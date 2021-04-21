#include "game/state/playerstatesnapshot.h"
#include "game/state/city/base.h"
#include "game/state/city/facility.h"
#include "game/state/shared/organisation.h"

namespace OpenApoc
{

PlayerStateSnapshot::PlayerStateSnapshot(sp<GameState> stateToSnap)
    : fundingModifier(stateToSnap->calculateFundingModifier()),
      fundingTerminated(stateToSnap->fundingTerminated), gameTime(stateToSnap->gameTime),
      totalScore(stateToSnap->totalScore), weekScore(stateToSnap->weekScore),
      weekly_rating_rules(stateToSnap->weekly_rating_rules),
      agent_salary(stateToSnap->agent_salary), playerBalance(stateToSnap->player->balance),
      playerIncome(stateToSnap->player->income),
      governmentBalance(stateToSnap->government->balance),
      government_player_relationship(stateToSnap->government->isRelatedTo(stateToSnap->player))
{
	int soldiers = 0, biochemists = 0, engineers = 0, physicists = 0;
	for (auto &a : stateToSnap->agents)
	{
		if (a.second->owner == stateToSnap->player)
		{
			switch (a.second->type->role)
			{
				case AgentType::Role::BioChemist:
					biochemists++;
					break;
				case AgentType::Role::Engineer:
					engineers++;
					break;
				case AgentType::Role::Physicist:
					physicists++;
					break;
				case AgentType::Role::Soldier:
					soldiers++;
					break;
			}
		}
		agent_qty.insert_or_assign(AgentType::Role::BioChemist, biochemists);
		agent_qty.insert_or_assign(AgentType::Role::Engineer, engineers);
		agent_qty.insert_or_assign(AgentType::Role::Physicist, physicists);
		agent_qty.insert_or_assign(AgentType::Role::Soldier, soldiers);
	}

	for (auto &base : stateToSnap->player_bases)
	{
		int basesCosts = 0;
		for (auto &f : base.second->facilities)
		{
			basesCosts += f->type->weeklyCost;
			playerBasesWeeklyCosts.insert_or_assign(base.first, basesCosts);
		}
	}
}

PlayerStateSnapshot::~PlayerStateSnapshot() {}

const UString PlayerStateSnapshot::getPlayerBalance()
{
	return Strings::fromInteger(playerBalance);
};
} // namespace OpenApoc

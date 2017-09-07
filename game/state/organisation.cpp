#include "game/state/organisation.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"
#include "library/strings.h"

namespace OpenApoc
{

// Returns 100% +- 25%, with a max of 20
int Organisation::getGuardCount(GameState &state) const
{
	return std::min(
	    20, randBoundsInclusive(state.rng, average_guards * 75 / 100, average_guards * 125 / 100));
}

void Organisation::takeOver(GameState &state, bool forced)
{
	if (!forced && randBoundsExclusive(state.rng, 0, 200) > infiltrationValue)
	{
		return;
	}
	takenOver = true;
	infiltrationValue = 200;
	StateRef<Organisation> org = {&state, id};
	current_relations[state.getPlayer()] = -100.0f;
	state.getPlayer()->current_relations[org] = -100.0f;
	current_relations[state.getAliens()] = 100.0f;
	state.getAliens()->current_relations[org] = 100.0f;
	for (auto &pair : state.organisations)
	{
		if (pair.second->id == id || !pair.second->takenOver)
		{
			continue;
		}
		current_relations[{&state, pair.first}] = 90.0f;
		pair.second->current_relations[org] = 90.0f;
	}
	// FIXME: Ensure above values do not change?
	// FIXME: Properly announce org taken over
	LogError("%s taken over by aliens!", name);
}

void Organisation::updateInfiltration(GameState &state)
{
	StateRef<Organisation> org = {&state, id};
	if (org == state.getPlayer() || org == state.getAliens())
	{
		return;
	}

	if (org->takenOver)
	{
		org->infiltrationValue = 200;
		return;
	}

	// FIXME: Properly read incursions value and difficulty
	int ufoIncursions = 1;
	int divizor = 42 - ufoIncursions;

	// Calculate infiltration modifier
	int infiltrationModifier = 0;
	for (auto &c : state.cities)
	{
		for (auto &b : c.second->buildings)
		{
			if (b.second->owner != org)
			{
				continue;
			}

			int infiltrationBuilding = 0;
			for (auto alien : b.second->current_crew)
			{
				infiltrationBuilding += alien.second * alien.first->infiltrationSpeed;
			}
			infiltrationBuilding *= b.second->function->infiltrationSpeed * org->infiltrationSpeed;
			infiltrationModifier += infiltrationBuilding;
		}
	}
	infiltrationModifier /= divizor;
	infiltrationModifier -= state.difficulty;
	if (state.gameTime.getHours() % 2)
	{
		infiltrationModifier--;
	}
	org->infiltrationValue = clamp(org->infiltrationValue - infiltrationModifier, 0, 200);
}

void Organisation::updateTakeOver(GameState &state, unsigned int ticks)
{
	ticksTakeOverAttemptAccumulated += ticks;
	while (ticksTakeOverAttemptAccumulated >= TICKS_PER_TAKEOVER_ATTEMPT)
	{
		ticksTakeOverAttemptAccumulated -= TICKS_PER_TAKEOVER_ATTEMPT;
		takeOver(state);
	}
}

float Organisation::getRelationTo(const StateRef<Organisation> &other) const
{
	if (other == this)
	{
		// Assume maximum relations
		return 100.0f;
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

sp<Organisation> Organisation::get(const GameState &state, const UString &id)
{
	auto it = state.organisations.find(id);
	if (it == state.organisations.end())
	{
		LogError("No organisation matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &Organisation::getPrefix()
{
	static UString prefix = "ORG_";
	return prefix;
}
const UString &Organisation::getTypeName()
{
	static UString name = "Organisation";
	return name;
}
}; // namespace OpenApoc

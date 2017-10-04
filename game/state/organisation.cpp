#include "game/state/organisation.h"
#include "framework/framework.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gameevent.h"
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
	if (!forced && randBoundsExclusive(state.rng, 0, 200) >= infiltrationValue)
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
	auto event = new GameOrganisationEvent(GameEventType::AlienTakeover, {&state, id});
	fw().pushEvent(event);
}

void Organisation::update(GameState &state, unsigned int ticks)
{
	for (auto &m : missions)
	{
		if (m.next < state.gameTime.getTicks())
		{
			m.execute(state, {&state, id});
		}
	}
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
	org->infiltrationValue = clamp(org->infiltrationValue + infiltrationModifier, 0, 200);
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

void Organisation::updateVehicleAgentPark(GameState &state)
{
	// Check that org owns a building
	bool found = false;
	for (auto &b : state.cities["CITYMAP_HUMAN"]->buildings)
	{
		if (b.second->owner.id != id)
		{
			continue;
		}
		found = true;
		break;
	}
	if (!found)
	{
		return;
	}

	if (agentPark > 0)
	{
		int countAgents = 0;
		for (auto &a : state.agents)
		{
			if (a.second->owner.id == id)
			{
				countAgents++;
			}
		}
		std::list<sp<Building>> buildingsRandomizer;
		for (auto &b : state.cities["CITYMAP_HUMAN"]->buildings)
		{
			if (b.second->owner.id != id)
			{
				continue;
			}
			buildingsRandomizer.push_back(b.second);
		}
		sp<Building> building = listRandomiser(state.rng, buildingsRandomizer);
		while (countAgents < agentPark)
		{
			auto agent = state.agent_generator.createAgent(state, {&state, id},
			                                               {&state, "AGENTTYPE_BUILDING_SECURITY"});
			agent->homeBuilding = {&state, building};
			agent->city = agent->homeBuilding->city;
			agent->enterBuilding(state, agent->homeBuilding);

			countAgents++;
		}
	}

	for (auto &entry : vehiclePark)
	{
		int countVehicles = 0;
		for (auto &v : state.vehicles)
		{
			if (v.second->owner.id == id && v.second->type == entry.first)
			{
				countVehicles++;
			}
		}
		while (countVehicles < entry.second)
		{
			// FIXME: Check if org has funds before buying vehicle

			std::list<sp<Building>> buildingsRandomizer;

			for (auto &b : state.cities["CITYMAP_HUMAN"]->buildings)
			{
				if (b.second->owner.id != id)
				{
					continue;
				}
				// Aim for at least 8 vehicles per building
				for (auto i = 0; i <= std::max(0, 8 - (int)b.second->currentVehicles.size()); i++)
				{
					buildingsRandomizer.push_back(b.second);
				}
			}

			sp<Building> building = listRandomiser(state.rng, buildingsRandomizer);

			auto v =
			    building->city->placeVehicle(state, entry.first, {&state, id}, {&state, building});
			v->homeBuilding = {&state, building};

			countVehicles++;
		}
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

void Organisation::adjustRelationTo(GameState &state, StateRef<Organisation> &other, float value)
{
	current_relations[other] = clamp(current_relations[other] + value, -100.0f, 100.0f);
	// Mirror player relations except in battle
	if (!state.current_battle && other == state.getPlayer())
	{
		other->current_relations[{&state, id}] = current_relations[other];
	}
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
Organisation::MissionPattern::MissionPattern(uint64_t minIntervalRepeat, uint64_t maxIntervalRepeat,
                                             unsigned minAmount, unsigned maxAmount,
                                             std::set<StateRef<VehicleType>> allowedTypes,
                                             Target target, std::set<Relation> relation)
    : minIntervalRepeat(minIntervalRepeat), maxIntervalRepeat(maxIntervalRepeat),
      minAmount(minAmount), maxAmount(maxAmount), allowedTypes(allowedTypes), target(target),
      relation(relation)
{
}

void Organisation::Mission::execute(GameState &state, StateRef<Organisation> owner)
{
	next = state.gameTime.getTicks() +
	       randBoundsInclusive(state.rng, pattern.minIntervalRepeat, pattern.maxIntervalRepeat);

	// Agent mission
	if (pattern.allowedTypes.empty())
	{
	}
	// Vehicle mission
	else
	{
		int count = randBoundsInclusive(state.rng, pattern.minAmount, pattern.maxAmount);
		// Special case
		if (pattern.target == Organisation::MissionPattern::Target::ArriveFromSpace)
		{
			LogWarning("Implement space liner arrival");
			return;
		}
		// Compile list of matching buildings with vehicles
		std::map<sp<Building>, std::list<sp<Vehicle>>> availableVehicles;
		for (auto &b : state.current_city->buildings)
		{
			if (b.second->owner != owner)
			{
				continue;
			}

			for (auto &v : b.second->currentVehicles)
			{
				if (v->owner == owner &&
				    pattern.allowedTypes.find(v->type) != pattern.allowedTypes.end())
				{
					availableVehicles[b.second].emplace_back(v);
				}
			}
		}
		if (availableVehicles.size() == 0)
		{
			// None available to take mission
			return;
		}
		std::list<sp<Building>> buildingsRandomizer;
		// Try pick one that has enough vehicles
		int maxSeenCount = 0;
		for (auto &e : availableVehicles)
		{
			if ((int)e.second.size() >= count)
			{
				buildingsRandomizer.emplace_back(e.first);
			}
			else if (maxSeenCount < (int)e.second.size())
			{
				maxSeenCount = (int)e.second.size();
			}
		}
		// Pick one with highest count
		if (buildingsRandomizer.size() == 0)
		{
			count = maxSeenCount;
			for (auto &e : availableVehicles)
			{
				if ((int)e.second.size() == maxSeenCount)
				{
					buildingsRandomizer.emplace_back(e.first);
				}
			}
		}
		auto sourceBuilding = listRandomiser(state.rng, buildingsRandomizer);

		// Special case
		if (pattern.target == Organisation::MissionPattern::Target::DepartToSpace)
		{
			LogWarning("Implement space liner departure");
			return;
		}

		// Pick destination building
		buildingsRandomizer.clear();
		for (auto &b : state.current_city->buildings)
		{
			if (b.second == sourceBuilding)
			{
				continue;
			}
			// Check if building fits
			switch (pattern.target)
			{
				case Organisation::MissionPattern::Target::Owned:
					if (b.second->owner == owner)
					{
						break;
					}
					continue;
				case Organisation::MissionPattern::Target::OwnedOrOther:
				case Organisation::MissionPattern::Target::Other:
					if (b.second->owner == owner)
					{
						if (pattern.target == Organisation::MissionPattern::Target::OwnedOrOther)
						{
							break;
						}
						else
						{
							continue;
						}
					}
					if (pattern.relation.empty())
					{
						break;
					}
					if (pattern.relation.find(owner->isRelatedTo(b.second->owner)) !=
					    pattern.relation.end())
					{
						break;
					}
					continue;
				case Organisation::MissionPattern::Target::ArriveFromSpace:
				case Organisation::MissionPattern::Target::DepartToSpace:
					LogError("Impossible to arrive/depart at this point?");
					return;
			}
			buildingsRandomizer.emplace_back(b.second);
		}
		if (buildingsRandomizer.size() == 0)
		{
			return;
		}

		auto destBuilding = listRandomiser(state.rng, buildingsRandomizer);

		// Do it
		while (count-- > 0)
		{
			auto v = availableVehicles[sourceBuilding].front();
			availableVehicles[sourceBuilding].pop_front();
			v->setMission(state,
			              VehicleMission::gotoBuilding(state, *v, {&state, destBuilding}, false));
			// Come back if we are going to another org
			if (destBuilding->owner != owner)
			{
				v->addMission(state, VehicleMission::snooze(state, *v, 10 * TICKS_PER_SECOND),
				              true);
				v->addMission(
				    state, VehicleMission::gotoBuilding(state, *v, {&state, sourceBuilding}), true);
			}
		}
	}
}

Organisation::Mission::Mission(uint64_t next, uint64_t minIntervalRepeat,
                               uint64_t maxIntervalRepeat, unsigned minAmount, unsigned maxAmount,
                               std::set<StateRef<VehicleType>> allowedTypes,
                               MissionPattern::Target target, std::set<Relation> relation)
    : next(next)
{
	pattern = {minIntervalRepeat, maxIntervalRepeat, minAmount, maxAmount, allowedTypes, target,
	           relation};
}
}; // namespace OpenApoc

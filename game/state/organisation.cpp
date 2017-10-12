#include "game/state/organisation.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "library/strings.h"

// Uncomment to turn off org missions
//#define DEBUG_TURN_OFF_ORG_MISSIONS

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

Organisation::PurchaseResult
Organisation::canPurchaseFrom(GameState &state, const StateRef<Building> &buyer, bool vehicle) const
{
	// Step 01: Check if org likes buyer
	if (isRelatedTo(buyer->owner) == Relation::Hostile)
	{
		return PurchaseResult::OrgHostile;
	}
	// Step 02: Check if org has buildings in buyer's city
	if (!getPurchaseBuilding(state, buyer))
	{
		return PurchaseResult::OrgHasNoBuildings;
	}
	// Step 03: Checks for cargo delivery
	if (!vehicle)
	{
		// Step 03.01: Find out who provides transportation services
		std::list<StateRef<Organisation>> ferryCompanies;
		for (auto &o : state.organisations)
		{
			if (o.second->providesTransportationServices)
			{
				ferryCompanies.emplace_back(&state, o.first);
			}
		}
		// Step 03.02: Check if a ferry provider exists that likes us both
		if (config().getBool("OpenApoc.NewFeature.FerryChecksRelationshipWhenBuying"))
		{
			// Clear those that don't like either
			for (auto it = ferryCompanies.begin(); it != ferryCompanies.end();)
			{
				if ((*it)->isRelatedTo({&state, id}) == Relation::Hostile ||
				    (*it)->isRelatedTo(buyer->owner) == Relation::Hostile)
				{
					it = ferryCompanies.erase(it);
				}
				else
				{
					it++;
				}
			}
			if (ferryCompanies.empty())
			{
				return PurchaseResult::NoTransportAvailable;
			}
		}
		// Step 03.03: Check if ferry provider has free ferries
		if (config().getBool("OpenApoc.NewFeature.CallExistingFerry"))
		{
			bool ferryFound = false;
			for (auto &o : ferryCompanies)
			{
				for (auto &v : state.vehicles)
				{
					if (v.second->owner != o || !v.second->type->provideFreightCargo ||
					    !v.second->missions.empty())
					{
						continue;
					}
					ferryFound = true;
					break;
				}
			}
			if (!ferryFound)
			{
				return PurchaseResult::NoTransportAvailable;
			}
		}
	}
	return PurchaseResult::OK;
}

StateRef<Building> Organisation::getPurchaseBuilding(GameState &state,
                                                     const StateRef<Building> &buyer) const
{
	std::list<StateRef<Building>> purchaseBuildings;
	for (auto &b : buyer->city->buildings)
	{
		if (b.second->owner.id == id && b.first != buyer.id)
		{
			purchaseBuildings.emplace_back(&state, b.first);
		}
	}
	if (purchaseBuildings.empty())
	{
		return nullptr;
	}
	else
	{
		// Prioritize building with cargo already bound to this buyer
		for (auto &b : purchaseBuildings)
		{
			for (auto &c : b->cargo)
			{
				if (c.destination == buyer)
				{
					return b;
				}
			}
		}
		return listRandomiser(state.rng, purchaseBuildings);
	}
}

void Organisation::purchase(GameState &state, const StateRef<Building> &buyer,
                            StateRef<VEquipmentType> vehicleEquipment, int count)
{
	// Expecting to be able to purchase
	auto building = getPurchaseBuilding(state, buyer);
	building->cargo.emplace_back(state, vehicleEquipment, count, StateRef<Organisation>{&state, id},
	                             buyer);
	LogWarning("PURCHASE: %s bought %dx%s at %s to %s ", buyer->owner.id, count,
	           vehicleEquipment.id, building.id, buyer.id);
	// FIXME: Economy
	auto owner = buyer->owner;
	owner->balance -= count * 1;
}

void Organisation::purchase(GameState &state, const StateRef<Building> &buyer,
                            StateRef<VAmmoType> vehicleAmmo, int count)
{
	// Expecting to be able to purchase
	auto building = getPurchaseBuilding(state, buyer);
	building->cargo.emplace_back(state, vehicleAmmo, count, StateRef<Organisation>{&state, id},
	                             buyer);
	LogWarning("PURCHASE: %s bought %dx%s at %s to %s ", buyer->owner.id, count, vehicleAmmo.id,
	           building.id, buyer.id);
	// FIXME: Economy
	auto owner = buyer->owner;
	owner->balance -= count * 1;
}

void Organisation::purchase(GameState &state, const StateRef<Building> &buyer,
                            StateRef<AEquipmentType> agentEquipment, int count)
{
	// Expecting to be able to purchase
	auto building = getPurchaseBuilding(state, buyer);
	building->cargo.emplace_back(state, agentEquipment, count, StateRef<Organisation>{&state, id},
	                             buyer);
	LogWarning("PURCHASE: %s bought %dx%s at %s to %s ", buyer->owner.id, count, agentEquipment.id,
	           building.id, buyer.id);
	// FIXME: Economy
	auto owner = buyer->owner;
	owner->balance -= count * 1;
}

void Organisation::purchase(GameState &state, const StateRef<Building> &buyer,
                            StateRef<VehicleType> vehicle, int count)
{
	// Expecting to be able to purchase
	auto building = getPurchaseBuilding(state, buyer);
	LogWarning("PURCHASE: %s bought %dx%s at %s to %s ", buyer->owner.id, count, vehicle.id,
	           building.id, buyer.id);
	auto v = building->city->placeVehicle(state, vehicle, buyer->owner, building);
	v->homeBuilding = buyer;
	v->setMission(state, VehicleMission::gotoBuilding(state, *v));
	// FIXME: Economy
	auto owner = buyer->owner;
	owner->balance -= count * 1;
}

void Organisation::updateMissions(GameState &state)
{
#ifdef DEBUG_TURN_OFF_ORG_MISSIONS
	return;
#endif
	if (state.getPlayer().id == id)
	{
		return;
	}
	for (auto &m : missions[state.current_city])
	{
		if (m.next < state.gameTime.getTicks())
		{
			m.execute(state, {&state, id});
		}
	}
	// Find rescue-capable craft
	StateRef<Vehicle> rescueTransport;
	for (auto &v : state.vehicles)
	{
		if (v.second->owner.id == id && v.second->missions.empty() &&
		    v.second->type->canRescueCrashed)
		{
			rescueTransport = {&state, v.first};
			break;
		}
	}
	// Attempt rescue someone
	if (rescueTransport)
	{
		// Rescue owned
		for (auto &v : state.vehicles)
		{
			if (v.second->city == rescueTransport->city && v.second->crashed &&
			    !v.second->carriedByVehicle && v.second->owner.id == id)
			{
				bool foundRescuer = false;
				for (auto &r : state.vehicles)
				{
					if (r.second->city == rescueTransport->city && r.second->type->canRescueCrashed)
					{
						for (auto &m : r.second->missions)
						{
							if (m->type == VehicleMission::MissionType::RecoverVehicle &&
							    m->targetVehicle.id == v.first)
							{
								foundRescuer = true;
								break;
							}
						}
					}
				}
				if (!foundRescuer)
				{
					rescueTransport->setMission(
					    state,
					    VehicleMission::recoverVehicle(state, *rescueTransport, {&state, v.first}));
					rescueTransport->addMission(
					    state, VehicleMission::gotoBuilding(state, *rescueTransport), true);
					break;
				}
			}
		}
		// Rescue allies
		for (auto &v : state.vehicles)
		{
			if (v.second->city == rescueTransport->city && v.second->crashed &&
			    !v.second->carriedByVehicle && v.second->owner.id != id &&
			    isRelatedTo(v.second->owner) == Relation::Allied)
			{
				bool foundRescuer = false;
				for (auto &r : state.vehicles)
				{
					if (r.second->city == rescueTransport->city && r.second->type->canRescueCrashed)
					{
						for (auto &m : r.second->missions)
						{
							if (m->type == VehicleMission::MissionType::RecoverVehicle &&
							    m->targetVehicle.id == v.first)
							{
								foundRescuer = true;
								break;
							}
						}
					}
				}
				if (!foundRescuer)
				{
					rescueTransport->setMission(
					    state,
					    VehicleMission::recoverVehicle(state, *rescueTransport, {&state, v.first}));
					rescueTransport->addMission(
					    state, VehicleMission::gotoBuilding(state, *rescueTransport), true);
					break;
				}
			}
		}
	}
}

void Organisation::updateHirableAgents(GameState &state)
{
	if (hirableTypes.empty())
	{
		return;
	}
	StateRef<Building> hireeLocation;
	for (auto &c : state.cities)
	{
		for (auto &b : c.second->buildings)
		{
			if (b.second->owner.id != id)
			{
				continue;
			}
			hireeLocation = {&state, b.first};
			break;
		}
	}
	if (!hireeLocation)
	{
		return;
	}
	std::set<sp<Agent>> agentsToRemove;
	for (auto &a : state.agents)
	{
		if (a.second->owner.id == id && hirableTypes.find(a.second->type) != hirableTypes.end())
		{
			agentsToRemove.insert(a.second);
		}
	}
	for (auto &a : agentsToRemove)
	{
		a->die(state, true);
	}
	int newAgents = randBoundsInclusive(state.rng, minHireePool, maxHireePool);
	for (int i = 0; i < newAgents; i++)
	{
		auto a = state.agent_generator.createAgent(state, {&state, id},
		                                           setRandomiser(state.rng, hirableTypes));
		a->homeBuilding = hireeLocation;
		a->city = hireeLocation->city;
		a->enterBuilding(state, hireeLocation);
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

	// if (agentPark > 0)
	//{
	//	int countAgents = 0;
	//	for (auto &a : state.agents)
	//	{
	//		if (a.second->owner.id == id)
	//		{
	//			countAgents++;
	//		}
	//	}
	//	std::list<sp<Building>> buildingsRandomizer;
	//	for (auto &b : state.cities["CITYMAP_HUMAN"]->buildings)
	//	{
	//		if (b.second->owner.id != id)
	//		{
	//			continue;
	//		}
	//		buildingsRandomizer.push_back(b.second);
	//	}
	//	sp<Building> building = listRandomiser(state.rng, buildingsRandomizer);
	//	while (countAgents < agentPark)
	//	{
	//		auto agent = state.agent_generator.createAgent(state, {&state, id},
	//		                                               {&state, "AGENTTYPE_BUILDING_SECURITY"});
	//		agent->homeBuilding = {&state, building};
	//		agent->city = agent->homeBuilding->city;
	//		agent->enterBuilding(state, agent->homeBuilding);

	//		countAgents++;
	//	}
	//}

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

void Organisation::adjustRelationTo(GameState &state, StateRef<Organisation> other, float value)
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

	int count = randBoundsInclusive(state.rng, pattern.minAmount, pattern.maxAmount);
	// Special case
	if (pattern.target == Organisation::MissionPattern::Target::ArriveFromSpace)
	{
		LogWarning("Implement space liner arrival");
		return;
	}
	// Compile list of matching buildings with vehicles
	std::map<sp<Building>, std::list<StateRef<Vehicle>>> availableVehicles;
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
				availableVehicles[b.second].push_back(v);
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
			buildingsRandomizer.push_back(e.first);
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
				buildingsRandomizer.push_back(e.first);
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
		buildingsRandomizer.push_back(b.second);
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
			v->addMission(state, VehicleMission::snooze(state, *v, 10 * TICKS_PER_SECOND), true);
			v->addMission(state, VehicleMission::gotoBuilding(state, *v, {&state, sourceBuilding}),
			              true);
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

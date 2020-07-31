#include "game/state/shared/organisation.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/baselayout.h"
#include "game/state/rules/city/scenerytiletype.h"
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
				return PurchaseResult::TranportHostile;
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
	for (auto &b : buildings)
	{
		if (b->city == buyer->city && b != buyer && b->isAlive())
		{
			purchaseBuildings.push_back(b);
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
		return pickRandom(state.rng, purchaseBuildings);
	}
}

void Organisation::purchase(GameState &state, const StateRef<Building> &buyer,
                            StateRef<VEquipmentType> vehicleEquipment, int count)
{
	int price = 0;
	if (state.economy.find(vehicleEquipment.id) == state.economy.end())
	{
		LogError("Economy not found for %s: How are we buying it then!?", vehicleEquipment.id);
	}
	else
	{
		auto &economy = state.economy[vehicleEquipment.id];
		price = economy.currentPrice;
		if (buyer->owner == state.getPlayer())
		{
			economy.currentStock -= count;
			if (economy.currentStock < 0)
			{
				LogInfo("Economy went into negative stock for %s: Was it because we used economy "
				        "to transfer?",
				        vehicleEquipment.id);
			}
		}
	}

	// Expecting to be able to purchase
	auto building = buyer->owner->id == id ? buyer : getPurchaseBuilding(state, buyer);
	building->cargo.emplace_back(state, vehicleEquipment, count, price,
	                             StateRef<Organisation>{&state, id}, buyer);
	LogWarning("PURCHASE: %s bought %dx%s at %s to %s ", buyer->owner.id, count,
	           vehicleEquipment.id, building.id, buyer.id);
	auto owner = buyer->owner;
	owner->balance -= count * price;
}

void Organisation::purchase(GameState &state, const StateRef<Building> &buyer,
                            StateRef<VAmmoType> vehicleAmmo, int count)
{
	int price = 0;
	if (state.economy.find(vehicleAmmo.id) == state.economy.end())
	{
		LogError("Economy not found for %s: How are we buying it then!?", vehicleAmmo.id);
	}
	else
	{
		auto &economy = state.economy[vehicleAmmo.id];
		price = economy.currentPrice;
		if (buyer->owner == state.getPlayer())
		{
			economy.currentStock -= count;
			if (economy.currentStock < 0)
			{
				LogInfo("Economy went into negative stock for %s: Was it because we used economy "
				        "to transfer?",
				        vehicleAmmo.id);
			}
		}
	}

	// Expecting to be able to purchase
	auto building = buyer->owner->id == id ? buyer : getPurchaseBuilding(state, buyer);
	building->cargo.emplace_back(state, vehicleAmmo, count, price,
	                             StateRef<Organisation>{&state, id}, buyer);
	LogWarning("PURCHASE: %s bought %dx%s at %s to %s ", buyer->owner.id, count, vehicleAmmo.id,
	           building.id, buyer.id);
	auto owner = buyer->owner;
	owner->balance -= count * price;
}

void Organisation::purchase(GameState &state, const StateRef<Building> &buyer,
                            StateRef<AEquipmentType> agentEquipment, int count)
{
	int price = 0;
	if (state.economy.find(agentEquipment.id) == state.economy.end())
	{
		LogError("Economy not found for %s: How are we buying it then!?", agentEquipment.id);
	}
	else
	{
		auto &economy = state.economy[agentEquipment.id];
		price = economy.currentPrice;
		if (buyer->owner == state.getPlayer())
		{
			economy.currentStock -= count;
			if (economy.currentStock < 0)
			{
				LogInfo("Economy went into negative stock for %s: Was it because we used economy "
				        "to transfer?",
				        agentEquipment.id);
			}
		}
	}

	// Expecting to be able to purchase
	auto building = buyer->owner->id == id ? buyer : getPurchaseBuilding(state, buyer);
	building->cargo.emplace_back(
	    state, agentEquipment,
	    count * (agentEquipment->type == AEquipmentType::Type::Ammo ? agentEquipment->max_ammo : 1),
	    price, StateRef<Organisation>{&state, id}, buyer);
	LogWarning("PURCHASE: %s bought %dx%s at %s to %s ", buyer->owner.id, count, agentEquipment.id,
	           building.id, buyer.id);
	auto owner = buyer->owner;
	owner->balance -= count * price;
}

void Organisation::purchase(GameState &state, const StateRef<Building> &buyer,
                            StateRef<VehicleType> vehicleType, int count)
{
	int price = 0;
	if (state.economy.find(vehicleType.id) == state.economy.end())
	{
		LogError("Economy not found for %s: How are we buying it then!?", vehicleType.id);
	}
	else
	{
		auto &economy = state.economy[vehicleType.id];
		price = economy.currentPrice;
		if (buyer->owner == state.getPlayer())
		{
			economy.currentStock -= count;
			if (economy.currentStock < 0)
			{
				LogError("Economy went into negative stock for %s: How the hell?", vehicleType.id);
			}
		}
	}
	// Expecting to be able to purchase
	auto building = buyer->owner->id == id ? buyer : getPurchaseBuilding(state, buyer);
	for (int i = 0; i < count; i++)
	{
		auto v = building->city->placeVehicle(state, vehicleType, buyer->owner, building);
		v->homeBuilding = buyer;
		v->setMission(state, VehicleMission::gotoBuilding(state, *v));
	}
	LogWarning("PURCHASE: %s bought %dx%s at %s to %s ", buyer->owner.id, count, vehicleType.id,
	           building.id, buyer.id);
	auto owner = buyer->owner;
	owner->balance -= count * price;
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
			m.execute(state, state.current_city, {&state, id});
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
			if (v.second->city == rescueTransport->city && v.second->owner.id == id &&
			    VehicleMission::canRecoverVehicle(state, *rescueTransport, *v.second))
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
		// Rescue allies but not aliens
		for (auto &v : state.vehicles)
		{
			if (v.second->city == rescueTransport->city && v.second->owner != state.getAliens() &&
			    v.second->owner.id != id && isRelatedTo(v.second->owner) == Relation::Allied &&
			    VehicleMission::canRecoverVehicle(state, *rescueTransport, *v.second))
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
	if (hirableAgentTypes.empty())
	{
		return;
	}
	StateRef<Building> hireeLocation;
	if (state.getCivilian().id == id)
	{
		std::vector<StateRef<Building>> buildingsWithoutBases;
		for (auto &b : state.cities["CITYMAP_HUMAN"]->buildings)
		{
			if (!b.second->base_layout)
				buildingsWithoutBases.emplace_back(&state, b.second);
		}
		if (buildingsWithoutBases.empty())
		{
			LogError("Cannot spawn new hirable agent - No building without base?");
		}
		hireeLocation = pickRandom(state.rng, buildingsWithoutBases);
	}
	else
	{
		if (buildings.empty())
		{
			return;
		}
		hireeLocation = pickRandom(state.rng, buildings);
	}
	std::set<sp<Agent>> agentsToRemove;
	for (auto &a : state.agents)
	{
		if (a.second->owner.id == id &&
		    hirableAgentTypes.find(a.second->type) != hirableAgentTypes.end())
		{
			if (randBoundsExclusive(state.rng, 0, 100) < CHANGE_HIREE_GONE)
			{
				agentsToRemove.insert(a.second);
			}
		}
	}
	for (auto &a : agentsToRemove)
	{
		a->die(state, true);
	}
	for (auto &entry : hirableAgentTypes)
	{
		int newAgents = randBoundsInclusive(state.rng, entry.second.first, entry.second.second);
		for (int i = 0; i < newAgents; i++)
		{
			auto a = state.agent_generator.createAgent(state, {&state, id}, entry.first);
			// Strip them of default equipment
			while (!a->equipment.empty())
			{
				a->removeEquipment(state, a->equipment.front());
			}
			a->homeBuilding = hireeLocation;
			a->city = hireeLocation->city;
			a->enterBuilding(state, hireeLocation);
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
	for (auto &b : buildings)
	{
		int infiltrationBuilding = 0;
		for (auto alien : b->current_crew)
		{
			infiltrationBuilding += alien.second * alien.first->infiltrationSpeed;
		}
		infiltrationBuilding *= b->function->infiltrationSpeed * org->infiltrationSpeed;
		infiltrationModifier += infiltrationBuilding;
	}
	infiltrationModifier /= divizor;
	infiltrationModifier -= state.difficulty;
	if (state.gameTime.getHours() % 2)
	{
		infiltrationModifier--;
	}
	org->infiltrationValue = clamp(org->infiltrationValue + infiltrationModifier, 0, 200);
}

void Organisation::updateDailyInfiltrationHistory()
{
	infiltrationHistory.push_front(this->infiltrationValue);
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
	for (auto &b : buildings)
	{
		if (b->city.id != "CITYMAP_HUMAN")
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
	//	sp<Building> building = pickRandom(state.rng, buildingsRandomizer);
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
		bool spaceLiner = false;
		for (auto &m : missions[{&state, "CITYMAP_HUMAN"}])
		{
			if (m.pattern.target == MissionPattern::Target::ArriveFromSpace ||
			    m.pattern.target == MissionPattern::Target::DepartToSpace)
			{
				if (m.pattern.allowedTypes.find(entry.first) != m.pattern.allowedTypes.end())
				{
					spaceLiner = true;
					break;
				}
			}
		}
		while (countVehicles < entry.second)
		{
			// FIXME: Check if org has funds before buying vehicle

			std::list<StateRef<Building>> buildingsRandomizer;

			if (spaceLiner)
			{
				buildingsRandomizer = state.cities["CITYMAP_HUMAN"]->spaceports;
			}
			else
			{
				for (auto &b : buildings)
				{
					if (b->city.id != "CITYMAP_HUMAN")
					{
						continue;
					}
					// Aim for at least 8 vehicles per building
					for (auto i = 0; i <= std::max(0, 8 - (int)b->currentVehicles.size()); i++)
					{
						buildingsRandomizer.emplace_back(b);
					}
				}
			}

			StateRef<Building> building = pickRandom(state.rng, buildingsRandomizer);

			auto v = building->city->placeVehicle(state, entry.first, {&state, id}, building);
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
	if (x < -50)
	{
		return Relation::Hostile;
	}
	else if (x < -25)
	{
		return Relation::Unfriendly;
	}
	else if (x < 25)
	{
		return Relation::Neutral;
	}
	else if (x < 75)
	{
		return Relation::Friendly;
	}
	else
	{
		return Relation::Allied;
	}
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

/**
 * Calculate the cost of a bribe
 * @param other - other organisation
 * @return - minimum sum of the bribe
 */
int Organisation::costOfBribeBy(const StateRef<Organisation> &other) const
{
	float improvement;
	float x = this->getRelationTo(other);
	if (x < -50) // Hostile
	{
		improvement = -50.0f - x;
	}
	else if (x < -25) // Unfriendly
	{
		improvement = -25.0f - x;
	}
	else if (x < 25) // Neutral
	{
		improvement = 25.0f - x;
	}
	else if (x < 75) // Friendly
	{
		improvement = 75.0f - x;
	}
	else // Allied (relationship cannot be improved)
	{
		return 0;
	}

	// The best approximation is 2030 * improvement + 19573
	// but vanilla X-Com:
	// 1. fond of numbers with 7 (27000, 37000 etc up to 127000)
	// 2. often, for unknown reason, reduces the sum
	// TODO: implement a more relevant formula
	return 2000 * std::max((int)improvement, 1) + 25000;
}

/**
 * The organisation is bribed by other org
 * @param state - GameState
 * @param other - other organisation
 * @param bribe - sum of the bribe
 * @return - true/false if success/fail
 */
bool Organisation::bribedBy(GameState &state, StateRef<Organisation> other, int bribe)
{
	if (bribe <= 0 || other->balance < bribe || bribe < costOfBribeBy(other))
	{
		return false;
	}

	float improvement;
	float x = this->getRelationTo(other);
	if (x < -50) // Hostile
	{
		improvement = -50.0f - x;
	}
	else if (x < -25) // Unfriendly
	{
		improvement = -25.0f - x;
	}
	else if (x < 25) // Neutral
	{
		improvement = 25.0f - x;
	}
	else if (x < 75) // Friendly
	{
		improvement = 75.0f - x;
	}
	else // Allied (relationship cannot be improved)
	{
		return false;
	}

	other->balance -= bribe;
	adjustRelationTo(state, other, improvement);
	return true;
}

template <>
sp<Organisation> StateObject<Organisation>::get(const GameState &state, const UString &id)
{
	auto it = state.organisations.find(id);
	if (it == state.organisations.end())
	{
		LogError("No organisation matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<Organisation>::getPrefix()
{
	static UString prefix = "ORG_";
	return prefix;
}
template <> const UString &StateObject<Organisation>::getTypeName()
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

void Organisation::Mission::execute(GameState &state, StateRef<City> city,
                                    StateRef<Organisation> owner)
{
	next = state.gameTime.getTicks() +
	       randBoundsInclusive(state.rng, pattern.minIntervalRepeat, pattern.maxIntervalRepeat);

	int count = randBoundsInclusive(state.rng, pattern.minAmount, pattern.maxAmount);
	// Special case
	if (pattern.target == Organisation::MissionPattern::Target::ArriveFromSpace)
	{
		if (city->spaceports.empty())
		{
			LogError("No spaceports in city!?");
			return;
		}
		// Make list of functional spaceports
		std::list<StateRef<Building>> spaceports;
		for (auto &b : city->spaceports)
		{
			if (b->isAlive())
			{
				bool intactPad = false;
				for (auto &p : b->landingPadLocations)
				{
					auto scenery = city->map->getTile(p)->presentScenery;
					if (scenery && scenery->type->isLandingPad)
					{
						intactPad = true;
						break;
					}
				}
				if (intactPad)
				{
					spaceports.push_back(b);
				}
			}
		}
		if (spaceports.empty())
		{
			return;
		}
		auto linerType = pickRandom(state.rng, pattern.allowedTypes);
		auto liner = city->placeVehicle(state, linerType, owner,
		                                VehicleMission::getRandomMapEdgeCoordinates(state, city));

		auto building = pickRandom(state.rng, spaceports);
		liner->homeBuilding = building;
		liner->setMission(state, VehicleMission::gotoBuilding(state, *liner));
		return;
	}
	// Compile list of matching buildings with vehicles
	std::map<sp<Building>, std::list<StateRef<Vehicle>>> availableVehicles;
	for (auto &b : owner->buildings)
	{
		if (b->city != city)
		{
			continue;
		}

		for (auto &v : b->currentVehicles)
		{
			if (v->owner == owner &&
			    pattern.allowedTypes.find(v->type) != pattern.allowedTypes.end())
			{
				availableVehicles[b].push_back(v);
			}
		}
	}
	if (availableVehicles.empty())
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
	if (buildingsRandomizer.empty())
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
	auto sourceBuilding = pickRandom(state.rng, buildingsRandomizer);

	// Special case
	if (pattern.target == Organisation::MissionPattern::Target::DepartToSpace)
	{
		auto v = availableVehicles[sourceBuilding].front();
		availableVehicles[sourceBuilding].pop_front();
		v->setMission(state, VehicleMission::departToSpace(state, *v));
		return;
	}

	// Pick destination building
	buildingsRandomizer.clear();
	for (auto &b : city->buildings)
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
	if (buildingsRandomizer.empty())
	{
		return;
	}

	auto destBuilding = pickRandom(state.rng, buildingsRandomizer);

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

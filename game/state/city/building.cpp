#include "game/state/city/building.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "game/state/city/agentmission.h"
#include "game/state/city/base.h"
#include "game/state/city/city.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/shared/organisation.h"

// Uncomment to make cargo system output warnings
#define DEBUG_VERBOSE_CARGO_SYSTEM

namespace OpenApoc
{

template <>
sp<BuildingFunction> StateObject<BuildingFunction>::get(const GameState &state, const UString &id)
{
	auto it = state.building_functions.find(id);
	if (it == state.building_functions.end())
	{
		LogError("No building_function matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<BuildingFunction>::getPrefix()
{
	static UString prefix = "BUILDINGFUNCTION_";
	return prefix;
}
template <> const UString &StateObject<BuildingFunction>::getTypeName()
{
	static UString name = "AgentType";
	return name;
}

template <> sp<Building> StateObject<Building>::get(const GameState &state, const UString &id)
{
	for (auto &city : state.cities)
	{
		auto it = city.second->buildings.find(id);
		if (it != city.second->buildings.end())
			return it->second;
	}

	LogError("No building type matching ID \"%s\"", id);
	return nullptr;
}

template <> const UString &StateObject<Building>::getPrefix()
{
	static UString prefix = "BUILDING_";
	return prefix;
}
template <> const UString &StateObject<Building>::getTypeName()
{
	static UString name = "Building";
	return name;
}

template <>
const UString &StateObject<Building>::getId(const GameState &state, const sp<Building> ptr)
{
	static const UString emptyString = "";
	for (auto &c : state.cities)
	{
		for (auto &b : c.second->buildings)
		{
			if (b.second == ptr)
				return b.first;
		}
	}
	LogError("No building matching pointer %p", ptr.get());
	return emptyString;
}

bool Building::hasAliens() const
{
	if (!preset_crew.empty())
	{
		return true;
	}
	for (auto &pair : current_crew)
	{
		if (pair.second > 0)
		{
			return true;
		}
	}
	return false;
}

void Building::updateDetection(GameState &state, unsigned int ticks)
{
	if (ticksDetectionTimeOut > 0)
	{
		if (ticksDetectionTimeOut < ticks)
		{
			ticksDetectionTimeOut = 0;
		}
		else
		{
			ticksDetectionTimeOut -= ticks;
		}
		return;
	}
	detected = false;
	ticksDetectionAttemptAccumulated += ticks;
	while (ticksDetectionAttemptAccumulated >= TICKS_PER_DETECTION_ATTEMPT[state.difficulty])
	{
		ticksDetectionAttemptAccumulated -= TICKS_PER_DETECTION_ATTEMPT[state.difficulty];
		detect(state, state.firstDetection || owner == state.getPlayer());
	}
}
void Building::updateCargo(GameState &state)
{
	StateRef<Building> thisRef = {&state, getId(state, shared_from_this())};

	// Step 01: Consume cargo with destination = this or zero count or hostile destination
	for (auto it = cargo.begin(); it != cargo.end();)
	{
		if (it->count != 0 && it->destination == thisRef)
		{
			it->arrive(state);
		}
		if (it->count == 0)
		{
			it = cargo.erase(it);
		}
		else if (owner->isRelatedTo(it->destination->owner) == Organisation::Relation::Hostile)
		{
			it->seize(state, owner);
			it = cargo.erase(it);
		}
		else
		{
			it++;
		}
	}

	// Step 02: Check expiry dates and expire cargo
	for (auto &c : cargo)
	{
		if (c.checkExpiryDate(state, thisRef) && c.destination->owner == state.getPlayer())
		{
			fw().pushEvent(new GameBuildingEvent(GameEventType::CargoExpiresSoon, thisRef));
		}
	}

	// Step 03.01: If we spawn ferries just spawn them at this point
	if (!config().getBool("OpenApoc.NewFeature.CallExistingFerry"))
	{
		std::list<StateRef<Organisation>> ferryCompanies;
		for (auto &o : state.organisations)
		{
			if (!o.second->providesTransportationServices)
			{
				continue;
			}
			ferryCompanies.emplace_back(&state, o.first);
		}
		bool spawnedFerry = false;
		do
		{
			// See if cargo or passengers exist
			bool needCallFerry = false;
			spawnedFerry = false;
			if (!cargo.empty())
			{
				needCallFerry = true;
			}
			else
				for (auto &a : currentAgents)
				{
					if (a->missions.empty() ||
					    a->missions.front()->type != AgentMission::MissionType::AwaitPickup)
					{
						continue;
					}
					needCallFerry = true;
					break;
				}
			if (!needCallFerry)
			{
				break;
			}
			// Passengers or cargo do exist, call one ferry
			// Cargo first
			for (auto &c : cargo)
			{
				// Find org that will ferry for us
				bool checkRelationship =
				    config().getBool("OpenApoc.NewFeature.FerryChecksRelationshipWhenBuying") ||
				    c.cost == 0;
				StateRef<Organisation> ferryCompany;
				for (auto &o : ferryCompanies)
				{
					if (checkRelationship)
					{
						if (o->isRelatedTo(c.destination->owner) == Organisation::Relation::Hostile)
						{
							continue;
						}
						if (c.originalOwner &&
						    o->isRelatedTo(c.originalOwner) == Organisation::Relation::Hostile)
						{
							continue;
						}
					}
					ferryCompany = o;
					break;
				}
				// Noone wants to ferry for us
				if (!ferryCompany)
				{
					continue;
				}
				// Find a vehicle type that will work for this type of cargo
				bool needBio = c.type == Cargo::Type::Bio;
				std::list<StateRef<VehicleType>> ferries;
				for (auto &t : state.vehicle_types)
				{
					if ((t.second->provideFreightBio && needBio) ||
					    (t.second->provideFreightCargo && !needBio))
					{
						ferries.emplace_back(&state, t.first);
					}
				}
				if (ferries.empty())
				{
					LogError("There is no ferry type for cargo with bio = %s in the game!?",
					         needBio);
					return;
				}
				// Spawn a random vehicle type and provide service
				auto v = city->placeVehicle(state, pickRandom(state.rng, ferries), ferryCompany);
				v->enterBuilding(state, thisRef);
				v->provideService(state, true);
				spawnedFerry = true;
#ifdef DEBUG_VERBOSE_CARGO_SYSTEM
				LogWarning("Spawned cargo ferry %s owned by %s at %s", v->type.id, ferryCompany.id,
				           thisRef.id);
#endif
				break;
			}
			// Ferry for agents
			for (auto &a : currentAgents)
			{
				if (a->missions.empty() ||
				    a->missions.front()->type != AgentMission::MissionType::AwaitPickup)
				{
					continue;
				}
				// Find org that will ferry for us
				StateRef<Organisation> ferryCompany;
				for (auto &o : ferryCompanies)
				{
					if (o->isRelatedTo(a->owner) == Organisation::Relation::Hostile)
					{
						continue;
					}
					ferryCompany = o;
					break;
				}
				// Noone wants to ferry for us
				if (!ferryCompany)
				{
					continue;
				}
				// Find a vehicle type that will work for agents
				std::list<StateRef<VehicleType>> ferries;
				for (auto &t : state.vehicle_types)
				{
					if (t.second->provideFreightAgent)
					{
						ferries.emplace_back(&state, t.first);
					}
				}
				if (ferries.empty())
				{
					LogError("There is no ferry type for agents in the game!?");
					return;
				}
				// Spawn a random vehicle type and provide service
				auto v = city->placeVehicle(state, pickRandom(state.rng, ferries), ferryCompany);
				v->enterBuilding(state, thisRef);
				v->provideService(state, true);
				spawnedFerry = true;
#ifdef DEBUG_VERBOSE_CARGO_SYSTEM
				LogWarning("Spawned passenger ferry %s owned by %s at %s", v->type.id,
				           ferryCompany.id, thisRef.id);
#endif
				break;
			}
			// Finally clear empty cargo
			for (auto it = cargo.begin(); it != cargo.end();)
			{
				if (it->count == 0)
				{
					it = cargo.erase(it);
				}
				else
				{
					it++;
				}
			}
		} while (spawnedFerry);
		return;
	}

	// Step 03.02: Compile list of carrying capacity required
	std::map<StateRef<Building>, std::map<StateRef<Organisation>, std::vector<int>>> spaceNeeded;
	std::set<StateRef<Building>> inboundFerries;
	std::set<StateRef<Building>> containsPurchase;
	for (auto &c : cargo)
	{
		if (c.count == 0)
		{
			LogError("Should not be possible to have zero cargo at this point?");
			continue;
		}
		auto sourceOrg = (!c.originalOwner || c.destination->owner == c.originalOwner)
		                     ? nullptr
		                     : c.originalOwner;
		spaceNeeded[c.destination][sourceOrg].resize(3);
		if (c.type == Cargo::Type::Bio)
		{
#ifdef DEBUG_VERBOSE_CARGO_SYSTEM
			LogWarning("BIOCARGO: %s needs to deliver %d to %s", thisRef.id,
			           c.count * c.space / c.divisor, c.destination.id);
#endif
			spaceNeeded[c.destination][sourceOrg][0] += std::max(1, c.count * c.space / c.divisor);
		}
		else
		{
#ifdef DEBUG_VERBOSE_CARGO_SYSTEM
			LogWarning("CARGO: %s needs to deliver %d to %s", thisRef.id,
			           c.count * c.space / c.divisor, c.destination.id);
#endif
			spaceNeeded[c.destination][sourceOrg][1] += std::max(1, c.count * c.space / c.divisor);
		}
		if (c.cost > 0)
		{
			containsPurchase.insert(c.destination);
		}
	}
	for (auto &a : currentAgents)
	{
		if (a->missions.empty() ||
		    a->missions.front()->type != AgentMission::MissionType::AwaitPickup)
		{
			continue;
		}
#ifdef DEBUG_VERBOSE_CARGO_SYSTEM
		LogWarning("AGENT: %s needs to deliver to %s", thisRef.id,
		           a->missions.front()->targetBuilding.id);
#endif
		spaceNeeded[a->missions.front()->targetBuilding][a->owner].resize(3);
		spaceNeeded[a->missions.front()->targetBuilding][a->owner][2]++;
	}

	// Step 04: Find if carrying capacity is satisfied by incoming ferries
	if (!spaceNeeded.empty())
	{
		for (auto &v : state.vehicles)
		{
			// Check if in this city
			if (v.second->city != city)
			{
				continue;
			}
			// Check if is a ferry
			if (v.second->missions.empty() ||
			    v.second->missions.back()->type != VehicleMission::MissionType::OfferService ||
			    v.second->missions.back()->missionCounter > 0)
			{
				continue;
			}
			// Check which target building can reserve this transport
			for (auto &bld : spaceNeeded)
			{
				// Already satisfied
				if (bld.second.empty())
				{
					continue;
				}
				// Not bound for this building
				if (v.second->missions.back()->targetBuilding != thisRef)
				{
					continue;
				}
				// Check if agrees to ferry for us:
				// - if this is not a purchase
				// - or if we're set to check purchases
				bool checkRelationship =
				    config().getBool("OpenApoc.NewFeature.FerryChecksRelationshipWhenBuying") ||
				    containsPurchase.find(bld.first) == containsPurchase.end();
				bool checkBuildingRelationship = false;
				for (auto &e : bld.second)
				{
					if (e.second[0] != 0 || e.second[1] != 0)
					{
						checkBuildingRelationship = true;
					}
				}
				if (checkRelationship && checkBuildingRelationship &&
				    v.second->owner->isRelatedTo(bld.first->owner) ==
				        Organisation::Relation::Hostile)
				{
					continue;
				}
				// Check for every cargo sending org
				bool reserved = false;
				for (auto &e : bld.second)
				{
					// Check it agrees to ferry for this org
					if (checkRelationship)
					{
						if (e.first && v.second->owner->isRelatedTo(e.first) ==
						                   Organisation::Relation::Hostile)
						{
							continue;
						}
					}
					// Check what exactly can it ferry
					if (v.second->type->provideFreightAgent && e.second[2] > 0)
					{
						reserved = true;
						int maxAmount = std::min(
						    v.second->getMaxPassengers() - v.second->getPassengers(), e.second[2]);
						e.second[2] -= maxAmount;
					}
					if (v.second->type->provideFreightCargo && e.second[1] > 0)
					{
						reserved = true;
						int maxAmount =
						    std::min(v.second->getMaxCargo() - v.second->getCargo(), e.second[1]);
						e.second[1] -= maxAmount;
					}
					if (v.second->type->provideFreightBio && e.second[0] > 0)
					{
						reserved = true;
						int maxAmount =
						    std::min(v.second->getMaxBio() - v.second->getBio(), e.second[0]);
						e.second[0] -= maxAmount;
					}
					if (reserved)
					{
						inboundFerries.insert(bld.first);
						break;
					}
				}
				// Clear up orgs that no longer need ferry
				if (reserved)
				{
					std::set<StateRef<Organisation>> orgsToRemove;
					for (auto &e : bld.second)
					{
						if (e.second[0] == 0 && e.second[1] == 0 && e.second[2] == 0)
						{
							orgsToRemove.insert(e.first);
						}
					}
					for (auto &o : orgsToRemove)
					{
						bld.second.erase(o);
					}
					break;
				}
			}
		}
		// Clear those satisfied
		std::set<StateRef<Building>> satisfiedBuildings;
		for (auto &bld : spaceNeeded)
		{
			// Already satisfied
			if (bld.second.empty())
			{
				satisfiedBuildings.insert(bld.first);
			}
		}
		for (auto &b : satisfiedBuildings)
		{
			spaceNeeded.erase(b);
		}
	}

	// Step 05: Order new ferries for remaining capacity
	if (!spaceNeeded.empty())
	{
		for (auto &v : state.vehicles)
		{
			// Check if in this city
			if (v.second->city != city)
			{
				continue;
			}
			// Check if company provides ferries and vehicle available
			if (!v.second->missions.empty() || !v.second->owner->providesTransportationServices)
			{
				continue;
			}
			// Check if is a ferry
			if (!v.second->type->provideFreightAgent && !v.second->type->provideFreightBio &&
			    !v.second->type->provideFreightCargo)
			{
				continue;
			}
			// Check which target building can reserve this transport
			for (auto &bld : spaceNeeded)
			{
				// Already satisfied
				if (bld.second.empty())
				{
					continue;
				}
				bool checkRelationship =
				    config().getBool("OpenApoc.NewFeature.FerryChecksRelationshipWhenBuying") ||
				    containsPurchase.find(bld.first) == containsPurchase.end();
				// Agrees to ferry for us
				bool checkBuildingRelationship = false;
				for (auto &e : bld.second)
				{
					if (e.second[0] != 0 || e.second[1] != 0)
					{
						checkBuildingRelationship = true;
					}
				}
				if (checkRelationship && checkBuildingRelationship &&
				    v.second->owner->isRelatedTo(bld.first->owner) ==
				        Organisation::Relation::Hostile)
				{
					continue;
				}
				bool reserved = false;
				for (auto &e : bld.second)
				{
					// Check it agrees to ferry for this org
					if (checkRelationship)
					{
						if (e.first && v.second->owner->isRelatedTo(e.first) ==
						                   Organisation::Relation::Hostile)
						{
							continue;
						}
					}
// Check what exactly can it ferry
#ifdef DEBUG_VERBOSE_CARGO_SYSTEM
					bool DEBUG_PASS = v.second->type->provideFreightAgent && e.second[2] > 0;
					bool DEBUG_CARGO = (v.second->type->provideFreightCargo && e.second[1] > 0) ||
					                   (v.second->type->provideFreightBio && e.second[0] > 0);
#endif
					if (v.second->type->provideFreightAgent && e.second[2] > 0)
					{
						reserved = true;
						int maxAmount = std::min(
						    v.second->getMaxPassengers() - v.second->getPassengers(), e.second[2]);
						e.second[2] -= maxAmount;
					}
					if (v.second->type->provideFreightCargo && e.second[1] > 0)
					{
						reserved = true;
						int maxAmount =
						    std::min(v.second->getMaxCargo() - v.second->getCargo(), e.second[1]);
						e.second[1] -= maxAmount;
					}
					if (v.second->type->provideFreightBio && e.second[0] > 0)
					{
						reserved = true;
						int maxAmount =
						    std::min(v.second->getMaxBio() - v.second->getBio(), e.second[0]);
						e.second[0] -= maxAmount;
					}
					// Order if we need it
					if (reserved)
					{
#ifdef DEBUG_VERBOSE_CARGO_SYSTEM
						LogWarning(
						    "Ordered ferry %s name %s in %s type %s owned by %s bound for %s",
						    DEBUG_CARGO && DEBUG_PASS ? "CA" : (DEBUG_CARGO ? "C" : "A"), v.first,
						    !v.second->currentBuilding ? "" : v.second->currentBuilding.id,
						    v.second->type.id, v.second->owner.id, bld.first.id);
#endif
						v.second->setMission(
						    state, VehicleMission::offerService(state, *v.second, thisRef));
						inboundFerries.insert(bld.first);
						break;
					}
				}
				// Clear up orgs that no longer need ferry
				if (reserved)
				{
					std::set<StateRef<Organisation>> orgsToRemove;
					for (auto &e : bld.second)
					{
						if (e.second[0] == 0 && e.second[1] == 0 && e.second[2] == 0)
						{
							orgsToRemove.insert(e.first);
						}
					}
					for (auto &o : orgsToRemove)
					{
						bld.second.erase(o);
					}
					break;
				}
			}
		}
		// Clear those satisfied
		std::set<StateRef<Building>> satisfiedBuildings;
		for (auto &e : spaceNeeded)
		{
			// Already satisfied
			if (e.second.empty())
			{
				satisfiedBuildings.insert(e.first);
			}
		}
		for (auto &b : satisfiedBuildings)
		{
			spaceNeeded.erase(b);
		}
	}

	// Step 06: Try to load cargo on owner's vehicles if:
	// - Allowed by game option
	// - Cargo is expiring
	// - Cargo had no ferries ordered for it
	if (config().getBool("OpenApoc.NewFeature.AllowManualCargoFerry"))
	{
		for (auto &c : cargo)
		{
			if (c.count == 0)
			{
				continue;
			}
			if (c.warned || inboundFerries.find(c.destination) == inboundFerries.end())
			{
				// First try those with matching home
				for (auto v : currentVehicles)
				{
					if (v->homeBuilding == c.destination)
					{
						v->provideService(state, false);
					}
				}
				// Then those with matching owner
				for (auto v : currentVehicles)
				{
					if (v->homeBuilding != c.destination && v->owner == c.destination->owner)
					{
						v->provideService(state, false);
					}
				}
			}
		}
	}

	// Step 07: Try to load agents on owner's vehicles if:
	// - Allowed by game option
	// - Agents are not to be serviced by incoming ferries
	if (config().getBool("OpenApoc.NewFeature.AllowManualCargoFerry"))
	{
		for (auto &bld : spaceNeeded)
		{
			for (auto &e : bld.second)
			{
				if (e.second[2] > 0)
				{
					// First try those with matching home
					for (auto v : currentVehicles)
					{
						if (v->homeBuilding == bld.first)
						{
							v->provideService(state, false);
						}
					}
					// Then those with matching owner
					for (auto v : currentVehicles)
					{
						if (v->homeBuilding != bld.first && v->owner == bld.first->owner)
						{
							v->provideService(state, false);
						}
					}
				}
			}
		}
	}

	// Step 08: Clear empty cargo
	for (auto it = cargo.begin(); it != cargo.end();)
	{
		if (it->count == 0)
		{
			it = cargo.erase(it);
		}
		else
		{
			it++;
		}
	}
}

void Building::detect(GameState &state, bool forced)
{
	if (ticksDetectionTimeOut > 0)
	{
		return;
	}
	if (!forced)
	{
		bool alien_spotted = false;
		int detectionValue = 0;
		for (auto &pair : current_crew)
		{
			if (pair.second == 0)
			{
				continue;
			}
			if (randBoundsExclusive(state.rng, 0, 100) < 100 / (pair.second + 1))
			{
				alien_spotted = true;
			}
			detectionValue += pair.first->detectionWeight * pair.second;
		}
		if (!alien_spotted)
		{
			return;
		}
		detectionValue *= function->detectionWeight - 2 * state.difficulty;
		detectionValue /= 100;
		if (owner->isRelatedTo(state.getAliens()) == Organisation::Relation::Allied)
		{
			detectionValue /= 2;
		}
		if (owner->isRelatedTo(state.getPlayer()) == Organisation::Relation::Hostile)
		{
			detectionValue /= 2;
		}
		if (owner->takenOver)
		{
			detectionValue /= 2;
		}
		if (randBoundsExclusive(state.rng, 0, 100) >= detectionValue - 10)
		{
			return;
		}
	}
	else
	{
		bool alien_spotted = false;
		for (auto &pair : current_crew)
		{
			if (pair.second == 0)
			{
				continue;
			}
			alien_spotted = true;
			break;
		}
		if (!alien_spotted)
		{
			return;
		}
	}
	state.firstDetection = false;
	ticksDetectionTimeOut = TICKS_DETECTION_TIMEOUT;
	if (base)
	{
		fw().pushEvent(new GameDefenseEvent(GameEventType::DefendTheBase, base, state.getAliens()));
	}
	else
	{
		detected = true;

		fw().pushEvent(new GameBuildingEvent(GameEventType::AlienSpotted,
		                                     {&state, Building::getId(state, shared_from_this())}));
	}
}

void Building::alienGrowth(GameState &state)
{
	// Aliens try to move
	alienMovement(state);
	// Calculate changes to building's crew
	std::map<StateRef<AgentType>, int> change_crew;
	for (auto &pair : current_crew)
	{
		int growth = 0;
		for (int i = 0; i < pair.second; i++)
		{
			if (randBoundsInclusive(state.rng, 0, 100) < pair.first->growthChance)
			{
				growth++;
			}
		}
		if (growth == 0)
		{
			continue;
		}
		// Growing aliens die
		change_crew[pair.first] -= growth;
		// And are replaced with a random one from available options
		// (or simply die if they don't have any)
		int rand = randBoundsExclusive(state.rng, 0, 100);
		for (auto &g : pair.first->growthOptions)
		{
			if (rand < g.first)
			{
				change_crew[g.second.first] += g.second.second * growth;
				break;
			}
		}
		// Additionally, suckers apply infiltration here
		if (pair.first->growthInfiltration > 0)
		{
			owner->infiltrationValue += growth * pair.first->growthInfiltration *
			                            function->infiltrationSpeed * owner->infiltrationSpeed;
			if (owner->infiltrationValue > 200)
			{
				owner->infiltrationValue = 200;
			}
		}
	}
	// Apply changes
	for (auto &pair : change_crew)
	{
		current_crew[pair.first] += pair.second;
	}
	// Disable detection if no aliens are there
	detected = detected && hasAliens();
}

void Building::alienMovement(GameState &state)
{
	if (!hasAliens())
	{
		return;
	}
	// Run once when crew landed and once every hour after grow
	// Pick 15 intact buildings within range of 15 tiles (counting from center to center)
	std::list<StateRef<Building>> neighbours;
	for (auto &b : city->buildings)
	{
		auto distVec = bounds.p0 + bounds.p1 - b.second->bounds.p0 - b.second->bounds.p1;
		distVec /= 2;
		int distance = std::abs(distVec.x) + std::abs(distVec.y);
		if (distance > 0 && distance <= 15)
		{
			neighbours.emplace_back(&state, b.first);
		}
		if (neighbours.size() >= 15)
		{
			break;
		}
	}
	if (neighbours.empty())
	{
		return;
	}
	// Pick one random of them
	auto bld = pickRandom(state.rng, neighbours);
	// For every alien calculate move percent as:
	//   alien's move chance + random 0..30
	// Calculate amount of moving aliens
	std::map<StateRef<AgentType>, int> moveAmounts;
	int totalMoveAmount = 0;
	for (auto &e : current_crew)
	{
		if (e.second == 0)
		{
			continue;
		}
		int movePercent =
		    std::min(100, e.first->movementPercent + randBoundsInclusive(state.rng, 0, 30));
		int moveAmount = e.second * movePercent / 100;
		if (moveAmount > 0)
		{
			moveAmounts[e.first] = moveAmount;
			totalMoveAmount += moveAmount;
		}
	}
	if (totalMoveAmount == 0)
	{
		return;
	}
	// Chance to move is:
	//   15 + 3 * amount + 20 (if owner is friendly+ to aliens)
	int friendlyBonus = 0;
	switch (bld->owner->isRelatedTo(state.getAliens()))
	{
		case Organisation::Relation::Friendly:
		case Organisation::Relation::Allied:
			friendlyBonus = 20;
			break;
		default:
			friendlyBonus = 0;
			break;
	}
	bool moving = randBoundsInclusive(state.rng, 0, 100) < 15 + 3 * totalMoveAmount + friendlyBonus;
	if (!moving)
	{
		return;
	}
	for (auto &e : moveAmounts)
	{
		current_crew[e.first] -= e.second;
		bld->current_crew[e.first] += e.second;
	}
	if (bld->base)
	{
		fw().pushEvent(
		    new GameDefenseEvent(GameEventType::DefendTheBase, bld->base, state.getAliens()));
	}
}

void Building::underAttack(GameState &state, StateRef<Organisation> attacker)
{
	if (owner->isRelatedTo(attacker) == Organisation::Relation::Hostile)
	{
		std::list<StateRef<Vehicle>> toLaunch;
		for (auto v : currentVehicles)
		{
			toLaunch.push_back(v);
		}
		for (auto v : toLaunch)
		{
			v->setMission(state, VehicleMission::patrol(state, *v, true, 5));
		}
		if (timeOfLastAttackEvent + TICKS_ATTACK_EVENT_TIMEOUT < state.gameTime.getTicks())
		{
			timeOfLastAttackEvent = state.gameTime.getTicks();
			fw().pushEvent(new GameBuildingEvent(GameEventType::BuildingAttacked,
			                                     {&state, shared_from_this()}, attacker));
		}
	}
}

void Building::collapse(GameState &state)
{
	std::list<sp<Scenery>> sceneryToCollapse;
	for (auto &p : buildingParts)
	{
		auto tile = city->map->getTile(p);
		if (tile->presentScenery)
		{
			sceneryToCollapse.push_back(tile->presentScenery);
		}
	}
	for (auto &s : sceneryToCollapse)
	{
		s->collapse(state);
	}
}

void Building::buildingPartChange(GameState &state, Vec3<int> part, bool intact)
{
	// FIXME: Implement proper base / building dying when enough is destroyed
	// Implement agents dying when building dies
	if (intact)
	{
		buildingParts.insert(part);
	}
	else
	{
		// Skin36 had some code figured out about this
		// which counted score of parts and when it was below certain value
		// building was considered dead
		buildingParts.erase(part);
		if (buildingParts.find(crewQuarters) == buildingParts.end())
		{
			while (!currentAgents.empty())
			{
				// For some reason need to assign first before calling die()
				auto agent = *currentAgents.begin();
				// Dying will remove agent from current agents list
				agent->die(state, true);
			}
		}
		if (!isAlive(state))
		{
			if (base)
			{
				base->die(state, true);
			}
		}
	}
}

void Building::decreasePendingInvestigatorCount(GameState &state)
{
	--this->pendingInvestigatorCount;
	if (this->pendingInvestigatorCount == 0)
	{
		fw().pushEvent(new GameBuildingEvent(GameEventType::CommenceInvestigation,
		                                     {&state, shared_from_this()}));
	}
	else if (this->pendingInvestigatorCount < 0) // shouldn't happen
	{
		LogError("Building investigate count < 0?");
		this->pendingInvestigatorCount = 0;
	}
}

bool Building::isAlive(GameState &state [[maybe_unused]]) const { return !buildingParts.empty(); }

} // namespace OpenApoc

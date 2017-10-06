#include "game/state/city/building.h"
#include "framework/framework.h"
#include "framework/configfile.h"
#include "game/state/base/base.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gameevent.h"
#include "game/state/city/agentmission.h"
#include "game/state/gamestate.h"
#include "game/state/organisation.h"

namespace OpenApoc
{

sp<BuildingFunction> BuildingFunction::get(const GameState &state, const UString &id)
{
	auto it = state.building_functions.find(id);
	if (it == state.building_functions.end())
	{
		LogError("No building_function matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &BuildingFunction::getPrefix()
{
	static UString prefix = "BUILDINGFUNCTION_";
	return prefix;
}
const UString &BuildingFunction::getTypeName()
{
	static UString name = "AgentType";
	return name;
}

sp<Building> Building::get(const GameState &state, const UString &id)
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

const UString &Building::getPrefix()
{
	static UString prefix = "BUILDING_";
	return prefix;
}
const UString &Building::getTypeName()
{
	static UString name = "Building";
	return name;
}

const UString &Building::getId(const GameState &state, const sp<Building> ptr)
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

void Building::updateCargo(GameState & state)
{
	// Update cargo, update vehicles and agents in this bld
	// FIXME: Implement calling for pickup
	
	StateRef<Building> thisPtr = { &state, getId(state, shared_from_this()) };
	// Step 01: Consume cargo with destination here
	for (auto &c : cargo)
	{
		if (c.destination == shared_from_this())
		{
			c.arrive(state);
		}
	}
	// Step 02: Check expiry dates and expire cargo
	for (auto &c : cargo)
	{
		if (c.checkExpiryDate(state) && c.destination->owner == state.getPlayer())
		{
			// FIXME: Implement warning player that cargo expires soon
			LogWarning("Implement warning player that cargo expires soon");
		}
	}
	// Step 03: Compile list of carrying capacity required
	std::map<StateRef<Building>, std::vector<int>> spaceNeeded;
	std::set<StateRef<Building>> inboundFerries;
	std::set<StateRef<Building>> containsPurchase;
	for (auto &c : cargo)
	{
		if (c.count == 0)
		{
			continue;
		}
		spaceNeeded[c.destination].resize(3);
		if (c.type == Cargo::Type::Bio)
		{
			spaceNeeded[c.destination][0] += c.count * c.space;
		}
		else
		{
			spaceNeeded[c.destination][1] += c.count * c.space;
		}
		if (c.cost > 0)
		{
			containsPurchase.insert(c.destination);
		}
	}
	for (auto &a : currentAgents)
	{
		if (a->missions.empty() || a->missions.front()->type != AgentMission::MissionType::AwaitPickup)
		{
			continue;
		}
		spaceNeeded[a->missions.front()->targetBuilding].resize(3);
		spaceNeeded[a->missions.front()->targetBuilding][2]++;
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
			if (v.second->missions.empty() || v.second->missions.back()->type != VehicleMission::MissionType::OfferService || v.second->missions.back()->missionCounter > 0)
			{
				continue;
			}
			// Check which target building can reserve this transport
			for (auto &e : spaceNeeded)
			{
				// Already satisfied
				if (e.second[0] == 0 && e.second[1] == 0 && e.second[2] == 0)
				{
					continue;
				}
				// Not bound for this building
				if (v.second->missions.back()->targetBuilding != thisPtr)
				{
					continue;
				}
				// Check if agrees to ferry for us:
				// - if this is not a purchase
				// - or if we're set to check purchases
				if (config().getBool("OpenApoc.NewFeature.FerryChecksRelationshipWhenBuying") || containsPurchase.find(e.first) == containsPurchase.end())
				{
					if (v.second->owner->isRelatedTo(owner) == Organisation::Relation::Hostile
						|| v.second->owner->isRelatedTo(e.first->owner) == Organisation::Relation::Hostile)
					{
						continue;
					}
				}
				// Check what exactly can it ferry
				bool reserved = false;
				if (v.second->type->provideFreightAgent && e.second[2] > 0)
				{
					reserved = true;
					int maxAmount = std::min(v.second->getMaxPassengers() - v.second->getPassengers(), e.second[2]);
					e.second[2] -= maxAmount;
				}
				if (v.second->type->provideFreightCargo && e.second[1] > 0)
				{
					reserved = true;
					int maxAmount = std::min(v.second->getMaxCargo() - v.second->getCargo(), e.second[1]);
					e.second[1] -= maxAmount;
				}
				if (v.second->type->provideFreightBio && e.second[0] > 0)
				{
					reserved = true;
					int maxAmount = std::min(v.second->getMaxBio() - v.second->getBio(), e.second[0]);
					e.second[0] -= maxAmount;
				}
				if (reserved)
				{
					inboundFerries.insert(thisPtr);
					break;
				}
			}
		}
		// Clear those satisfied
		std::set<StateRef<Building>> satisfiedBuildings;
		for (auto &e : spaceNeeded)
		{
			// Already satisfied
			if (e.second[0] == 0 && e.second[1] == 0 && e.second[2] == 0)
			{
				satisfiedBuildings.insert(e.first);
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
			if (!v.second->type->provideFreightAgent && !v.second->type->provideFreightBio && !v.second->type->provideFreightCargo)
			{
				continue;
			}
			// Check which target building can reserve this transport
			for (auto &e : spaceNeeded)
			{
				// Already satisfied
				if (e.second[0] == 0 && e.second[1] == 0 && e.second[2] == 0)
				{
					continue;
				}
				// Agrees to ferry for us
				if (config().getBool("OpenApoc.NewFeature.FerryChecksRelationshipWhenBuying") || containsPurchase.find(e.first) == containsPurchase.end())
				{
					if (v.second->owner->isRelatedTo(owner) == Organisation::Relation::Hostile
						|| v.second->owner->isRelatedTo(e.first->owner) == Organisation::Relation::Hostile)
					{
						continue;
					}
				}
				// Check what exactly can it ferry
				bool reserved = false;
				if (v.second->type->provideFreightAgent && e.second[2] > 0)
				{
					reserved = true;
					int maxAmount = std::min(v.second->getMaxPassengers() - v.second->getPassengers(), e.second[2]);
					e.second[2] -= maxAmount;
				}
				if (v.second->type->provideFreightCargo && e.second[1] > 0)
				{
					reserved = true;
					int maxAmount = std::min(v.second->getMaxCargo() - v.second->getCargo(), e.second[1]);
					e.second[1] -= maxAmount;
				}
				if (v.second->type->provideFreightBio && e.second[0] > 0)
				{
					reserved = true;
					int maxAmount = std::min(v.second->getMaxBio() - v.second->getBio(), e.second[0]);
					e.second[0] -= maxAmount;
				}
				// Order if we need it
				if (reserved)
				{
					v.second->setMission(state, VehicleMission::offerService(state, *v.second, thisPtr));
					inboundFerries.insert(thisPtr);
					break;
				}
			}
		}
		// Clear those satisfied
		std::set<StateRef<Building>> satisfiedBuildings;
		for (auto &e : spaceNeeded)
		{
			// Already satisfied
			if (e.second[0] == 0 && e.second[1] == 0 && e.second[2] == 0)
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
	// - Cargo is expriring
	// - Cargo had no ferries ordered for it
	for (auto &c : cargo)
	{
		if (c.count == 0)
		{
			continue;
		}
		if (c.warned || inboundFerries.find(c.destination) == inboundFerries.end())
		{
			for (auto v : currentVehicles)
			{
				if (v->homeBuilding == c.destination)
				{
					v->provideService(state, false);
				}
			}
		}
	}
	// Step 07: Try to load agents on owner's vehicles if:
	// - Agents are not to be serviced by incoming ferries
	for (auto &e : spaceNeeded)
	{
		if (e.second[2] > 0)
		{
			for (auto v : currentVehicles)
			{
				if (v->homeBuilding == e.first)
				{
					v->provideService(state, false);
				}
			}
		}
	}
	// Step 08: Clear empty cargo
	for (auto it = cargo.begin(); it !=cargo.end();)
	{
		if (it->count = 0)
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
	StateRef<Base> base;
	if (owner == state.getPlayer())
	{
		auto thisSP = shared_from_this();
		for (auto &b : state.player_bases)
		{
			if (b.second->building == thisSP)
			{
				base = {&state, b.first};
				break;
			}
		}
	}
	if (base)
	{
		auto event = new GameDefenseEvent(GameEventType::DefendTheBase, base, state.getAliens());
		fw().pushEvent(event);
	}
	else
	{
		detected = true;

		auto event = new GameBuildingEvent(GameEventType::AlienSpotted,
		                                   {&state, Building::getId(state, shared_from_this())});
		fw().pushEvent(event);
	}
}

void Building::alienGrowth(GameState &state)
{
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

} // namespace OpenApoc

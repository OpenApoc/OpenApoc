#include "game/state/city/base.h"
#include "dependencies/magic_enum/include/magic_enum/magic_enum.hpp"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/facility.h"
#include "game/state/city/vehicle.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/city/baselayout.h"
#include "game/state/rules/city/facilitytype.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/shared/organisation.h"
#include "library/strings_format.h"
#include <random>

namespace OpenApoc
{

constexpr int Base::SIZE;

template <> sp<Base> StateObject<Base>::get(const GameState &state, const UString &id)
{
	auto it = state.player_bases.find(id);
	if (it == state.player_bases.end())
	{
		LogError("No baseas matching ID \"{}\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<Base>::getPrefix()
{
	static UString prefix = "BASE_";
	return prefix;
}
template <> const UString &StateObject<Base>::getTypeName()
{
	static UString name = "Base";
	return name;
}
template <> const UString &StateObject<Base>::getId(const GameState &state, const sp<Base> ptr)
{
	static const UString emptyString = "";
	for (auto &b : state.player_bases)
	{
		if (b.second == ptr)
			return b.first;
	}
	LogError("No base matching pointer {}", static_cast<void *>(ptr.get()));
	return emptyString;
}

Base::Base(GameState &state, StateRef<Building> building) : building(building)
{
	corridors = std::vector<std::vector<bool>>(SIZE, std::vector<bool>(SIZE, false));
	for (auto &rect : building->base_layout->baseCorridors)
	{
		for (int x = rect.p0.x; x < rect.p1.x; ++x)
		{
			for (int y = rect.p0.y; y < rect.p1.y; ++y)
			{
				corridors[x][y] = true;
			}
		}
	}
	StateRef<FacilityType> type = {&state, FacilityType::getPrefix() + "ACCESS_LIFT"};
	if (canBuildFacility(type, building->base_layout->baseLift, true) != BuildError::NoError)
	{
		LogError("Building {} has invalid lift location", building->name);
	}
	else
	{
		buildFacility(state, type, building->base_layout->baseLift, true);
	}
}

void Base::die(GameState &state, bool collapse)
{
	fw().pushEvent(new GameSomethingDiedEvent(
	    GameEventType::BaseDestroyed, name,
	    collapse ? /*by collapsing building*/ "" : "byEnemyForces", building->crewQuarters));

	for (auto &b : building->city->buildings)
	{
		for (auto &c : b.second->cargo)
		{
			if (c.destination == building)
			{
				c.refund(state, {&state, b.first});
			}
		}
	}
	for (auto &v : state.vehicles)
	{
		for (auto &c : v.second->cargo)
		{
			if (c.destination == building)
			{
				// This is base transfer in progress
				if (c.originalOwner == c.destination->owner)
				{
					// Re-route to another base
					for (auto &b : state.player_bases)
					{
						if (b.second->building == building)
						{
							continue;
						}
						c.destination = b.second->building;
						break;
					}
				}
				else
				{
					c.refund(state, nullptr);
				}
			}
		}
	}
	for (auto a : building->currentAgents)
	{
		a->die(state, true);
	}
	for (auto v : building->currentVehicles)
	{
		v->die(state, true);
	}
	building->base.clear();
	building->owner = state.getGovernment();
	building->collapse(state);
	building.clear();

	state.player_bases.erase(Base::getId(state, shared_from_this()));
	state.current_base.clear();
	if (state.player_bases.empty())
	{
		LogError("Player lost, but we have no screen for that yet!");
		return;
	}
	else
	{
		state.current_base = {&state, state.player_bases.begin()->first};
	}
}

sp<Facility> Base::getFacility(Vec2<int> pos) const
{
	for (auto &facility : facilities)
	{
		if (pos.x >= facility->pos.x && pos.x < facility->pos.x + facility->type->size &&
		    pos.y >= facility->pos.y && pos.y < facility->pos.y + facility->type->size)
		{
			return facility;
		}
	}
	return nullptr;
}

static bool randomlyPlaceFacility(GameState &state, Base &base, StateRef<FacilityType> facility)
{

	std::vector<Vec2<int>> possible_positions;
	for (int y = 0; y < base.SIZE; y++)
	{
		for (int x = 0; x < base.SIZE; x++)
		{
			Vec2<int> position{x, y};
			if (base.canBuildFacility(facility, position, true) == Base::BuildError::NoError)
			{
				possible_positions.push_back(position);
			}
		}
	}
	if (possible_positions.empty())
	{
		return false;
	}
	std::uniform_int_distribution<int> positionDistribution(0, (int)possible_positions.size() - 1);
	auto position = possible_positions[positionDistribution(state.rng)];
	if (base.canBuildFacility(facility, position, true) == Base::BuildError::NoError)
	{
		base.buildFacility(state, facility, position, true);
		return true;
	}
	else
	{
		LogError("Position {} in base in possible list but failed to build", position);
		return false;
	}
}

static bool tryToPlaceInitialFacilities(GameState &state, Base &base)
{
	for (auto &facilityTypePair : state.initial_facilities)
	{
		StateRef<FacilityType> facilityType{&state, facilityTypePair.first};
		auto count = facilityTypePair.second;
		for (unsigned int i = 0; i < count; i++)
		{
			if (!randomlyPlaceFacility(state, base, facilityType))
			{
				return false;
			}
		}
	}
	return true;
}

void Base::startingBase(GameState &state)
{
	while (!tryToPlaceInitialFacilities(state, *this))
	{
		LogInfo("Failed to place facilities, trying again");
		// Cleanup the partially-placed base
		for (int y = 0; y < this->SIZE; y++)
		{
			for (int x = 0; x < this->SIZE; x++)
			{
				this->destroyFacility(state, {x, y});
			}
		}
		// There always must be a single 'access lift' base module even if everything else has been
		// removed
		if (this->facilities.size() > 1)
		{
			LogError("Failed to cleanup facilities after unsuccessful random place");
		}
		if (this->facilities.empty())
		{
			LogError("No access lift after facility cleanup after unsuccessful random place");
		}
		if (this->facilities[0]->type->fixed != true)
		{
			LogError("Remaining facility after cleanup not an access lift?");
		}
	}
}

Base::BuildError Base::canBuildFacility(StateRef<FacilityType> type, Vec2<int> pos, bool free) const
{
	if (pos.x < 0 || pos.y < 0 || pos.x + type->size > SIZE || pos.y + type->size > SIZE)
	{
		return BuildError::OutOfBounds;
	}
	for (int x = pos.x; x < pos.x + type->size; ++x)
	{
		for (int y = pos.y; y < pos.y + type->size; ++y)
		{
			if (!corridors[x][y])
			{
				return BuildError::OutOfBounds;
			}
		}
	}
	for (int x = pos.x; x < pos.x + type->size; ++x)
	{
		for (int y = pos.y; y < pos.y + type->size; ++y)
		{
			if (getFacility({x, y}) != nullptr)
			{
				return BuildError::Occupied;
			}
		}
	}
	if (!free)
	{
		if (!building)
		{
			LogError("Building disappeared");
			return BuildError::OutOfBounds;
		}
		else if (building->owner->balance - type->buildCost < 0)
		{
			return BuildError::NoMoney;
		}
	}
	return BuildError::NoError;
}

void Base::buildFacility(GameState &state, StateRef<FacilityType> type, Vec2<int> pos, bool free)
{
	const auto canBuildFacilityResult = canBuildFacility(type, pos, free);

	if (canBuildFacilityResult != BuildError::NoError)
	{
		const auto canBuildFacilityEnum = magic_enum::enum_name(canBuildFacilityResult);
		const auto logMessage =
		    fmt::format("Error when trying to build facility: {}", canBuildFacilityEnum);
		LogWarning(logMessage);
		return;
	}

	auto facility = mksp<Facility>(type);
	facilities.push_back(facility);
	facility->pos = pos;
	if (!free)
	{
		if (!building)
		{
			LogError("Building disappeared");
		}
		else
		{
			building->owner->balance -= type->buildCost;
		}
		facility->buildTime = type->buildTime;
	}
	if (type->capacityAmount > 0)
	{
		ResearchTopic::LabSize size;
		// FIXME: Make LabSize set-able outside of the facility size?
		if (type->size > 1)
		{
			size = ResearchTopic::LabSize::Large;
		}
		else
		{
			size = ResearchTopic::LabSize::Small;
		}
		switch (type->capacityType)
		{
			case FacilityType::Capacity::Chemistry:
			{
				auto lab = mksp<Lab>();
				lab->size = size;
				lab->type = ResearchTopic::Type::BioChem;
				auto id = Lab::generateObjectID(state);
				state.research.labs[id] = lab;
				facility->lab = {&state, id};
				break;
			}
			case FacilityType::Capacity::Physics:
			{
				auto lab = mksp<Lab>();
				lab->size = size;
				lab->type = ResearchTopic::Type::Physics;
				auto id = Lab::generateObjectID(state);
				state.research.labs[id] = lab;
				facility->lab = {&state, id};
				break;
			}
			case FacilityType::Capacity::Workshop:
			{
				auto lab = mksp<Lab>();
				lab->size = size;
				lab->type = ResearchTopic::Type::Engineering;
				auto id = Lab::generateObjectID(state);
				state.research.labs[id] = lab;
				facility->lab = {&state, id};
				break;
			}
			default:
				// Non-lab modules don't need special handling
				break;
		}
	}
}

Base::BuildError Base::canDestroyFacility(GameState &state, Vec2<int> pos) const
{
	const auto facility = getFacility(pos);

	if (facility == nullptr)
	{
		return BuildError::OutOfBounds;
	}

	if (facility->type->fixed)
	{
		return BuildError::Indestructible;
	}

	if (facility->buildTime > 0)
	{
		return BuildError::NoError;
	}

	switch (facility->type->capacityType)
	{
		case FacilityType::Capacity::Quarters:
		case FacilityType::Capacity::Stores:
		case FacilityType::Capacity::Aliens:
		{
			const auto capacityUsed = getCapacityUsed(state, facility->type->capacityType);
			const auto capacityTotal = getCapacityTotal(facility->type->capacityType);
			const auto capacityAmount = facility->type->capacityAmount;

			if (capacityUsed > capacityTotal - capacityAmount)
			{
				return BuildError::Occupied;
			}
			break;
		}
		case FacilityType::Capacity::Physics:
		case FacilityType::Capacity::Chemistry:
		case FacilityType::Capacity::Workshop:
			if (facility->lab && facility->lab->current_project)
			{
				return BuildError::Occupied;
			}
			break;
		default:
			return BuildError::NoError;
	}
	return BuildError::NoError;
}

void Base::destroyFacility(GameState &state, Vec2<int> pos)
{
	const auto canDestroyFacilityResult = canDestroyFacility(state, pos);

	if (canDestroyFacilityResult != BuildError::NoError)
	{
		const auto canDestroyFacilityEnum = magic_enum::enum_name(canDestroyFacilityResult);
		const auto logMessage =
		    fmt::format("Error when trying to destroy facility: {}", canDestroyFacilityEnum);
		LogWarning(logMessage);
		return;
	}

	const auto facility = getFacility(pos);

	for (auto f = facilities.begin(); f != facilities.end(); ++f)
	{
		if (*f != facility)
			continue;

		if (facility->lab)
		{
			for (auto &a : facility->lab->assigned_agents)
			{
				a->assigned_to_lab = false;
			}
			if (facility->lab->current_project)
			{
				facility->lab->current_project->current_lab = "";
				facility->lab->current_project = "";
			}
			facility->lab = "";
			state.research.labs.erase(facility->lab.id);
		}

		if (facility->type->buildTime > 0)
		{
			building->owner->balance +=
			    facility->type->buildCost * facility->buildTime / facility->type->buildTime;
		}

		facilities.erase(f);
		break;
	}
}

bool Base::alienContainmentExists() const
{
	return getCapacityTotal(FacilityType::Capacity::Aliens) > 0;
}

bool Base::alienContainmentIsEmpty(GameState &state)
{
	for (const auto &f : facilities)
	{
		if (f->type->capacityType == FacilityType::Capacity::Aliens &&
		    getCapacityUsed(state, f->type->capacityType) != 0)
		{
			return false;
		}
	}
	return true;
}

int Base::getCapacityUsed(GameState &state, FacilityType::Capacity type) const
{
	int total = 0;
	switch (type)
	{
		case FacilityType::Capacity::Repair:
			for (const auto &v : state.vehicles)
			{
				const auto maxHealth = v.second->getMaxHealth();
				const auto health = v.second->getHealth();

				if (v.second->homeBuilding == building && maxHealth > health)
				{
					total += maxHealth - health;
				}
			}
			// Show percentage of repair bay used if it can repair in one hour, or 100% if can't
			if (total > 0)
			{
				const auto max = getCapacityTotal(type);
				return std::min(total, max);
			}
			break;
		case FacilityType::Capacity::Medical:
			for (const auto &a : state.agents)
			{
				if (a.second->homeBuilding == building &&
				    a.second->modified_stats.health < a.second->current_stats.health)
				{
					total++;
				}
			}
			break;
		case FacilityType::Capacity::Training:
			for (const auto &a : state.agents)
			{
				if (a.second->homeBuilding == building &&
				    a.second->trainingAssignment == TrainingAssignment::Physical)
				{
					total++;
				}
			}
			break;
		case FacilityType::Capacity::Psi:
			for (const auto &a : state.agents)
			{
				if (a.second->homeBuilding == building &&
				    a.second->trainingAssignment == TrainingAssignment::Psi)
				{
					total++;
				}
			}
			break;
		case FacilityType::Capacity::Chemistry:
			[[fallthrough]];
		case FacilityType::Capacity::Physics:
			[[fallthrough]];
		case FacilityType::Capacity::Workshop:
			for (const auto &f : facilities)
			{
				if (f->type->capacityType == type && f->lab)
				{
					total += (int)f->lab->assigned_agents.size();
				}
			}
			break;
		case FacilityType::Capacity::Quarters:
			for (const auto &a : state.agents)
			{
				if (a.second->homeBuilding == building)
				{
					total++;
				}
			}
			break;
		case FacilityType::Capacity::Stores:
			for (const auto &e : inventoryAgentEquipment)
			{
				StateRef<AEquipmentType> ae = {&state, e.first};
				total += ae->type == AEquipmentType::Type::Ammo
				             ? ae->store_space * ((e.second + ae->max_ammo - 1) / ae->max_ammo)
				             : ae->store_space * e.second;
			}
			for (const auto &e : inventoryVehicleEquipment)
			{
				StateRef<VEquipmentType> ve = {&state, e.first};
				total += ve->store_space * e.second;
			}
			for (const auto &e : inventoryVehicleAmmo)
			{
				StateRef<VAmmoType> va = {&state, e.first};
				total += va->store_space * e.second;
			}
			break;
		case FacilityType::Capacity::Aliens:
		{
			for (const auto &e : inventoryBioEquipment)
			{
				if (e.second == 0)
					continue;

				StateRef<AEquipmentType> ae = {&state, e.first};
				total += ae->store_space * e.second;
			}
		}
		break;
		case FacilityType::Capacity::Nothing:
			// Nothing needs to be handled
			break;
	}

	return total;
}

int Base::getCapacityTotal(FacilityType::Capacity type) const
{
	int total = 0;
	for (const auto &f : facilities)
	{
		if (f->type->capacityType == type && f->buildTime == 0)
		{
			total += f->type->capacityAmount;
		}
	}
	return total;
}

float Base::getUsage(GameState &state, const sp<Facility> facility, const int delta) const
{
	if (!facility->lab)
		return getUsage(state, facility->type->capacityType, delta);

	float usage = 0.0f;
	if (delta != 0)
	{
		LogError("Delta is only supposed to be used with stores, alien containment and LQ!");
	}

	if (facility->lab->current_project)
	{
		usage = (float)facility->lab->assigned_agents.size();
		usage /= facility->type->capacityAmount;
	}

	return static_cast<float>(ceilf(usage * 100.0f));
}

float Base::getUsage(GameState &state, const FacilityType::Capacity type, const int delta) const
{
	const auto used = getCapacityUsed(state, type) + delta;
	const auto total = getCapacityTotal(type);
	if (total == 0)
	{
		return used > 0 ? 999 : 0;
	}

	auto usage = (float)used / total * 100;
	usage = std::min(999.f, usage);

	return usage;
}
}; // namespace OpenApoc

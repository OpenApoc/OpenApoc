#include "game/state/base/base.h"
#include "game/state/base/facility.h"
#include "game/state/city/baselayout.h"
#include "game/state/city/building.h"
#include "game/state/gamestate.h"
#include "game/state/organisation.h"
#include "game/state/rules/facility_type.h"
#include "library/strings_format.h"
#include <random>

namespace OpenApoc
{

constexpr int Base::SIZE;

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
		LogError("Building %s has invalid lift location", building->name);
	}
	else
	{
		buildFacility(state, type, building->base_layout->baseLift, true);
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
		LogError("Position %s in base in possible list but failed to build", position);
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
	if (canBuildFacility(type, pos, free) == BuildError::NoError)
	{
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
}

Base::BuildError Base::canDestroyFacility(Vec2<int> pos) const
{
	auto facility = getFacility(pos);
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
			if (getCapacityUsed(facility->type->capacityType) >
			    getCapacityTotal(facility->type->capacityType) - facility->type->capacityAmount)
			{
				return BuildError::Occupied;
			}
			break;
		case FacilityType::Capacity::Physics:
		case FacilityType::Capacity::Chemistry:
		case FacilityType::Capacity::Workshop:
			if (facility->lab)
			{
				if (facility->lab->current_project)
				{
					return BuildError::Occupied;
				}
			}
			break;
		default:
			return BuildError::NoError;
	}
	return BuildError::NoError;
}

void Base::destroyFacility(GameState &state, Vec2<int> pos)
{
	if (canDestroyFacility(pos) == BuildError::NoError)
	{
		auto facility = getFacility(pos);
		for (auto f = facilities.begin(); f != facilities.end(); ++f)
		{
			if (*f == facility)
			{
				if (facility->lab)
				{
					auto id = facility->lab.id;
					if (facility->lab->current_project)
					{
						facility->lab->current_project->current_lab = "";
						facility->lab->current_project = "";
					}
					facility->lab = "";
					state.research.labs.erase(id);
				}
				facilities.erase(f);
				break;
			}
		}
	}
}

int Base::getCapacityUsed(FacilityType::Capacity type) const
{
	int total = 0;
	for (auto f = facilities.begin(); f != facilities.end(); ++f)
	{
		if ((*f)->type->capacityType == type)
		{
			if ((*f)->lab)
			{
				total += (int)(*f)->lab->assigned_agents.size();
			}
			else
			{
				// TODO: Calculate usage of other facilities
			}
		}
	}
	return total;
}

int Base::getCapacityTotal(FacilityType::Capacity type) const
{
	int total = 0;
	for (auto f = facilities.begin(); f != facilities.end(); ++f)
	{
		if ((*f)->type->capacityType == type)
		{
			total += (*f)->type->capacityAmount;
		}
	}
	return total;
}

int Base::getUsage(sp<Facility> facility) const
{
	float usage = 0.0f;
	if (facility->lab)
	{
		if (facility->lab->current_project)
		{
			usage = (float)facility->lab->assigned_agents.size();
			usage /= facility->type->capacityAmount;
		}
	}
	else
	{
		usage = (float)getCapacityUsed(facility->type->capacityType);
		usage /= getCapacityTotal(facility->type->capacityType);
	}
	return static_cast<int>(usage * 100);
}

sp<Base> Base::get(const GameState &state, const UString &id)
{
	auto it = state.player_bases.find(id);
	if (it == state.player_bases.end())
	{
		LogError("No baseas matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &Base::getPrefix()
{
	static UString prefix = "BASE_";
	return prefix;
}
const UString &Base::getTypeName()
{
	static UString name = "Base";
	return name;
}
const UString &Base::getId(const GameState &state, const sp<Base> ptr)
{
	static const UString emptyString = "";
	for (auto &b : state.player_bases)
	{
		if (b.second == ptr)
			return b.first;
	}
	LogError("No base matching pointer %p", ptr.get());
	return emptyString;
}
}; // namespace OpenApoc

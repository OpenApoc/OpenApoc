#include "game/state/base/base.h"
#include "game/state/base/facility.h"
#include "game/state/city/building.h"
#include "game/state/organisation.h"

#include <random>

namespace OpenApoc
{

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
		LogError("Building %s has invalid lift location", building->name.c_str());
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

static void randomlyPlaceFacility(GameState &state, Base &base, StateRef<FacilityType> facility)
{
	auto positionDistribution = std::uniform_int_distribution<int>(0, base.SIZE);
	while (true)
	{
		auto position = Vec2<int>{positionDistribution(state.rng), positionDistribution(state.rng)};
		if (base.canBuildFacility(facility, position, true) == Base::BuildError::NoError)
		{
			base.buildFacility(state, facility, position, true);
			return;
		}
	}
}

void Base::startingBase(GameState &state)
{
	// Place large facilites first, as they're more likely to not fit...
	for (auto &facilityTypePair : state.initial_facilities)
	{
		StateRef<FacilityType> facility = {&state, facilityTypePair.first};
		if (facility->size < 2)
			continue;
		randomlyPlaceFacility(state, *this, facility);
	}
	for (auto &facilityTypePair : state.initial_facilities)
	{
		StateRef<FacilityType> facility = {&state, facilityTypePair.first};
		if (facility->size > 1)
			continue;
		randomlyPlaceFacility(state, *this, facility);
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
					auto id = UString::format("%s%u", Lab::getPrefix().c_str(),
					                          state.research.num_labs_created++);
					state.research.labs[id] = lab;
					facility->lab = {&state, id};
					break;
				}
				case FacilityType::Capacity::Physics:
				{
					auto lab = mksp<Lab>();
					lab->size = size;
					lab->type = ResearchTopic::Type::Physics;
					auto id = UString::format("%s%u", Lab::getPrefix().c_str(),
					                          state.research.num_labs_created++);
					state.research.labs[id] = lab;
					facility->lab = {&state, id};
					break;
				}
				case FacilityType::Capacity::Workshop:
				{
					auto lab = mksp<Lab>();
					lab->size = size;
					lab->type = ResearchTopic::Type::Engineering;
					auto id = UString::format("%s%u", Lab::getPrefix().c_str(),
					                          state.research.num_labs_created++);
					state.research.labs[id] = lab;
					facility->lab = {&state, id};
					break;
				}
				// TODO: Engineering 'labs'
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
		return BuildError::Occupied;
	}
	if (facility->lab)
	{
		if (facility->lab->current_project)
		{
			return BuildError::Occupied;
		}
		if (!facility->lab->assigned_agents.empty())
		{
			return BuildError::Occupied;
		}
	}
	// TODO: Check if facility is in use
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

template <> sp<Base> StateObject<Base>::get(const GameState &state, const UString &id)
{
	auto it = state.player_bases.find(id);
	if (it == state.player_bases.end())
	{
		LogError("No baseas matching ID \"%s\"", id.c_str());
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
	LogError("No base matching pointer %p", ptr.get());
	return emptyString;
}
}; // namespace OpenApoc

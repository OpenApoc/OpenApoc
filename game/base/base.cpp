#include "game/base/base.h"
#include "framework/framework.h"
#include "game/base/facility.h"
#include "game/city/building.h"
#include "game/organisation.h"

#include <unordered_set>

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
		buildFacility(type, building->base_layout->baseLift, true);
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

void Base::startingBase(GameState &state, std::default_random_engine &rng)
{
	// Figure out positions available for placement
	std::vector<Vec2<int>> positions;
	for (int x = 0; x < SIZE; ++x)
	{
		for (int y = 0; y < SIZE; ++y)
		{
			if (corridors[x][y] && getFacility({x, y}) == nullptr)
			{
				positions.push_back({x, y});
			}
		}
	}

	// Randomly fill them with facilities
	for (auto &i : state.facility_types)
	{
		if (i.second->fixed)
			continue;
		if (positions.empty())
			break;
		std::uniform_int_distribution<int> facilityPos(0, positions.size() - 1);
		buildFacility({&state, i.first}, positions[facilityPos(rng)], true);

		// Clear used positions
		for (auto pos = positions.begin(); pos != positions.end();)
		{
			if (getFacility(*pos) != nullptr)
			{
				pos = positions.erase(pos);
			}
			else
			{
				++pos;
			}
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
			if (getFacility({x, y}) != nullptr)
			{
				return BuildError::Occupied;
			}
		}
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

void Base::buildFacility(StateRef<FacilityType> type, Vec2<int> pos, bool free)
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
	// TODO: Check if facility is in use
	return BuildError::NoError;
}

void Base::destroyFacility(Vec2<int> pos)
{
	if (canDestroyFacility(pos) == BuildError::NoError)
	{
		auto facility = getFacility(pos);
		for (auto f = facilities.begin(); f != facilities.end(); ++f)
		{
			if (*f == facility)
			{
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
}; // namespace OpenApoc

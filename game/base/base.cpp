#include "game/base/base.h"
#include "game/city/building.h"
#include "game/organisation.h"
#include "game/base/facility.h"
#include "game/rules/buildingdef.h"
#include "framework/framework.h"

#include <unordered_set>

namespace OpenApoc
{

Base::Base(sp<Building> building) : bld(building)
{
	corridors = std::vector<std::vector<bool>>(SIZE, std::vector<bool>(SIZE, false));
	for (auto &rect : building->def.getBaseCorridors())
	{
		for (int x = rect.p0.x; x <= rect.p1.x; ++x)
		{
			for (int y = rect.p0.y; y <= rect.p1.y; ++y)
			{
				corridors[x][y] = true;
			}
		}
	}
	auto &def = fw().rules->getFacilityDefs().at("ACCESS_LIFT");
	if (canBuildFacility(def, building->def.getBaseLift(), true) != BuildError::None)
	{
		LogError("Building %s has invalid lift location", building->def.getName().c_str());
	}
	else
	{
		buildFacility(def, building->def.getBaseLift(), true);
	}
}

sp<const Facility> Base::getFacility(Vec2<int> pos) const
{
	for (auto &facility : facilities)
	{
		if (pos.x >= facility->pos.x && pos.x < facility->pos.x + facility->def.size &&
		    pos.y >= facility->pos.y && pos.y < facility->pos.y + facility->def.size)
		{
			return facility;
		}
	}
	return nullptr;
}

void Base::startingBase(std::default_random_engine &rng)
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
	for (auto &i : fw().rules->getFacilityDefs())
	{
		if (i.second.fixed)
			continue;
		if (positions.empty())
			break;
		std::uniform_int_distribution<int> facilityPos(0, positions.size() - 1);
		buildFacility(i.second, positions[facilityPos(rng)], true);

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

Base::BuildError Base::canBuildFacility(const FacilityDef &def, Vec2<int> pos, bool free) const
{
	if (pos.x < 0 || pos.y < 0 || pos.x + def.size > SIZE || pos.y + def.size > SIZE)
	{
		return BuildError::OutOfBounds;
	}
	for (int x = pos.x; x < pos.x + def.size; ++x)
	{
		for (int y = pos.y; y < pos.y + def.size; ++y)
		{
			if (getFacility({x, y}) != nullptr)
			{
				return BuildError::Occupied;
			}
		}
	}
	for (int x = pos.x; x < pos.x + def.size; ++x)
	{
		for (int y = pos.y; y < pos.y + def.size; ++y)
		{
			if (!corridors[x][y])
			{
				return BuildError::OutOfBounds;
			}
		}
	}
	if (!free)
	{
		auto building = bld.lock();
		if (!building)
		{
			LogError("Building disappeared");
			return BuildError::OutOfBounds;
		}
		else if (building->owner->balance - def.buildCost < 0)
		{
			return BuildError::NoMoney;
		}
	}
	return BuildError::None;
}

void Base::buildFacility(const FacilityDef &def, Vec2<int> pos, bool free)
{
	if (canBuildFacility(def, pos, free) == BuildError::None)
	{
		facilities.emplace_back(new Facility(def));
		auto facility = facilities.back();
		facility->pos = pos;
		if (!free)
		{
			auto building = bld.lock();
			if (!building)
			{
				LogError("Building disappeared");
			}
			else
			{
				building->owner->balance -= def.buildCost;
			}
			facility->buildTime = def.buildTime;
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
	if (facility->def.fixed)
	{
		return BuildError::Occupied;
	}
	// TODO: Check if facility is in use
	return BuildError::None;
}

void Base::destroyFacility(Vec2<int> pos)
{
	if (canDestroyFacility(pos) == BuildError::None)
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

}; // namespace OpenApoc

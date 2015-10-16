#include "game/base/base.h"
#include "game/city/building.h"
#include "game/organisation.h"
#include "game/rules/buildingdef.h"
#include "framework/framework.h"

namespace OpenApoc
{

Base::Base(const Building &building, const Framework &fw) : building(building)
{
	corridors = std::vector<std::vector<bool>>(SIZE, std::vector<bool>(SIZE, false));
	for (auto &rect : building.def.getBaseCorridors())
	{
		for (int x = rect.p0.x; x <= rect.p1.x; ++x)
		{
			for (int y = rect.p0.y; y <= rect.p1.y; ++y)
			{
				corridors[x][y] = true;
			}
		}
	}
	const FacilityDef &def = fw.rules->getFacilityDefs().at("ACCESS_LIFT");
	if (canBuildFacility(def, building.def.getBaseLift()) != BuildError::None)
	{
		LogError("Building %s has invalid lift location", building.def.getName().c_str());
	}
	else
	{
		buildFacility(def, building.def.getBaseLift());
	}
}

const Facility *Base::getFacility(Vec2<int> pos) const
{
	for (auto &facility : facilities)
	{
		if (pos.x >= facility.pos.x && pos.x < facility.pos.x + facility.def.size &&
		    pos.y >= facility.pos.y && pos.y < facility.pos.y + facility.def.size)
		{
			return &facility;
		}
	}
	return nullptr;
}

Base::BuildError Base::canBuildFacility(const FacilityDef &def, Vec2<int> pos) const
{
	if (pos.x < 0 || pos.y < 0 || pos.x + def.size > SIZE || pos.y + def.size > SIZE)
	{
		return BuildError::OutOfBounds;
	}
	if (getFacility(pos) != nullptr)
	{
		return BuildError::Occupied;
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
	if (!def.fixed && building.owner.balance - def.buildCost < 0)
	{
		return BuildError::NoMoney;
	}
	return BuildError::None;
}

void Base::buildFacility(const FacilityDef &def, Vec2<int> pos)
{
	if (canBuildFacility(def, pos) == BuildError::None)
	{
		building.owner.balance -= def.buildCost;
		facilities.emplace_back(def);
		facilities.back().buildTime = def.buildTime;
		facilities.back().pos = pos;
	}
}

Base::BuildError Base::canDestroyFacility(Vec2<int> pos) const
{
	const Facility *facility = getFacility(pos);
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
		const Facility *facility = getFacility(pos);
		for (auto f = facilities.begin(); f != facilities.end(); ++f)
		{
			if (&*f == facility)
			{
				// facilities.erase(f);
				break;
			}
		}
	}
}

}; // namespace OpenApoc

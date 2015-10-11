#include "game/base/base.h"
#include "game/city/building.h"
#include "game/organisation.h"

namespace OpenApoc
{

Base::Base(const Building &building) : building(building) {}

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

Base::BuildError Base::canBuildFacility(const FacilityDef& def, Vec2<int> pos) const
{
	const Facility *facility = getFacility(pos);
	if (facility != nullptr)
	{
		return BuildError::Occupied;
	}
	else if (building.owner.balance - def.buildCost < 0)
	{
		return BuildError::NoMoney;
	}
	else
	{
		// TODO: Check if facility is out of bounds
	}
	return BuildError::None;
}


void Base::buildFacility(const FacilityDef& def, Vec2<int> pos)
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
	else
	{
		// TODO: Check if facility is in use
	}
	return BuildError::None;
}

void Base::destroyFacility(Vec2<int> pos)
{
	if (canDestroyFacility(pos) == BuildError::None)
	{
		const Facility *facility = getFacility(pos);
		for (auto f = facilities.begin(); f != facilities.end(); ++f)
		{
			//if (&*f == facility)
			{
				//facilities.erase(f);
				break;
			}
		}
	}
}

}; // namespace OpenApoc

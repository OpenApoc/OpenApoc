#include "game/state/rules/facility_type.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

const std::map<FacilityType::Capacity, UString> FacilityType::CapacityMap = {

    {Capacity::Nothing, "nothing"},   {Capacity::Quarters, "quarters"},
    {Capacity::Stores, "stores"},     {Capacity::Medical, "medical"},
    {Capacity::Training, "training"}, {Capacity::Psi, "psi"},
    {Capacity::Repair, "repair"},     {Capacity::Chemistry, "chemistry"},
    {Capacity::Physics, "physics"},   {Capacity::Workshop, "workshop"},
    {Capacity::Aliens, "aliens"},

};

FacilityType::FacilityType()
    : fixed(false), buildCost(0), buildTime(0), weeklyCost(0), capacityType(Capacity::Nothing),
      capacityAmount(0), size(1)
{
}

bool FacilityType::isVisible() const { return !this->fixed && this->dependency.satisfied(); }

sp<FacilityType> FacilityType::get(const GameState &state, const UString &id)
{
	auto it = state.facility_types.find(id);
	if (it == state.facility_types.end())
	{
		LogError("No facility type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &FacilityType::getPrefix()
{
	static UString prefix = "FACILITYTYPE_";
	return prefix;
}

const UString &FacilityType::getTypeName()
{
	static UString name = "FacilityType";
	return name;
}

} // namespace OpenApoc

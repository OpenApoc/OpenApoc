#include "game/state/rules/facility_type.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

const std::map<FacilityType::Capacity, UString> FacilityType::CapacityMap = {

    {FacilityType::Capacity::Nothing, "nothing"},
    {FacilityType::Capacity::Quarters, "quarters"},
    {FacilityType::Capacity::Stores, "stores"},
    {FacilityType::Capacity::Medical, "medical"},
    {FacilityType::Capacity::Training, "training"},
    {FacilityType::Capacity::Psi, "psi"},
    {FacilityType::Capacity::Repair, "repair"},
    {FacilityType::Capacity::Chemistry, "chemistry"},
    {FacilityType::Capacity::Physics, "physics"},
    {FacilityType::Capacity::Workshop, "workshop"},
    {FacilityType::Capacity::Aliens, "aliens"},

};

FacilityType::FacilityType()
    : fixed(false), buildCost(0), buildTime(0), weeklyCost(0), capacityType(Capacity::Nothing),
      capacityAmount(0), size(1)
{
}

template <>
sp<FacilityType> StateObject<FacilityType>::get(const GameState &state, const UString &id)
{
	auto it = state.facility_types.find(id);
	if (it == state.facility_types.end())
	{
		LogError("No facility type matching ID \"%s\"", id.c_str());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<FacilityType>::getPrefix()
{
	static UString prefix = "FACILITYTYPE_";
	return prefix;
}
template <> const UString &StateObject<FacilityType>::getTypeName()
{
	static UString name = "FacilityType";
	return name;
}

} // namespace OpenApoc

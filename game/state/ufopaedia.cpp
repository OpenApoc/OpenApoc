#include "game/state/ufopaedia.h"
#include "game/state/research.h"

namespace OpenApoc
{

const std::map<UfopaediaEntry::Data, UString> UfopaediaEntry::DataMap = {

    {Data::Nothing, "nothing"},     {Data::Organisation, "organisation"},
    {Data::Vehicle, "vehicle"},     {Data::VehicleEquipment, "vequipment"},
    {Data::Equipment, "equipment"}, {Data::Facility, "facility"},
    {Data::Building, "building"}

};

UfopaediaEntry::UfopaediaEntry() : data_type(Data::Nothing) {}

bool UfopaediaEntry::isVisible() const { return this->dependency.satisfied(); }

} // namespace OpenApoc

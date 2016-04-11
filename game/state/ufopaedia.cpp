#include "game/state/ufopaedia.h"
#include "game/state/research.h"

namespace OpenApoc
{

const std::map<UfopaediaEntry::Data, UString> UfopaediaEntry::DataMap = {

    {Data::None, "none"},           {Data::Organisation, "organisation"},
    {Data::Vehicle, "vehicle"},     {Data::VehicleEquipment, "vequipment"},
    {Data::Equipment, "equipment"}, {Data::Facility, "facility"},
    {Data::Building, "building"}

};

UfopaediaEntry::UfopaediaEntry() : data_type(Data::None) {}

bool UfopaediaEntry::isVisible() const
{
	// No required research = always visible
	if (!this->required_research)
	{
		return true;
	}
	// Otherwise only visible if the research is complete
	return (this->required_research->isComplete());
}

} // namespace OpenApoc

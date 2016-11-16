#include "game/state/ufopaedia.h"
#include "game/state/gamestate.h"
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

template <>
sp<UfopaediaEntry> StateObject<UfopaediaEntry>::get(const GameState &state, const UString &id)
{
	for (auto &cat : state.ufopaedia)
	{
		auto entry = cat.second->entries.find(id);
		if (entry != cat.second->entries.end())
			return entry->second;
	}
	LogError("No UFOPaedia entry matching ID \"%s\"", id);
	return nullptr;
}

template <> const UString &StateObject<UfopaediaEntry>::getPrefix()
{
	static UString prefix = "PAEDIAENTRY_";
	return prefix;
}
template <> const UString &StateObject<UfopaediaEntry>::getTypeName()
{
	static UString name = "UfopaediaEntry";
	return name;
}

} // namespace OpenApoc

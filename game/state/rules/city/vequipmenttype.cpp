#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/gamestate.h"
#include "game/state/tilemap/tilemap.h"

namespace OpenApoc
{

template <> const UString &StateObject<VEquipmentType>::getPrefix()
{
	static UString prefix = "VEQUIPMENTTYPE_";
	return prefix;
}

template <> const UString &StateObject<VEquipmentType>::getTypeName()
{
	static UString name = "VEquipmentType";
	return name;
}

template <>
sp<VEquipmentType> StateObject<VEquipmentType>::get(const GameState &state, const UString &id)
{
	auto it = state.vehicle_equipment.find(id);
	if (it == state.vehicle_equipment.end())
	{
		LogError("No vequipement type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

// note: the range value in vanilla game files is given in half-metres
int VEquipmentType::getRangeInTiles() const { return range / 2 / (int)VELOCITY_SCALE_CITY.x; }
int VEquipmentType::getRangeInMetres() const { return range / 2; }

} // namespace OpenApoc

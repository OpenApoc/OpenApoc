#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/gamestate.h"
#include "game/state/tilemap/tilemap.h"

namespace OpenApoc
{

// A bit painful but as not everything is expected to be set we have to zero all the non-constructed
// types
VEquipmentType::VEquipmentType()
    : type(EquipmentSlotType::VehicleGeneral), weight(0), max_ammo(0), store_space(0), speed(0),
      damage(0), accuracy(0), fire_delay(0), tail_size(0), guided(false), turn_rate(0), range(0),
      firing_arc_1(0), firing_arc_2(0), point_defence(false), explosion_graphic(0), power(0),
      top_speed(0), accuracy_modifier(0), cargo_space(0), passengers(0), alien_space(0),
      missile_jamming(0), shielding(0), cloaking(false), teleporting(false){};

const UString &VEquipmentType::getPrefix()
{
	static UString prefix = "VEQUIPMENTTYPE_";
	return prefix;
}

const UString &VEquipmentType::getTypeName()
{
	static UString name = "VEquipmentType";
	return name;
}

sp<VEquipmentType> VEquipmentType::get(const GameState &state, const UString &id)
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

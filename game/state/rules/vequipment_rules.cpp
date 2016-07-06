#include "game/state/rules/vequipment_type.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

const std::map<VEquipmentType::Type, UString> VEquipmentType::TypeMap = {
    {Type::Engine, "engine"}, {Type::Weapon, "weapon"}, {Type::General, "general"},
};

const std::map<VEquipmentType::User, UString> VEquipmentType::UserMap = {
    {User::Ground, "ground"}, {User::Air, "air"},
};

// A bit painful but as not everything is expected to be set we have to zero all the non-constructed
// types
VEquipmentType::VEquipmentType()
    : type(Type::General), weight(0), max_ammo(0), store_space(0), speed(0), damage(0), accuracy(0),
      fire_delay(0), tail_size(0), guided(false), turn_rate(0), range(0), firing_arc_1(0),
      firing_arc_2(0), point_defence(false), explosion_graphic(0), power(0), top_speed(0),
      accuracy_modifier(0), cargo_space(0), passengers(0), alien_space(0), missile_jamming(0),
      shielding(0), cloaking(false), teleporting(false){};

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
		LogError("No vequipement type matching ID \"%s\"", id.c_str());
		return nullptr;
	}
	return it->second;
}

} // namespace OpenApoc

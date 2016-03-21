#include "game/rules/rules.h"
#include "game/rules/rules_private.h"
#include "game/rules/rules_helper.h"
#include "game/rules/vequipment.h"
#include "framework/framework.h"

namespace OpenApoc
{

const std::map<VEquipmentType::Type, UString> VEquipmentType::TypeMap = {
    {VEquipmentType::Type::Engine, "engine"},
    {VEquipmentType::Type::Weapon, "weapon"},
    {VEquipmentType::Type::General, "general"},
};

const std::map<VEquipmentType::User, UString> VEquipmentType::UserMap = {
    {VEquipmentType::User::Ground, "ground"}, {VEquipmentType::User::Air, "air"},
};

// A bit painful but as not everything is expected to be set we have to zero all the non-constructed
// types
VEquipmentType::VEquipmentType()
    : weight(0), max_ammo(0), store_space(0), speed(0), projectile_image(0), damage(0), accuracy(0),
      fire_delay(0), tail_size(0), guided(false), turn_rate(0), range(0), firing_arc_1(0),
      firing_arc_2(0), point_defence(false), power(0), top_speed(0), accuracy_modifier(0),
      cargo_space(0), passengers(0), alien_space(0), missile_jamming(0), shielding(0),
      cloaking(false), teleporting(false){};

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

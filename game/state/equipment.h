#pragma once

#include "library/rect.h"

namespace OpenApoc
{

enum class EquipmentSlotType
{
	General,
	ArmorBody,
	ArmorLegs,
	ArmorHelmet,
	ArmorLeftHand,
	ArmorRightHand,
	LeftHand,
	RightHand,
	VehicleGeneral,
	VehicleWeapon,
	VehicleEngine,
};

static inline constexpr bool isVehicleEquipmentSlot(EquipmentSlotType type)
{
	return (type == EquipmentSlotType::VehicleGeneral || type == EquipmentSlotType::VehicleWeapon ||
	        type == EquipmentSlotType::VehicleEngine);
}

static inline constexpr bool isAgentEquipmentSlot(EquipmentSlotType type)
{
	return (!isVehicleEquipmentSlot(type));
}

static inline constexpr bool isArmorEquipmentSlot(EquipmentSlotType type)
{
	return (type == EquipmentSlotType::ArmorBody || type == EquipmentSlotType::ArmorLegs ||
	        type == EquipmentSlotType::ArmorHelmet || type == EquipmentSlotType::ArmorLeftHand ||
	        type == EquipmentSlotType::ArmorRightHand);
}

enum class AlignmentX
{
	Left,
	Right,
	Centre,
};

enum class AlignmentY
{
	Top,
	Bottom,
	Centre,
};

class EquipmentLayoutSlot
{
  public:
	EquipmentSlotType type = EquipmentSlotType::General;
	AlignmentX align_x = AlignmentX::Left;
	AlignmentY align_y = AlignmentY::Top;
	Rect<int> bounds;
	EquipmentLayoutSlot() = default;
	EquipmentLayoutSlot(AlignmentX align_x, AlignmentY align_y, Rect<int> bounds)
	    : align_x(align_x), align_y(align_y), bounds(bounds)
	{
	}
};
} // namespace OpenApoc

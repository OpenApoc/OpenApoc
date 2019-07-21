#pragma once

#include "library/rect.h"
#include "library/sp.h"
#include <list>

namespace OpenApoc
{

class Image;

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

class Equipment
{
  public:
	virtual ~Equipment() = default;
	virtual sp<Image> getEquipmentArmorImage() const = 0;
	virtual sp<Image> getEquipmentImage() const = 0;
	// Returns the size in 'slots'
	virtual Vec2<int> getEquipmentSlotSize() const = 0;
};

class EquippableObject
{
  public:
	virtual ~EquippableObject() = default;
	virtual bool drawLines() const { return true; }
	virtual sp<Equipment> getEquipmentAt(const Vec2<int> &position) const = 0;
	virtual const std::list<EquipmentLayoutSlot> &getSlots() const = 0;
	// Returns a list of (position, equipment). The position is the single 'owning' slot position
	// for equipment that spans over multiple slots.
	virtual std::list<std::pair<Vec2<int>, sp<Equipment>>> getEquipment() const = 0;
};

} // namespace OpenApoc

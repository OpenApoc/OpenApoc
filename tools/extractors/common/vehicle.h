#pragma once
#include <cstdint>
struct VehicleData
{
	uint16_t manufacturer;
	uint16_t movement_type;
	uint16_t animation_type;
	uint16_t size_x;
	uint16_t size_y;
	uint16_t size_z;
	uint16_t image_position_1;
	uint16_t image_position_2;
	uint16_t image_position_3;
	uint16_t image_position_4;
	uint16_t graphic_frame;
	uint16_t acceleration;
	/* All dimension-capable craft have a non-zero value, I don't know what the values (1-8) then
	 * mean... */
	uint16_t dimension_travel;
	uint16_t top_speed;
	uint16_t shadow_graphic;
	uint16_t shadow_position_1;
	uint16_t shadow_position_2;
	uint16_t shadow_position_3;
	uint16_t shadow_position_4;
	// I /think/ the collision model is 'loftemps_height' stacked 'loftemps_index'
	// - which explains why it is thought they're related to 'where the guns fire from'
	// if the 'height' is used in that, as well as the 'evade chance' - as smaller volumes
	// are harder to hit
	uint16_t loftemps_height;
	uint16_t loftemps_index;
	uint16_t unknown2;
	uint16_t unknown3;
	uint16_t constitution;
	uint16_t crash_constitution;
	uint16_t weight;
	uint16_t armour_rear;
	uint16_t armour_top;
	uint16_t armour_right;
	uint16_t armour_left;
	uint16_t armour_bottom;
	uint16_t armour_front;
	uint16_t passenger_capacity;
	uint16_t aggressiveness;
	uint16_t score;
	uint8_t equipment_layout;
	char equipment_screen_name[8];
	uint8_t unknown4;
	uint8_t unknown5;
	uint8_t loaded_equipment_slots[45];
};

static_assert(sizeof(struct VehicleData) == 126, "Invalid vehicle_data size");

struct VehicleEquipmentLayoutSlot
{
	uint8_t position_x;
	uint8_t position_y;
	uint8_t size_x;
	uint8_t size_y;
#define VEHICLE_EQUIPMENT_LAYOUT_SLOT_TYPE_ENGINE 0x00
#define VEHICLE_EQUIPMENT_LAYOUT_SLOT_TYPE_WEAPON 0x01
#define VEHICLE_EQUIPMENT_LAYOUT_SLOT_TYPE_GENERAL 0x02
	uint8_t type;
	uint8_t no_partial_equipment; // '1' if equipment must be fully contained within this slot (All
	                              // weapons & engines)
	                              // '0' if a single equipment object can
	                              // overlap multiple slots (All 'general' equipment)
#define VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_LEFT 0x00
#define VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_CENTRE 0x01
#define VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_RIGHT 0x02
	uint8_t alignment_x;
#define VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_TOP 0x00
//#define VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_CENTRE 0x01 same as above
#define VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_BOTTOM 0x02
	uint8_t alignment_y;
};
static_assert(sizeof(struct VehicleEquipmentLayoutSlot) == 8,
              "invalid VehicleEquipmentLayoutSlot size");

struct VehicleEquipmentLayout
{
	uint16_t slot_count;
	VehicleEquipmentLayoutSlot slots[45];
};

static_assert(sizeof(struct VehicleEquipmentLayout) == 362, "invalid VehicleEquipmentLayout size");

#define NUM_VEHICLES 33

#define VEHICLE_DATA_OFFSET_START 0x189C8C
#define VEHICLE_DATA_OFFSET_END                                                                    \
	(VEHICLE_DATA_OFFSET_START + NUM_VEHICLES * sizeof(struct VehicleData))

#define VEHICLE_NAME_STRTAB_OFFSET_START 0x1495A2
#define VEHICLE_NAME_STRTAB_OFFSET_END 0x14976E

#define VEHICLE_EQUIPMENT_LAYOUT_OFFSET_START 1620416
#define VEHICLE_EQUIPMENT_LAYOUT_OFFSET_END 1629104

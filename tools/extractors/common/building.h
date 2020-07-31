#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct BldFileEntry
{
	uint16_t name_idx; // This index is matched against building name table
	uint16_t x0;
	uint16_t x1;
	uint16_t y0;
	uint16_t y1;
	// 0x0A
	uint16_t unknown1[80]; // People tube data populated in savegame
	// 0xAA
	uint16_t function_idx; // Specifies building function (aka type)
	uint16_t unknown2[5];
	// 0xB6
	uint8_t is_purchaseable;
	uint8_t unknown3_flag;
	uint16_t unknown4;
	// 0xBA
	uint16_t price; // Savegame value, divided by 2000
	uint16_t alien_detection_unknown;
	uint16_t maintenance_costs;
	uint16_t maximum_workforce;
	uint16_t current_workforce;
	uint16_t current_wage;
	uint16_t income_per_capita;
	// 0xC8
	uint16_t owner_idx;
	uint16_t unknown5;
	uint8_t investment_value;
	uint8_t respect_value;
	uint16_t unknown6[3];
	// 0xD4
	uint8_t alien_count[14];
};
#pragma pack(pop)

static_assert(sizeof(struct BldFileEntry) == 226, "Unexpected bld_file_entry size");

#pragma pack(push, 1)
struct BuildingCostData
{
	uint16_t cost;
	uint16_t workers;
	uint16_t income;
	uint16_t agentSpawnType;
	uint16_t investmentValue;
	uint16_t respectValue;
};
#pragma pack(pop)

static_assert(sizeof(struct BuildingCostData) == 12, "Unexpected bld_cost_struc size");

#define BUILDING_COST_STRUCT_OFFSET_START 0x1405C0
#define BUILDING_COST_STRUCT_OFFSET_END 0x14080C

#define BUILDING_NAME_STRTAB_OFFSET_START 0x149DD2
#define BUILDING_NAME_STRTAB_OFFSET_END 0x14A66A

#define BUILDING_FUNCTION_STRTAB_OFFSET_START 0x14AB46
#define BUILDING_FUNCTION_STRTAB_OFFSET_END 0x14ADCB

#define ALIEN_BUILDING_NAME_STRTAB_OFFSET_START 0x14AD23
#define ALIEN_BUILDING_NAME_STRTAB_OFFSET_END 0x14ADCB

struct BuildingInfiltrationSpeed
{
	int32_t speed;
};
static_assert(sizeof(struct BuildingInfiltrationSpeed) == 4, "Invalid OrgInfiltrationSpeed size");
#define BUILDING_INFILTRATION_SPEED_OFFSET_START 1319916
#define BUILDING_INFILTRATION_SPEED_OFFSET_END 1320112
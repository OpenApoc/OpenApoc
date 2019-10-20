#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct BldFileEntry
{
	// This value makes no sense
	uint16_t name_idx;
	uint16_t x0;
	uint16_t x1;
	uint16_t y0;
	uint16_t y1;
	uint16_t unknown1[80];
	// Specifies both function and name idx
	uint16_t function_idx;
	uint16_t unknown2[14];
	uint16_t owner_idx;
	uint16_t unknown3[12];
};
#pragma pack(pop)

static_assert(sizeof(struct BldFileEntry) == 226, "Unexpected bld_file_entry size");

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
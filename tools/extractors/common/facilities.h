#pragma once
#include <cstdint>

// FIXME: It's possible this entire struct should be shifted 2/4 bytes up (IE one/both of
// unknown3/4 actually belong to the 'next' struct)

struct FacilityData
{
	uint32_t cost;
	uint8_t image_offset;
	uint8_t size;
	uint16_t build_time;
	uint16_t maintainance_cost;
	uint16_t capacity;
	uint16_t unknown1;
	uint16_t unknown2;
};

static_assert(sizeof(struct FacilityData) == 16, "Invalid facility_data size");

#define FACILITY_DATA_OFFSET_START 0x13D794
#define FACILITY_DATA_OFFSET_END 0x13D8D4

#define FACILITY_STRTAB_OFFSET_START 0x14A66C
#define FACILITY_STRTAB_OFFSET_END 0x14A7A7

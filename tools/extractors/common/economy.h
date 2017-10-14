#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct EconomyData
{
	uint16_t week;
	uint32_t basePrice;
	uint16_t minStock;
	uint16_t maxStock;
	uint32_t curPrice;
	uint32_t curStock;
	uint32_t lastStock;
};
#pragma pack(pop)

static_assert(sizeof(struct EconomyData) == 22, "Invalid EconomyData size");

#define ECONOMY_DATA1_OFFSET_START 1629104
#define ECONOMY_DATA1_OFFSET_END 1630930

#define ECONOMY_DATA2_OFFSET_START 1630932
#define ECONOMY_DATA2_OFFSET_END 1631262

#define ECONOMY_DATA3_OFFSET_START 1632232
#define ECONOMY_DATA3_OFFSET_END 1634146

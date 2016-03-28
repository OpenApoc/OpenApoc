#pragma once
#include <cstdint>

struct scenery_minimap_colour_t
{
	uint8_t palette_index;
};

static_assert(sizeof(struct scenery_minimap_colour_t) == 1, "Invalid scenery_minimap_colour_t size");

#define SCENERY_MINIMAP_COLOUR_DATA_OFFSET_START 0x13D8D4 
#define SCENERY_MINIMAP_COLOUR_DATA_OFFSET_END 0x13DC91

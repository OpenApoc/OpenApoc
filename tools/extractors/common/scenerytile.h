#pragma once
#include <cstdint>

struct SceneryMinimapColour
{
	uint8_t palette_index;
};

static_assert(sizeof(struct SceneryMinimapColour) == 1, "Invalid SceneryMinimapColour size");

#define SCENERY_MINIMAP_COLOUR_DATA_OFFSET_START 0x13D8D4
#define SCENERY_MINIMAP_COLOUR_DATA_OFFSET_END 0x13DC91

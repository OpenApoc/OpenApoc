#pragma once

#include <cstdint>
struct BaseLayoutData
{
	char module[8][8];
};

static_assert(sizeof(struct BaseLayoutData) == 64, "Invalid baselayout_data size");

#define NUM_BASELAYOUT_ENTRIES 16
#define BASELAYOUT_OFFSET_START 0x14258C
#define BASELAYOUT_OFFSET_END                                                                      \
	(BASELAYOUT_OFFSET_START + NUM_BASELAYOUT_ENTRIES * sizeof(struct BaseLayoutData))

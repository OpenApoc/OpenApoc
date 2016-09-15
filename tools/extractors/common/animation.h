#pragma once
#include <cstdint>
#include "tools/extractors/common/animation.h"

struct AnimationDataAD
{
	uint16_t offset;
};
static_assert(sizeof(struct AnimationDataAD) == 2, "Invalid AnimationDataAD size");

struct AnimationDataUA
{
	uint16_t offset;
	uint8_t frame_count;
	uint8_t unknown;
};
static_assert(sizeof(struct AnimationDataUA) == 4, "Invalid AnimationDataUA size");

struct AnimationEntryUF
{
	uint16_t frame_idx;
	uint8_t x_offset;
	uint8_t y_offset;
};
static_assert(sizeof(struct AnimationEntryUF) == 4, "Invalid AnimationEntryUF size");

#pragma pack(push, 1)
struct AnimationDataUF
{
	AnimationEntryUF parts[7];
	uint8_t draw_order[7];
	uint8_t unknown;
};
#pragma pack(pop)
static_assert(sizeof(struct AnimationDataUF) == 36, "Invalid AnimationDataUF size");
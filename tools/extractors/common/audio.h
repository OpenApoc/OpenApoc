#pragma once

#include <cstdint>
struct RawSoundData
{
	char filename[46];
	char unknown1[4];
	uint16_t samplerate;
	char unknown2[12];
};

static_assert(sizeof(struct RawSoundData) == 64, "Invalid rawsound_data size");

#define NUM_RAWSOUND_ENTRIES 215
#define RAWSOUND_OFFSET_START 0x19337C
#define RAWSOUND_OFFSET_END                                                                        \
	(RAWSOUND_OFFSET_START + NUM_RAWSOUND_ENTRIES * sizeof(struct RawSoundData))

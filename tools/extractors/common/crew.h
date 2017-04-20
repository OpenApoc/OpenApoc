#pragma once

#include <cstdint>

// These are offsets into UFO2P.EXE

#define CREW_UFO_DOWNED_OFFSET_START 1303904
#define CREW_UFO_DOWNED_OFFSET_END 1304464
#define CREW_UFO_DEPOSIT_OFFSET_START 1304464
#define CREW_UFO_DEPOSIT_OFFSET_END 1305024
#define CREW_ALIEN_BUILDING_OFFSET_START 1645476
#define CREW_ALIEN_BUILDING_OFFSET_END 1646088

struct CrewData
{
	uint32_t alien_egg;
	uint32_t brainsucker;
	uint32_t multiworm;
	uint32_t hyperworm;
	uint32_t crysalis;
	uint32_t anthropod;
	uint32_t skeletoid;
	uint32_t spitter;
	uint32_t popper;
	uint32_t megaspawn;
	uint32_t psimorph;
	uint32_t queenspawn;
	uint32_t micronoid;
	uint32_t brainsucker_pod_unused;
};
static_assert(sizeof(struct CrewData) == 56, "Invalid crew_data size");

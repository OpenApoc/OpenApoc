#pragma once

#include <cstdint>

struct ResearchData
{
	uint8_t labSize; // 0 = small, 1 = large
	uint8_t unknown1;
	uint8_t unknown2;
	uint8_t
	    prereqType; // 0 = craft equipment, 1 = agent equipment, 3 = alien life form, 0xff = nune
	uint16_t unknown3;
	uint16_t prereq;
	uint16_t leadsTo1;
	uint16_t leadsTo2;
	uint16_t prereqTech[3]; // IDX into research list, 0xffff for none
	uint16_t score;
	uint32_t skillHours;
	uint8_t researchGroup; // 0 = BioChem, 1 = Quantum Phys
	uint8_t ufopaediaGroup;
	uint16_t ufopaediaEntry;
};

static_assert(sizeof(struct ResearchData) == 28, "Invalid research_data size");

#define RESEARCH_DATA_OFFSET_START 0x13EE80
#define RESEARCH_DATA_OFFSET_END 0x13F954
#define RESEARCH_NAME_STRTAB_OFFSET_START 0x14E3BA
#define RESEARCH_NAME_STRTAB_OFFSET_END 0x14EA20
#define RESEARCH_DESCRIPTION_STRTAB_OFFSET_START 0x14EA22
#define RESEARCH_DESCRIPTION_STRTAB_OFFSET_END 0x1501F1

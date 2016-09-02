#pragma once

#include <cstdint>

#define AGENT_EQUIPMENT_TYPE_ARMOR 0x00
#define AGENT_EQUIPMENT_TYPE_WEAPON 0x01
#define AGENT_EQUIPMENT_TYPE_GENERAL 0x02
#define AGENT_EQUIPMENT_TYPE_EMPTY 0x04

// These are offsets into UFO2P.EXE because equipment names there are better formatted than those in
// TACP.EXE
#define AGENT_EQUIPMENT_NAMES_OFFSET_START 1349488
#define AGENT_EQUIPMENT_NAMES_OFFSET_END 1351088

// Everything else in this file is an offset into TACP.EXE
#define DAMAGE_TYPE_NAMES_OFFSET_START 3024553
#define DAMAGE_TYPE_NAMES_OFFSET_END 3024569

struct AgentEquipmentData
{
	uint16_t score;
	uint8_t type;     // AGENT_EQUIPMENT_TYPE_*
	uint8_t data_idx; // Index into Agent***Data
	uint16_t weight;
	uint16_t sprite_idx;
	uint8_t size_x;
	uint8_t size_y;
	uint16_t manufacturer;
	uint8_t store_space;
	uint8_t armor;
	uint8_t unknown01;
	uint8_t unknown02;
	uint16_t unused01;
};

// SCR <= == T	IDX	WT <= == PIC <= == W	H	ORG <= == STOR	ARM ? ? ? <= ==

static_assert(sizeof(struct AgentEquipmentData) == 18, "Invalid agent_equipment_data size");
#define AGENT_EQUIPMENT_DATA_OFFSET_START 3154678
#define AGENT_EQUIPMENT_DATA_OFFSET_END 3156244

/*
Notes about figuring out data location in the exe

there is nowhere to read damage modifier names from

3149424
3151264
l 92
c 20
built-in equipment sets

3156876
3157996
l 140
c 8
score alien equipment sets
3157996
3159676
l 140
c 12
tech level human equipment sets

^^^ these are stored in a transponed order. first, every weapon#1 is stored for score 1,2,3,...8,
then weapon#2 for score 1,2,3...8, etc. to 10, then grenade 1-10, then equipment 1-10
weapon has 3 16bit values, grenade and equipment 2 16bit values. same way for humans.

2105504
2105520
l 1
c ?
Damage type shield bypass data (up to entropy enzyme only?)

3144440
3149422
l 106
c 37
Unit stats

3151452
3152280
l 36
c 23
Damage mods (stored in transponed order, and up to gun emplacement only?)

3152280
3153952
l 38
c 44
Payload information (power speed etc.)

3153952
3154102
l 10
c 15
Armor

3154102
3154678
l 16
c 36
Weapons and Grenades

3154678
3156244
l 18
c 87
AgentEquipmentData (MAIN)

3156244
3156736
l 12
c 41
General (Ammo and Gadgets)

3156736
3156876
l 4
c 35 (7x5)
Score Requirement

*/
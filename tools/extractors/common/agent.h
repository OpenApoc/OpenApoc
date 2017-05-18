#pragma once

#include <cstdint>

#define AGENT_MOVEMENT_TYPE_STATIONARY 0x00
#define AGENT_MOVEMENT_TYPE_STANDART 0x01
#define AGENT_MOVEMENT_TYPE_FLYING 0x03
#define AGENT_MOVEMENT_TYPE_STANDART_LARGE 0x05
#define AGENT_MOVEMENT_TYPE_FLYING_LARGE 0x07

// These are offsets into UFO2P.EXE
#define AGENT_TYPE_NAMES_OFFSET_START 1391631
#define AGENT_TYPE_NAMES_OFFSET_END 1392288

struct AgentTypeData
{
	uint16_t score;
	uint8_t speed_base;
	uint8_t speed_inc;
	uint16_t health_base;
	uint16_t health_inc;
	uint8_t unknown1_base;
	uint8_t unknown1_inc;
	uint8_t stamina_base;
	uint8_t stamina_inc;
	uint8_t reactions_base;
	uint8_t reactions_inc;
	uint8_t strength_base;
	uint8_t strength_inc;
	uint8_t bravery_base;
	uint8_t bravery_inc;
	uint8_t psi_energy_base;
	uint8_t psi_energy_inc;
	uint8_t psi_attack_base;
	uint8_t psi_attack_inc;
	uint8_t psi_defense_base;
	uint8_t psi_defense_inc;
	uint8_t unknown2_base;
	uint8_t unknown2_inc;
	uint8_t accuracy_base;
	uint8_t accuracy_inc;
	uint8_t unknown3_base;
	uint8_t unknown3_inc;
	uint8_t unknown4_base;
	uint8_t unknown4_inc;
	uint8_t biochemistry_base;
	uint8_t biochemistry_inc;
	uint8_t quantum_physics_base;
	uint8_t quantum_physics_inc;
	uint8_t engineering_base;
	uint8_t engineering_inc;
	uint8_t unknown5[22];
	uint16_t loftemps_height;
	uint16_t loftemps_idx;
	uint16_t unknown6;
	uint8_t movement_type;
	uint8_t unknown7;
	uint8_t inventory;
	uint8_t unknown8;
	uint16_t armor_head;
	uint16_t armor_body;
	uint16_t armor_right;
	uint16_t armor_left;
	uint16_t armor_leg;
	uint16_t armor_unknown;
	uint8_t unknown9;
	uint8_t equipment_sets[5];
	uint16_t damage_modifier;
	uint16_t unknown10[3];
	uint16_t image_position[4];
	uint16_t image;
};
static_assert(sizeof(struct AgentTypeData) == 106, "Invalid agent_type size");

#define AGENT_TYPE_DATA_OFFSET_START 1398132
#define AGENT_TYPE_DATA_OFFSET_END 1403114

struct AgentInfiltrationSpeed
{
	int32_t speed;
};
static_assert(sizeof(struct AgentInfiltrationSpeed) == 4, "Invalid OrgInfiltrationSpeed size");
#define AGENT_INFILTRATION_SPEED_OFFSET_START 1397788
#define AGENT_INFILTRATION_SPEED_OFFSET_END 1397844

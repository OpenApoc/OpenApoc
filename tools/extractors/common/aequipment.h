#pragma once

#include <cstdint>

#define AGENT_EQUIPMENT_TYPE_ARMOR 0x00
#define AGENT_EQUIPMENT_TYPE_WEAPON 0x01
#define AGENT_EQUIPMENT_TYPE_GENERAL 0x02
#define AGENT_EQUIPMENT_TYPE_EMPTY 0x04

#define AGENT_ARMOR_BODY_PART_LEGS 0x00
#define AGENT_ARMOR_BODY_PART_BODY 0x01
#define AGENT_ARMOR_BODY_PART_LEFT_ARM 0x02
#define AGENT_ARMOR_BODY_PART_RIGHT_ARM 0x03
#define AGENT_ARMOR_BODY_PART_HELMET 0x04

#define AGENT_GENERAL_TYPE_AMMO_OR_LOOT 0x00
#define AGENT_GENERAL_TYPE_MOTION_SCANNER 0x01
#define AGENT_GENERAL_TYPE_STRUCTURE_PROBE 0x02
#define AGENT_GENERAL_TYPE_VORTEX_ANALYZER 0x03
#define AGENT_GENERAL_TYPE_MULTI_TRACKER 0x04
#define AGENT_GENERAL_TYPE_MIND_SHIELD 0x05
#define AGENT_GENERAL_TYPE_MIND_BENDER 0x06
#define AGENT_GENERAL_TYPE_ALIEN_DETECTOR 0x07
#define AGENT_GENERAL_TYPE_DISRUPTOR_SHIELD 0x08
#define AGENT_GENERAL_TYPE_TELEPORTER 0x09
#define AGENT_GENERAL_TYPE_CLOAKING_FIELD 0x0a
#define AGENT_GENERAL_TYPE_DIMENSION_FORCE_FIELD 0x0b
#define AGENT_GENERAL_TYPE_MEDI_KIT 0x0c

#define AGENT_GRENADE_TRIGGER_TYPE_NORMAL 0xffff
#define AGENT_GRENADE_TRIGGER_TYPE_PROXIMITY 0x01
#define AGENT_GRENADE_TRIGGER_TYPE_BOOMEROID 0x02

// These are offsets into UFO2P.EXE because equipment names there are better formatted
#define AGENT_EQUIPMENT_NAMES_OFFSET_START 1349488
#define AGENT_EQUIPMENT_NAMES_OFFSET_END 1351088

// Everything else in this file is an offset into TACP.EXE
#define DAMAGE_TYPE_NAMES_OFFSET_START 3024353
#define DAMAGE_TYPE_NAMES_OFFSET_END 3024569

struct DamageTypeData
{
	uint8_t ignore_shield;
};
static_assert(sizeof(struct DamageTypeData) == 1, "Invalid damage_type_data size");
#define DAMAGE_TYPE_DATA_OFFSET_START 2105504
#define DAMAGE_TYPE_DATA_OFFSET_END 2105520

struct DamageModifierData
{
	uint16_t damage_type_data[18];
};
static_assert(sizeof(struct DamageModifierData) == 36, "Invalid damage_modifier_data size");
#define DAMAGE_MODIFIER_DATA_OFFSET_START 3151452
#define DAMAGE_MODIFIER_DATA_OFFSET_END 3152280

#pragma pack(push, 1)
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
	uint16_t unused01;
	uint8_t artifact;
};
#pragma pack(pop)
static_assert(sizeof(struct AgentEquipmentData) == 18, "Invalid agent_equipment_data size");
#define AGENT_EQUIPMENT_DATA_OFFSET_START 3154678
#define AGENT_EQUIPMENT_DATA_OFFSET_END 3156244

struct AgentArmorData
{
	uint8_t unknown01;
	uint8_t unknown02;
	uint8_t unknown03;
	uint8_t body_part;
	uint8_t armor;
	uint8_t damage_modifier;
	uint8_t unknown04;
	uint8_t unknown05;
	uint8_t unknown06;
	uint8_t unknown07;
};
static_assert(sizeof(struct AgentArmorData) == 10, "Invalid agent_armor_data size");
#define AGENT_ARMOR_DATA_OFFSET_START 3153952
#define AGENT_ARMOR_DATA_OFFSET_END 3154102

struct AgentWeaponData
{
	uint8_t ammo_effect[3];
	uint8_t ammo_rounds[3];
	uint8_t ammo_recharge[3];
	uint8_t unknown01;
	uint8_t unknown02;
	uint8_t unknown03;
	uint16_t ammo_type;
	uint16_t grenade_effect;
};
static_assert(sizeof(struct AgentWeaponData) == 16, "Invalid agent_weapon_data size");
#define AGENT_WEAPON_DATA_OFFSET_START 3154102
#define AGENT_WEAPON_DATA_OFFSET_END 3154678

struct AgentGeneralData
{
	uint8_t ammo_effect;
	uint8_t ammo_rounds;
	uint8_t ammo_recharge;
	uint8_t unknown01;
	uint8_t unknown02;
	uint8_t unknown03;
	uint16_t ammo_type_duplicate;
	uint16_t ammo_type;
	uint16_t type;
};
static_assert(sizeof(struct AgentGeneralData) == 12, "Invalid agent_general_data size");
#define AGENT_GENERAL_DATA_OFFSET_START 3156244
#define AGENT_GENERAL_DATA_OFFSET_END 3156736

struct AgentPayloadData
{
	uint16_t speed;
	uint16_t projectile_image;
	uint16_t damage;
	uint16_t accuracy;
	uint16_t fire_delay;
	uint16_t unknown01;
	uint16_t guided;
	uint16_t turn_rate;
	uint16_t range;
	uint16_t ttl;
	uint16_t unknown02;
	uint16_t unknown03;
	uint16_t explosion_graphic; // FIXME: Find ptang lookup? (how many frames etc.)
	uint16_t unknown04;
	uint16_t fire_sfx;
	uint16_t impact_sfx;
	uint16_t damage_type;
	uint16_t trigger_type;
	uint16_t explosion_depletion_rate;
};
static_assert(sizeof(struct AgentPayloadData) == 38, "Invalid agent_payload_data size");
#define AGENT_PAYLOAD_DATA_OFFSET_START 3152280
#define AGENT_PAYLOAD_DATA_OFFSET_END 3153952

struct AgentEquipmentSetBuiltInDataWeapon
{
	uint32_t weapon_idx;
	uint32_t weapon_chance;
	uint32_t clip_idx;
	uint32_t clip_amount;
};
struct AgentEquipmentSetBuiltInDataItem
{
	uint32_t item_idx;
	uint32_t item_chance;
	uint32_t item_amount;
};
struct AgentEquipmentSetBuiltInData
{
	AgentEquipmentSetBuiltInDataWeapon weapons[2];
	AgentEquipmentSetBuiltInDataItem items[5];
};
static_assert(sizeof(struct AgentEquipmentSetBuiltInData) == 92,
              "Invalid agent_equipment_set_built_in size");
#define AGENT_EQUIPMENT_SET_BUILTIN_DATA_OFFSET_START 3149424
#define AGENT_EQUIPMENT_SET_BUILTIN_DATA_OFFSET_END 3151264

struct AgentEquipmentSetScoreDataWeapon
{
	uint16_t weapon_idx;
	uint16_t clip_idx;
	uint16_t clip_amount;
};
struct AgentEquipmentSetScoreDataGrenade
{
	uint16_t grenade_idx;
	uint16_t grenade_amount;
};
struct AgentEquipmentSetScoreDataAlien
{
	AgentEquipmentSetScoreDataWeapon weapons[10][8];
	AgentEquipmentSetScoreDataGrenade grenades[10][8];
	uint16_t equipment[10][8][2];
};
static_assert(sizeof(struct AgentEquipmentSetScoreDataAlien) == 140 * 8,
              "Invalid agent_equipment_set_score_alien size");
#define AGENT_EQUIPMENT_SET_SCORE_ALIEN_DATA_OFFSET_START 3156876
#define AGENT_EQUIPMENT_SET_SCORE_ALIEN_DATA_OFFSET_END 3157996

struct AgentEquipmentSetScoreDataHuman
{
	AgentEquipmentSetScoreDataWeapon weapons[10][12];
	AgentEquipmentSetScoreDataGrenade grenades[10][12];
	uint16_t equipment[10][12][2];
};
static_assert(sizeof(struct AgentEquipmentSetScoreDataHuman) == 140 * 12,
              "Invalid agent_equipment_set_score_human size");
#define AGENT_EQUIPMENT_SET_SCORE_HUMAN_DATA_OFFSET_START 3157996
#define AGENT_EQUIPMENT_SET_SCORE_HUMAN_DATA_OFFSET_END 3159676

struct AgentEquipmentSetScoreRequirement
{
	uint32_t score[5][7];
};
static_assert(sizeof(struct AgentEquipmentSetScoreRequirement) == 35 * 4,
              "Invalid agent_equipment_set_score_requirement size");
#define AGENT_EQUIPMENT_SET_SCORE_REQUIREMENT_DATA_OFFSET_START 3156736
#define AGENT_EQUIPMENT_SET_SCORE_REQUIREMENT_DATA_OFFSET_END 3156876
#pragma once

#include <cstdint>

#define SPAWN_TYPE_PLAYER 0x00
#define SPAWN_TYPE_ENEMY 0x01
#define SPAWN_TYPE_CIVILIAN 0x02

struct BuildingExitData
{
	uint32_t exits[15];
	uint32_t unused;
};
static_assert(sizeof(struct BuildingExitData) == 16 * 4, "Unexpected BuildingExitData size");

#pragma pack(push, 1)
struct BuildingDatStructure
{
	uint32_t chunk_x;
	uint32_t chunk_y;
	uint32_t chunk_z;
	uint32_t battle_x;
	uint32_t battle_y;
	uint32_t battle_z;
	uint8_t allow_entrance_from;
	uint8_t allow_exit_from;
	uint32_t entrance_min_level;
	uint32_t entrance_max_level;
	uint32_t exit_min_level;
	uint32_t exit_max_level;
	BuildingExitData exits[2][15];
	uint32_t destroyed_ground_idx;
};
#pragma pack(pop)
static_assert(sizeof(struct BuildingDatStructure) == 1966, "Unexpected BuildingDatStructure size");

struct SecSdtStructure
{
	uint32_t chunks_x;
	uint32_t chunks_y;
	uint32_t chunks_z;
	uint32_t occurrence_min;
	uint32_t occurrence_max;
};
static_assert(sizeof(struct SecSdtStructure) == 20, "Unexpected SecSdtStructure size");

struct LineOfSightData
{
	uint16_t unknown01;
	uint16_t begin_x;
	uint16_t begin_y;
	uint16_t begin_z;
	uint16_t end_x;
	uint16_t end_y;
	uint16_t end_z;
	uint8_t unknown02[116];
	uint8_t ai_patrol_priority;
	uint8_t ai_target_priority;
	uint8_t spawn_type;
	uint8_t spawn_priority;
	uint8_t spawn_large;
	uint8_t spawn_walkers;
};
static_assert(sizeof(struct LineOfSightData) == 136, "Unexpected LineOfSightData size");

struct LootLocationData
{
	uint16_t x;
	uint16_t y;
	uint16_t z;
	uint16_t priority;
};
static_assert(sizeof(struct LootLocationData) == 8, "Unexpected LootLocationData size");

struct SmpData
{
	uint8_t GD;
	uint8_t LW;
	uint8_t RW;
	uint8_t FT;
};
static_assert(sizeof(struct SmpData) == 4, "Unexpected SmpData size");

struct RubbleDatStructure
{
	uint8_t feature[5];
	uint8_t left_wall[5];
	uint8_t right_wall[5];
};
static_assert(sizeof(struct RubbleDatStructure) == 15, "Unexpected RubbleDatStructure size");

#pragma pack(push, 1)
struct BattleMapPartEntry
{
	uint16_t constitution;
	uint16_t explosion_power;
	uint16_t explosion_depletion_rate;
	uint16_t explosion_type;
	uint8_t fire_resist;
	uint8_t fire_burn_time;
	uint8_t fire_burn_intensity;
	uint8_t block_physical;
	uint8_t block_gas;
	uint8_t block_fire;
	uint8_t block_psionic;
	uint8_t loftemps_lof[20];
	uint8_t loftemps_los[20];
	uint8_t unused01[3];
	uint8_t size;
	uint8_t unused02;
	uint16_t damaged_idx;
	uint16_t animation_idx;
	uint8_t animation_length;
	uint8_t animation_autoloop;
	uint8_t transparent;
	uint8_t sfx;
	uint8_t is_door;
	uint8_t is_door_closed;
	uint8_t los_through_terrain;
	uint8_t is_floor;
	uint8_t is_gravlift;
	uint8_t unused03;
	uint16_t alternative_object_requirement;
	uint16_t alternative_object_idx;
	uint8_t alternative_object_type;
	uint8_t movement_cost;
	uint8_t is_climbable;
	uint8_t height;
	uint8_t is_floating;
	uint8_t provides_support;
	uint8_t gets_support_from;
	uint8_t independent_structure;
};
#pragma pack(pop)
static_assert(sizeof(struct BattleMapPartEntry) == 86, "Unexpected battlemap_entry size");

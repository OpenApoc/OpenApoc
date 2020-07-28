#pragma once
#include <cstdint>

namespace OpenApoc
{

#define ORGANISATION_NAME_STRTAB_OFFSET_START 1355537
#define ORGANISATION_NAME_STRTAB_OFFSET_END 1355822

#pragma pack(push, 1)
struct OrganisationData
{
	uint16_t organization_type;
	uint32_t raiding_strength;
	uint8_t starting_tech_level;
	uint8_t average_guards;
	uint32_t starting_funds;
	uint32_t starting_funding;
	uint16_t rebuilding_rate;
};
#pragma pack(pop)
static_assert(sizeof(struct OrganisationData) == 18, "Invalid organisation_data size");
// Income may then also be modified by a per-tile value based on owned buildings & their 'types'?
#define ORGANISATION_DATA_OFFSET_START 1315944
#define ORGANISATION_DATA_OFFSET_END 1316448

struct OrgRaidLootData
{
	uint32_t loot_idx[3][5];
};
static_assert(sizeof(struct OrgRaidLootData) == 15 * 4, "Invalid raid_loot_data size");
#define ORGANISATION_RAID_LOOT_DATA_OFFSET_START 1646596
#define ORGANISATION_RAID_LOOT_DATA_OFFSET_END 1648216

struct OrgStartingRelationshipsData
{
	int32_t relationships[28];
};
static_assert(sizeof(struct OrgStartingRelationshipsData) == 28 * 4,
              "Invalid OrgStartingRelationshipsData size");
#define ORGANISATION_STARTING_RELATIONSHIPS_DATA_OFFSET_START 1609792
#define ORGANISATION_STARTING_RELATIONSHIPS_DATA_OFFSET_END 1612928

struct OrgInfiltrationSpeed
{
	int32_t speed;
};
static_assert(sizeof(struct OrgInfiltrationSpeed) == 4, "Invalid OrgInfiltrationSpeed size");
#define ORGANISATION_INFILTRATION_SPEED_OFFSET_START 1320112
#define ORGANISATION_INFILTRATION_SPEED_OFFSET_END 1320224

struct OrgVehicleParkData
{
	uint32_t vehiclePark;
};

#define ORGANISATION_VEHICLE_PARK_DATA_OFFSET_START 1609384
#define ORGANISATION_VEHICLE_PARK_DATA_OFFSET_END 1609496

} // namespace OpenApoc

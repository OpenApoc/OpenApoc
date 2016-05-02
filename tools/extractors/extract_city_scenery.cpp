#include "framework/data.h"
#include "framework/framework.h"
#include "framework/palette.h"
#include "game/state/rules/scenery_tile_type.h"
#include "library/voxel.h"
#include "tools/extractors/extractors.h"

namespace OpenApoc
{

static UString SCENERY_MINIMAP_PALETTE = "xcom3/ufodata/base.pcx";

#define TILE_TYPE_GENERAL (0x00)
#define TILE_TYPE_ROAD (0x01)
#define TILE_TYPE_PEOPLE_TUBE_JUNCTION (0x02)
#define TILE_TYPE_PEOPLE_TUBE (0x03)
#define TILE_TYPE_CITY_WALL (0x04)

#define ROAD_TYPE_STRAIGHT_BEND (0x00) // Straight or bend
#define ROAD_TYPE_JUNCTION (0x01)      // T junction or crossroads
#define ROAD_TYPE_TERMINAL (0x02)      // End of a road - allows u-turns?

#define WALK_TYPE_NONE (0x00)
#define WALK_TYPE_INTO (0x01)
#define WALK_TYPE_ONTO (0x02)

#pragma pack(push, 1)
struct citymap_tile_entry
{
	uint8_t tile_type;            // TILE_TYPE_*
	uint8_t road_type;            // ROAD_TYPE_*
	uint8_t road_junction[4];     // 0 = not connect, 1 = is connected, in N E S W order
	uint8_t road_level_change[4]; // 0 = flat, 1 = up, 0xFF (-1) = down, in NESW order
	uint8_t height;               // 0-16
	uint8_t constitution;         // 0-255
	uint8_t value;                // 0-255
	uint8_t overlaytile_idx; // Offset of an 'overlay' sprite in ufodata/cityovr.pck - 0xff appears
	                         // to mean "none"
	uint8_t landing_pad;     // 1 == landing pad
	uint8_t walk_type;       // WALK_TYPE_*
	uint8_t voxelIdx[16];
	uint8_t has_basement;               // 0 = no basement, 1 = has basement
	uint8_t people_tube_connections[6]; // 0=not connected, 1 = connected, in N E S W Up Down order
	uint8_t mass;                       // 0-10
	uint8_t strength;                   // 0-100
	uint8_t unknown3;
	uint16_t damagedtile_idx;
	uint16_t supportile_idx;
	uint8_t unknown4[4];
	uint16_t stratmap_idx;
};
#pragma pack(pop)

static_assert(offsetof(struct citymap_tile_entry, stratmap_idx) == 50,
              "Invalid strategy map offset");

static_assert(offsetof(struct citymap_tile_entry, strength) == 0x28, "OFFSET");
static_assert(offsetof(struct citymap_tile_entry, damagedtile_idx) == 0x2A,
              "Invalid damaged tile index offset");
static_assert(offsetof(struct citymap_tile_entry, overlaytile_idx) == 0x0D,
              "Invalid overlay tile index offset");
static_assert(sizeof(struct citymap_tile_entry) == 52, "Unexpected citymap_tile_entry size");

void InitialGameStateExtractor::extractCityScenery(GameState &state, UString tilePrefix,
                                                   UString datFile, UString spriteFile,
                                                   UString stratmapFile, UString lofFile,
                                                   UString ovrFile, sp<City> city)
{
	auto &data = this->ufo2p;
	auto minimap_palette = fw().data->load_palette(SCENERY_MINIMAP_PALETTE);
	auto inFile = fw().data->fs.open("xcom3/ufodata/" + datFile + ".dat");
	if (!inFile)
	{
		LogError("Failed to open \"" + datFile + ".dat\"");
	}

	auto fileSize = inFile.size();

	auto tileCount = fileSize / sizeof(struct citymap_tile_entry);
	LogInfo("Loading %zu tile entries", tileCount);
	if (fileSize % sizeof(citymap_tile_entry))
	{
		LogWarning("filesize %zu doesn't divide by tile record size", fileSize);
	}

	for (unsigned i = 0; i < tileCount; i++)
	{
		struct citymap_tile_entry entry;
		inFile.read((char *)&entry, sizeof(entry));

		UString id =
		    UString::format("%s%s%u", SceneryTileType::getPrefix().c_str(), tilePrefix.c_str(), i);

		auto tile = mksp<SceneryTileType>();

		if (entry.damagedtile_idx)
		{
			tile->damagedTile = {&state,
			                     UString::format("%s%s%u", SceneryTileType::getPrefix().c_str(),
			                                     tilePrefix.c_str(), entry.damagedtile_idx)};
		}

		auto imageString = UString::format(
		    "PCK:xcom3/UFODATA/" + spriteFile + ".PCK:xcom3/UFODATA/" + spriteFile + ".TAB:%u", i);

		tile->sprite = fw().data->load_image(imageString);

		tile->voxelMap = mksp<VoxelMap>(Vec3<int>{32, 32, 16});

		for (unsigned z = 0; z < 16; z++)
		{
			auto lofString = UString::format("LOFTEMPS:xcom3/UFODATA/" + lofFile +
			                                     ".DAT:xcom3/UFODATA/" + lofFile + ".TAB:%d",
			                                 (int)entry.voxelIdx[z]);
			tile->voxelMap->slices[z] = fw().data->load_voxel_slice(lofString);
		}

		if (entry.stratmap_idx != 0)
		{
			auto stratmapString =
			    UString::format("PCKSTRAT:xcom3/UFODATA/" + stratmapFile + ".PCK:xcom3/UFODATA/" +
			                        stratmapFile + ".TAB:%d",
			                    (int)entry.stratmap_idx);
			tile->strategySprite = fw().data->load_image(stratmapString);
		}

		if (entry.landing_pad == 1)
		{
			tile->isLandingPad = true;
		}
		else
		{
			tile->isLandingPad = false;
		}

		if (entry.overlaytile_idx != 0xff)
		{
			auto overlayString = UString::format("PCK:xcom3/UFODATA/" + ovrFile +
			                                         ".PCK:xcom3/UFODATA/" + ovrFile + ".TAB:%d",
			                                     (int)entry.overlaytile_idx);
			tile->overlaySprite = fw().data->load_image(overlayString);
		}

		// FIXME: Only the 'human' city has a minimap visible from the base screen? So this doesn't
		// exist for the alien map?
		if (datFile == "citymap")
		{
			uint8_t palette_index = data.scenery_minimap_colour->get(i).palette_index;
			tile->minimap_colour = minimap_palette->GetColour(palette_index);
		}

		// FIXME: I /think/ all scenery tiles have an offset of {32,32} to the center {0.5, 0.5, 0}
		// point
		tile->imageOffset = {32, 32};

		city->tile_types[id] = tile;
	}
}

} // namespace OpenApoc

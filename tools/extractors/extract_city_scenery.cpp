#include "framework/data.h"
#include "framework/framework.h"
#include "framework/palette.h"
#include "game/state/city/city.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "library/strings_format.h"
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
                                                   UString ovrFile, sp<City> city) const
{
	auto &data = this->ufo2p;
	auto minimap_palette = fw().data->loadPalette(SCENERY_MINIMAP_PALETTE);
	bool alien = datFile == "alienmap";
	auto inFile = fw().data->fs.open("xcom3/ufodata/" + datFile + ".dat");
	if (!inFile)
	{
		LogError("Failed to open \"%s.dat\"", datFile);
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

		UString id = format("%s%s%u", SceneryTileType::getPrefix(), tilePrefix, i);

		auto tile = mksp<SceneryTileType>();

		tile->overlayHeight = entry.height;
		tile->height = entry.height;

		switch (entry.walk_type)
		{
			case WALK_TYPE_NONE:
				tile->walk_mode = SceneryTileType::WalkMode::None;
				break;
			case WALK_TYPE_INTO:
				tile->walk_mode = SceneryTileType::WalkMode::Into;
				tile->height = 0;
				break;
			case WALK_TYPE_ONTO:
				tile->walk_mode = SceneryTileType::WalkMode::Onto;
				break;
			default:
				LogError("Unexpected scenery walk type %d for ID %s", (int)entry.walk_type, id);
		}

		switch (entry.tile_type)
		{
			case TILE_TYPE_GENERAL:
				tile->tile_type = SceneryTileType::TileType::General;
				break;
			case TILE_TYPE_ROAD:
				tile->tile_type = SceneryTileType::TileType::Road;
				tile->height = 0;
				break;
			case TILE_TYPE_PEOPLE_TUBE_JUNCTION:
				tile->tile_type = SceneryTileType::TileType::PeopleTubeJunction;
				break;
			case TILE_TYPE_PEOPLE_TUBE:
				tile->tile_type = SceneryTileType::TileType::PeopleTube;
				break;
			case TILE_TYPE_CITY_WALL:
				tile->tile_type = SceneryTileType::TileType::CityWall;
				break;
			default:
				LogError("Unexpected scenery tile type %d for ID %s", (int)entry.tile_type, id);
		}

		switch (entry.road_type)
		{
			case ROAD_TYPE_STRAIGHT_BEND:
				tile->road_type = SceneryTileType::RoadType::StraightBend;
				break;
			case ROAD_TYPE_JUNCTION:
				tile->road_type = SceneryTileType::RoadType::Junction;
				break;
			case ROAD_TYPE_TERMINAL:
				tile->road_type = SceneryTileType::RoadType::Terminal;
				break;
			default:
				LogError("Unexpected scenery road type %d for ID %s", (int)entry.road_type, id);
		}

		tile->connection.resize(4);
		tile->hill.resize(4);
		tile->tube.resize(6);

		for (int i = 0; i < 4; i++)
		{
			tile->connection[i] = entry.road_junction[i];
		}
		for (int i = 0; i < 4; i++)
		{
			tile->hill[i] = entry.road_level_change[i];
		}
		for (int i = 0; i < 6; i++)
		{
			tile->tube[i] = entry.people_tube_connections[i];
		}

		if (alien && ((i >= 50 && i <= 58) || i == 61))
		{
			if (i == 58 || i == 61)
			{
				tile->tile_type = SceneryTileType::TileType::PeopleTubeJunction;
				tile->tube[5] = true;
			}
			else
			{
				tile->tile_type = SceneryTileType::TileType::PeopleTube;
			}
			std::set<int> north = {52, 53, 54, 57, 61};
			std::set<int> east = {50, 51, 56, 57, 58};
			std::set<int> south = {53, 54, 55, 56, 61};
			std::set<int> west = {50, 51, 52, 55, 58};
			if (north.find(i) != north.end())
			{
				tile->tube[0] = true;
			}
			if (east.find(i) != east.end())
			{
				tile->tube[1] = true;
			}
			if (south.find(i) != south.end())
			{
				tile->tube[2] = true;
			}
			if (west.find(i) != west.end())
			{
				tile->tube[3] = true;
			}
		}

		tile->constitution = entry.constitution;
		tile->strength = entry.strength;
		tile->mass = entry.mass;
		tile->value = entry.value;
		tile->basement = entry.has_basement > 0;

		for (unsigned i = 0; i < 4; i++)
		{
			if (entry.road_level_change[i] != 0)
			{
				tile->isHill = true;
				tile->overlayHeight = 8;
				tile->height = 8;
				break;
			}
		}
		if (tile->value > 5)
		{
			tile->isBuildingPart = true;
		}
		// Correct walk modes
		if (tile->height < 8 && tile->walk_mode == SceneryTileType::WalkMode::Onto)
		{
			tile->walk_mode = SceneryTileType::WalkMode::Into;
		}
		if (tile->height >= 8 && tile->walk_mode == SceneryTileType::WalkMode::Into)
		{
			tile->walk_mode = SceneryTileType::WalkMode::Onto;
		}
		// Common city property
		if (i < 134 || i == 230 || i == 777 || (i >= 169 && i <= 194) || i > 936)
		{
			tile->commonProperty = true;
		}
		// Incorrect walkmodes
		if (i >= 890 && i <= 908)
		{
			tile->walk_mode = SceneryTileType::WalkMode::None;
		}
		// Roads with overhead stuff
		if (i == 18 || i == 19 || i == 37 || i == 38)
		{
			tile->walk_mode = SceneryTileType::WalkMode::None;
		}
		if (entry.damagedtile_idx)
		{
			tile->damagedTile = {&state, format("%s%s%u", SceneryTileType::getPrefix(), tilePrefix,
			                                    entry.damagedtile_idx)};
		}

		auto imageString = format(
		    "PCK:xcom3/ufodata/" + spriteFile + ".pck:xcom3/ufodata/" + spriteFile + ".tab:%u", i);

		tile->sprite = fw().data->loadImage(imageString);

		tile->voxelMap = mksp<VoxelMap>(Vec3<int>{32, 32, 16});

		for (unsigned z = 0; z < 16; z++)
		{
			if (entry.voxelIdx[z] == 0)
				continue;
			auto lofString = format("LOFTEMPS:xcom3/ufodata/" + lofFile + ".dat:xcom3/ufodata/" +
			                            lofFile + ".tab:%d",
			                        (int)entry.voxelIdx[z]);
			tile->voxelMap->slices[z] = fw().data->loadVoxelSlice(lofString);
		}

		if (entry.stratmap_idx != 0)
		{
			auto stratmapString = format("PCKSTRAT:xcom3/ufodata/" + stratmapFile +
			                                 ".pck:xcom3/ufodata/" + stratmapFile + ".tab:%d",
			                             (int)entry.stratmap_idx);
			tile->strategySprite = fw().data->loadImage(stratmapString);
		}

		if (entry.landing_pad == 1)
		{
			tile->isLandingPad = true;
			tile->walk_mode = SceneryTileType::WalkMode::None;
		}
		else
		{
			tile->isLandingPad = false;
		}

		if (entry.overlaytile_idx != 0xff)
		{
			auto overlayString =
			    format("PCK:xcom3/ufodata/" + ovrFile + ".pck:xcom3/ufodata/" + ovrFile + ".tab:%d",
			           (int)entry.overlaytile_idx);
			tile->overlaySprite = fw().data->loadImage(overlayString);
		}

		// FIXME: Only the 'human' city has a minimap visible from the base screen? So this doesn't
		// exist for the alien map?
		if (datFile == "citymap")
		{
			uint8_t palette_index = data.scenery_minimap_colour->get(i).palette_index;
			tile->minimap_colour = minimap_palette->getColour(palette_index);
		}

		tile->imageOffset = CITY_IMAGE_OFFSET;

		city->tile_types[id] = tile;
	}
}

} // namespace OpenApoc

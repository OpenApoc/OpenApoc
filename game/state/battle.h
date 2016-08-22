#pragma once
#include "framework/includes.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

	class BattleTileType : public StateObject<BattleTileType>
	{
		sp<Image> sprite;
		sp<Image> strategySprite;

	};

	class BattleLeftWallType : public StateObject<BattleLeftWallType>
	{
		sp<Image> sprite;

	};

	class BattleRightWallType : public StateObject<BattleRightWallType>
	{
		sp<Image> sprite;

	};

	class BattleSceneryType : public StateObject<BattleSceneryType>
	{
		sp<Image> sprite;

	};


/*
#define CITY_TILE_X (64)
#define CITY_TILE_Y (32)
#define CITY_TILE_Z (16)

#define CITY_STRAT_TILE_X 8
#define CITY_STRAT_TILE_Y 8

	class Vehicle;
	class GameState;
	class Building;
	class Projectile;
	class Scenery;
	class Doodad;
	class DoodadType;
	class SceneryTileType;
	class BaseLayout;
*/

	class Battle
	{
	public:
		Battle() = default;
		~Battle();

		void initMap();

		Vec3<int> size;
		std::map<UString, sp<BattleTileType>> tile_types;
		std::map<UString, sp<BattleLeftWallType>> left_wall_types;
		std::map<UString, sp<BattleRightWallType>> right_wall_types;
		std::map<UString, sp<BattleSceneryType>> scenery_types;

		std::map<Vec3<int>, StateRef<BattleTileType>> initial_tiles;
		std::map<Vec3<int>, StateRef<BattleLeftWallType>> initial_left_walls;
		std::map<Vec3<int>, StateRef<BattleRightWallType>> initial_right_walls;
		std::map<Vec3<int>, StateRef<BattleSceneryType>> initial_scenery;
		
		//up<TileMap> map;

		//void update(GameState &state, unsigned int ticks);
	};

/*
	class City : public StateObject<City>
	{
	public:
		City() = default;
		~City() override;

		void initMap();

		Vec3<int> size;

		std::map<UString, sp<SceneryTileType>> tile_types;
		std::map<Vec3<int>, StateRef<SceneryTileType>> initial_tiles;
		std::map<UString, sp<Building>> buildings;
		std::set<sp<Scenery>> scenery;
		std::set<sp<Doodad>> doodads;
		std::set<sp<Doodad>> portals;

		std::set<sp<Projectile>> projectiles;

		up<TileMap> map;

		void update(GameState &state, unsigned int ticks);
		void dailyLoop(GameState &state);

		void generatePortals(GameState &state);
		sp<Doodad> placeDoodad(StateRef<DoodadType> type, Vec3<float> position);
	};
*/	
}; // namespace OpenApoc

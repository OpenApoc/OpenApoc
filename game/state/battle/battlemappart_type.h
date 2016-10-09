#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <vector>

namespace OpenApoc
{
class DamageType;
class Sample;
class DamageModifier;
class Image;
class VoxelMap;

class BattleMapPartType : public StateObject<BattleMapPartType>
{
  public:
	enum class Type
	{
		Ground,
		LeftWall,
		RightWall,
		Feature
	};
	enum class SupportedByType
	{
		Below = 0,
		
		North = 1,
		East = 2,
		South = 3,
		West = 4,
		
		Above = 5,
		
		Unknown07 = 7,
		
		NorthBelow = 11,
		EastBelow = 12,
		SouthBelow = 13,
		WestBelow = 14,
		
		Unknown20 = 20,
		
		NorthAbove = 21,
		EastAbove = 22,
		SouthAbove = 23,
		WestAbove = 24,
		
		WallsNorthWest = 36,
		
		AnotherNorth = 41,
		AnotherEast = 42,
		AnotherSouth = 43,
		AnotherWest = 44,
		
		AnotherNorthBelow = 51,
		AnotherEastBelow = 52,
		AnotherSouthBelow = 53,
		AnotherWestBelow = 54,
	};

	Type type = Type::Ground;

	sp<Image> sprite;
	sp<Image> strategySprite;
	sp<VoxelMap> voxelMapLOF;
	sp<VoxelMap> voxelMapLOS;
	Vec2<float> imageOffset;

	int constitution = 0;
	int explosion_power = 0;
	int explosion_depletion_rate = 0;
	StateRef<DamageType> explosion_type;

	int fire_resist = 0;
	int fire_burn_time = 0;

	int block_physical = 0;
	int block_gas = 0;
	int block_fire = 0;
	int block_psionic = 0;
	int size = 0;

	StateRef<BattleMapPartType> damaged_map_part;
	StateRef<BattleMapPartType> alternative_map_part;
	std::vector<sp<Image>> animation_frames;

	bool transparent = false;
	int sfxIndex = -1;
	bool door = false;
	bool los_through_terrain = false;
	bool floor = false;
	bool gravlift = false;
	int movement_cost = 0;
	int height = 0;
	// Does not require support
	bool floating = false;
	bool provides_support = false;
	SupportedByType supported_by = SupportedByType::Below;
	bool independent_structure = false;
	bool exit = false;

	// Following members are not serialized, but rather are set up upon being loaded

	StateRef<DamageModifier> damageModifier;
	sp<std::vector<sp<Sample>>> walkSounds;
	sp<Sample> objectDropSound;
	// Object list spawned when this one falls 
	std::vector<StateRef<BattleMapPartType>> rubble;
	// Destroyed ground replacement for level 0 grounds
	StateRef<BattleMapPartType> destroyed_ground_tile;
};
}

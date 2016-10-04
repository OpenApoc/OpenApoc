#pragma once
#include "framework/image.h"
#include "framework/includes.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include "library/voxel.h"

namespace OpenApoc
{
class DamageType;
class Sample;
class DamageModifier;

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
		Below,
		North,
		East,
		South,
		West,
		Above,
		Unknown07,
		NorthBelow,
		EastBelow,
		SouthBelow,
		WestBelow,
		Unknown20,
		NorthAbove,
		EastAbove,
		SouthAbove,
		WestAbove,
		Unknown30,
		Unknown32,
		Unknown36,
		Unknown41,
		Unknown42,
		Unknown43,
		Unknown44,
		Unknown51,
		Unknown52,
		Unknown53,
		Unknown54,
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
	bool floating = false;
	bool provides_support = false;
	SupportedByType supported_by = SupportedByType::Below;
	bool independent_structure = false;
	bool exit = false;

	// Following members are not serialized, but rather are set up upon being loaded

	StateRef<DamageModifier> damageModifier;
	sp<std::vector<sp<Sample>>> walkSounds;
	sp<Sample> objectDropSound;
	// Destroyed ground replacement for ground at level 0, rubble for other types
	std::vector<StateRef<BattleMapPartType>> destroyed_map_parts;
};
}

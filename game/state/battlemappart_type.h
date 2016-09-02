#pragma once
#include "framework/image.h"
#include "framework/includes.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include "library/voxel.h"

namespace OpenApoc
{
class BattleMapPartType : public StateObject<BattleMapPartType>
{
  public:
	enum class Type
	{
		Ground,
		LeftWall,
		RightWall,
		Scenery
	};
	static const std::map<Type, UString> TypeMap;
	Type type = Type::Ground;

	enum class ExplosionType
	{
		BlankOrSmoke,
		AlienGas,
		Incendary,
		StunGas,
		HighExplosive
	};
	static const std::map<ExplosionType, UString> ExplosionTypeMap;

	sp<Image> sprite;
	sp<Image> strategySprite;
	sp<VoxelMap> voxelMapLOF;
	sp<VoxelMap> voxelMapLOS;
	Vec2<float> imageOffset = {0, 0};

	int constitution = 0;
	int explosion_power = 0;
	int explosion_radius_divizor = 0;
	ExplosionType explosion_type = ExplosionType::BlankOrSmoke;
	StateRef<BattleMapPartType> damaged_map_part;
	std::vector<sp<Image>> animation_frames;
};
}

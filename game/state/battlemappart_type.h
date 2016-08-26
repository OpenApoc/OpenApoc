#pragma once
#include "framework/image.h"
#include "framework/includes.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
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
	Type type;

	sp<Image> sprite;
	sp<Image> strategySprite;
	sp<VoxelMap> voxelMap;
	Vec2<float> imageOffset;
};
}
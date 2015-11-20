#pragma once
#include "library/sp.h"

#include "library/vec.h"
#include "library/strings.h"
#include <vector>
#include <memory>
#include <map>

namespace OpenApoc
{

class RulesLoader;
class Image;
class VoxelMap;

class VehicleDefinition
{
  private:
	VehicleDefinition() {}
	friend class RulesLoader;

  public:
	enum class Type
	{
		Flying,
		Ground,
	};
	enum class Direction
	{
		N,
		NNE,
		NE,
		NEE,
		E,
		SEE,
		SE,
		SSE,
		S,
		SSW,
		SW,
		SWW,
		W,
		NWW,
		NW,
		NNW,
	};
	enum class Banking
	{
		Flat,
		Left,
		Right,
		Ascending,
		Decending,
	};

	UString ID;
	UString name;
	UString manufacturer;
	Type type;
	std::map<Banking, std::map<Direction, sp<Image>>> sprites;
	std::map<Direction, sp<Image>> strategySprites;
	std::map<Direction, sp<Image>> shadowSprites;
	// The same sprites but using vectors for directions
	std::vector<std::pair<Vec3<float>, sp<Image>>> directionalSprites;
	std::vector<std::pair<Vec3<float>, sp<Image>>> directionalStrategySprites;
	std::vector<std::pair<Vec3<float>, sp<Image>>> directionalShadowSprites;
	Vec3<float> size;
	Vec2<float> imageOffset;
	Vec2<float> shadowOffset;
	sp<VoxelMap> voxelMap;
};

}; // namespace OpenApoc

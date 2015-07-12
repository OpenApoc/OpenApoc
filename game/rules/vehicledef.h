#pragma once

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
	VehicleDefinition(){};
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

	UString name;
	Type type;
	std::map<Banking, std::map<Direction, std::shared_ptr<Image> > > sprites;
	//The same sprites but using vectors for directions
	std::vector<std::pair<Vec3<float>, std::shared_ptr<Image>>> directionalSprites;
	Vec3<float> size;
	std::shared_ptr<VoxelMap> voxelMap;
};

}; //namespace OpenApoc

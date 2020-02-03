#pragma once

#include "game/state/rules/battle/damage.h"
#include "game/state/stateobject.h"
#include "game/state/tilemap/tilemap.h"
#include "library/sp.h"
#include "library/vec.h"
#include <set>
#include <vector>

namespace OpenApoc
{
class DamageType;
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
	Type type = Type::Ground;
	enum class AutoConvert
	{
		None,
		Fire,
		Smoke
	};
	AutoConvert autoConvert = AutoConvert::None;

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

	std::map<DamageType::BlockType, int> block;

	int size = 0;

	StateRef<BattleMapPartType> damaged_map_part;
	StateRef<BattleMapPartType> alternative_map_part;
	std::vector<sp<Image>> animation_frames;

	bool transparent = false;
	int sfxIndex = -1;
	bool door = false;
	bool blocksLOS = false;
	bool floor = false;
	bool gravlift = false;
	int movement_cost = 0;
	// Max value 39
	int height = 0;
	// Does not require support
	bool floating = false;
	bool provides_support = false;
	bool independent_structure = false;
	bool exit = false;

	// All these must be destroyed in alien buildings for the building to be "disabled"
	bool missionObjective = false;
	// When reinforcements spawn in alien buildings, they spawn on one of these
	bool reinforcementSpawner = false;

	// Support parameters
	// (rules for support are explained in depth in BattleMapPart::findSupport() definition)

	bool supportedByAbove = false;

	// Directions in which objects can "cling to" other objects and stay afloat.
	// If set, object can get support from another object in this direction.
	// Rules for different types:
	//  - Ground will get support from a Ground only
	//  - Feature will get support from a Feature or a matching perpendicular Wall
	//	  (Right if N/S, Left if E/W)
	//  - Wall will get support from the same type of Wall
	//	  (provided the Wall's type matches direction: Left for N/S, Right for E/W)
	std::set<MapDirection> supportedByDirections;
	bool isSupportedByDirection(MapDirection direction)
	{
		return supportedByDirections.find(direction) != supportedByDirections.end();
	}

	// Additional types objects can get directional support from.
	// Directions must be also set for this to work.
	// If set, objects can additionally get support from this type of object in their direction.
	//
	// Rules for different types:
	//  - Ground/Wall on a current level
	//  - Feature on a level below
	std::set<Type> supportedByTypes;

	// Unknowns from vanilla, can be 7 or 20
	int vanillaSupportedById = 0;
	// Debug output to ensure our collapsible map system is working properly
	// Expects only combinations found in vanilla files
	int getVanillaSupportedById()
	{
		if (vanillaSupportedById)
			return vanillaSupportedById;
		if (supportedByDirections.empty() && supportedByTypes.empty())
			return 0;
		if (supportedByDirections.size() == 2 && supportedByTypes.size() == 3)
			return 36;
		int result = (int)*supportedByDirections.begin();
		if (supportedByTypes.empty())
			return result;
		if (supportedByTypes.size() == 1)
			return (*supportedByTypes.begin() == Type::Ground ? 10 : 20) + result;
		return (supportedByTypes.find(Type::Feature) == supportedByTypes.end() ? 40 : 50) + result;
	}

	// Following members are not serialized, but rather are set up upon being loaded

	StateRef<DamageModifier> damageModifier;
	sp<std::vector<sp<Sample>>> walkSounds;
	sp<Sample> objectDropSound;
	// Object list spawned when this one falls
	std::vector<StateRef<BattleMapPartType>> rubble;
	// Destroyed ground replacement for level 0 grounds
	StateRef<BattleMapPartType> destroyed_ground_tile;
};
} // namespace OpenApoc

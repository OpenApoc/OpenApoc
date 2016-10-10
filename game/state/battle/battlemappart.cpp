#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart_type.h"
#include "library/strings_format.h"
#include "game/state/city/projectile.h"
#include "game/state/gamestate.h"
#include "game/state/rules/damage.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_battleitem.h"
#include <algorithm>
#include <set>

namespace OpenApoc
{

void BattleMapPart::die(GameState &state, bool violently)
{
	if (violently)
	{
		// FIXME: Explode if nessecary
	}

	// If falling just cease to be, do damage and add rubble
	if (falling)
	{
		this->tileObject->removeFromMap();
		this->tileObject.reset();
		destroyed = true;

		LogWarning("Deal damage!");
		return;
	}

	// Doodad
	auto doodad = state.current_battle->placeDoodad({ &state, "DOODAD_29_EXPLODING_TERRAIN" },
		tileObject->getCenter());

	// Replace with damaged / destroyed
	if (!this->damaged && type->damaged_map_part)
	{
		this->damaged = true;
		this->type = type->damaged_map_part;
	}
	else
	{
		// Don't destroy bottom tiles, else everything will leak out
		// Replace ground with destroyed
		if (this->position.z == 0 && this->type->type == BattleMapPartType::Type::Ground)
		{
			this->type = type->destroyed_ground_tile;
		}
		// Destroy map part
		else
		{
			destroyed = true;
		}
	}
	
	// Cease functioning
	ceaseDoorFunction();
	ceaseSupportProvision();

	if (destroyed)
	{
		this->tileObject->removeFromMap();
		this->tileObject.reset();
	}
}


int BattleMapPart::getMaxFrames()
{
	return alternative_type ? (int)alternative_type->animation_frames.size()
	                        : (int)type->animation_frames.size();
}

int BattleMapPart::getAnimationFrame()
{
	if (door)
	{
		return std::min(getMaxFrames() - 1, door->getAnimationFrame());
	}
	else
	{
		return type->animation_frames.size() == 0
		           ? -1
		           : animation_frame_ticks / TICKS_PER_FRAME_MAP_PART;
	}
}

bool BattleMapPart::handleCollision(GameState &state, Collision &c)
{
	if (!this->tileObject)
	{
		// It's possible multiple projectiles hit the same tile in the same
		// tick, so if the object has already been destroyed just NOP this.
		// The projectile will still 'hit' this tile though.
		return false;
	}
	if (this->falling)
	{
		// Already falling, just continue
		return false;
	}

	// Calculate damage (hmm, apparently Apoc uses 50-150 damage model for terrain, unlike UFO1&2
	// which used 25-75
	int damage = randDamage050150(state.rng, c.projectile->damageType->dealDamage(
	                                             c.projectile->damage, type->damageModifier));
	if (damage <= type->constitution)
	{
		return false;
	}

	// If we came this far, map part has been damaged and must cease to be
	die(state);
	return false;
}

void BattleMapPart::ceaseDoorFunction()
{
	if (!door)
	{
		return;
	}

	if (alternative_type)
		type = alternative_type;
	// Remove from door's map parts
	wp<BattleMapPart> sft = shared_from_this();
	door->mapParts.remove_if([sft](wp<BattleMapPart> p) {
		auto swp = sft.lock();
		auto sp = p.lock();
		if (swp && sp)
			return swp == sp;
		return false;
	});
	door.clear();
}

bool BattleMapPart::findSupport()
{
	// Initial setup and quick checks
	providesHardSupport = true;
	if (type->floating)
	{
		return true;
	}
	auto pos = tileObject->getOwningTile()->position;
	if (pos.z == 0)
	{
		return true;
	}
	auto &map = tileObject->map;
	auto tileType = tileObject->getType();

	// Clean support providers for this map part
	for (int x = pos.x - 1; x <= pos.x + 1; x++)
	{
		for (int y = pos.y - 1; y <= pos.y + 1; y++)
		{
			for (int z = pos.z - 1; z <= pos.z + 1; z++)
			{
				if (x < 0 || x >= map.size.x
					|| y < 0 || y >= map.size.y
					|| z < 0 || z >= map.size.z)
				{
					continue;
				}
				auto tile = map.getTile(x, y, z);
				for (auto &o : tile->ownedObjects)
				{
					if (o->getType() == TileObject::Type::Ground
						|| o->getType() == TileObject::Type::Feature
						|| o->getType() == TileObject::Type::LeftWall
						|| o->getType() == TileObject::Type::RightWall)
					{
						auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
						auto it = mp->supportedParts.begin();
						while (it!=mp->supportedParts.end())
						{
							auto &p = *it;
							if (p.first == pos && p.second == type->type)
							{
								it = supportedParts.erase(it);
							}
							else
							{
								it++;
							}
						}
					}
				}
			}
		}
	}

	// TODO: FIGURE OUT WHICH OF THESE ARE NOT HARD SUPPORTS
	// There are several ways battle map part can get supported:
	//
	// Ground:
	//  - G1: Feature Below
	//  - G2: Wall Adjacent Below
	//
	// Feature:
	//  - F1: Ground Current
	//  - F2: Feature Below
	//  - F3: Feature Above (if "Above" supported by) 
	//
	// Wall:
	//  - W2: Feature Current
	//  - W2: Ground Adjacent Current
	//  - W3: Feature Adjacent Below
	//  - W4: Wall Below
	//  - W5: Wall Above (if "Above" supported by) 
	// 
	// Then, there is a specified "Supported By" condition:
	//  - Below condition has no meaning, as every map part can be supported by it
	//  - Above condition is kinda unique, and was described above where appropriate
	//  - Other conditions all basically mean that instead of 2 supports, only 1 support is enough,
	//    provided it comes from the right direction and elevation
	//
	// Then, every object can cling to two adjacent objects of the same type
	//  - This does not provide hard support
	//
	// Finally, every map part can be supported if it has established support lines
	// on both sides to a supported object
	//  - Object "shoots" a line in both directions and as long as there is an object on every tile
	//    the line continues, and if an object providing hard support is reached, 
	//	  then it can be supported
	//  - Objects supported this way cannot provide hard support to other objects
	//
	// Therefore, it makes sense to first check special conditions, and then check
	// adjacency and "supported by" conditions
	std::map<Vec3<int>, sp<BattleMapPart>> supports;
	auto sft = shared_from_this();

	// Step 01: Check for special conditions
	// Bounds to check for special conditions in
	int startX = pos.x - 1;
	int endX = pos.x + 1;
	int startY = pos.y - 1;
	int endY = pos.y + 1;
	int startZ = pos.z - 1;
	int endZ = pos.z + 1;
	switch (type->type)
	{
		case BattleMapPartType::Type::Ground:
			// We're only interested in tiles current or below
			endZ = pos.z;
			break;
		case BattleMapPartType::Type::Feature:
			// We're only interested in tiles with matching x and y
			startX = pos.x;
			endX = startX;
			startY = pos.y;
			endY = startY;
			break;
		case BattleMapPartType::Type::LeftWall:
			// We're only interested in tiles above/below and to the west
			endX = pos.x;
			startY = pos.y;
			endY = startY;
			break;
		case BattleMapPartType::Type::RightWall:
			// We're only interested in tiles above/below and to the north
			endY = pos.y;
			startX = pos.x;
			endX = startX;
			break;
	}

	// Step 01: Check for special conditions
	// Do the check
	for (int x = startX; x <= endX; x++)
	{
		for (int y = startY; y <= endY; y++)
		{
			for (int z = startZ; z <= endZ; z++)
			{
				if (x < 0 || x >= map.size.x
					|| y < 0 || y >= map.size.y
					|| z < 0 || z >= map.size.z)
				{
					continue;
				}
				auto tile = map.getTile(x, y, z);
				for (auto &o : tile->ownedObjects)
				{
					bool canSupport = false;
					
					switch (type->type)
					{
					case BattleMapPartType::Type::Ground:
						//  - G1: Feature Current/Below
						canSupport = canSupport || (x == pos.x && y == pos.y && o->getType() == TileObject::Type::Feature);
						//  - G3: Wall Adjacent Below
						if ((x >= pos.x || y >= pos.y) && z < pos.z)
						{
							canSupport = canSupport || (x >= pos.x && o->getType() == TileObject::Type::LeftWall);
							canSupport = canSupport || (y >= pos.y && o->getType() == TileObject::Type::RightWall);
						}
						break;
					case BattleMapPartType::Type::Feature:
						//  - F1: Ground Current
						canSupport = canSupport || (z == pos.z && o->getType() == TileObject::Type::Ground);
						//  - F2: Feature Below
						canSupport = canSupport || (z < pos.z && o->getType() == TileObject::Type::Feature);
						//  - F3: Feature Above (if "Above" supported by) 
						canSupport = canSupport || (z > pos.z && o->getType() == TileObject::Type::Feature 
							&& type->supported_by == BattleMapPartType::SupportedByType::Above);
						break;
					case BattleMapPartType::Type::LeftWall:
						//  - W1: Feature Current
						canSupport = canSupport || (x == pos.x && y == pos.y && z == pos.z && o->getType() == TileObject::Type::Feature);
						//  - W2: Ground Adjacent Current
						canSupport = canSupport || (z == pos.z && o->getType() == TileObject::Type::Ground);
						//  - W3: Feature Adjacent Below
						canSupport = canSupport || (z < pos.z && o->getType() == TileObject::Type::Feature);
						//  - W4: Wall Below
						canSupport = canSupport || (x == pos.x && z < pos.z 
							&& o->getType() == TileObject::Type::LeftWall);
						//  - W5: Wall Above (if "Above" supported by) 
						canSupport = canSupport || (x == pos.x && z > pos.z 
							&& o->getType() == TileObject::Type::LeftWall
							&& type->supported_by == BattleMapPartType::SupportedByType::Above);
						break;
					case BattleMapPartType::Type::RightWall:
						//  - W1: Feature Current
						canSupport = canSupport || (x == pos.x && y == pos.y && z == pos.z && o->getType() == TileObject::Type::Feature);
						//  - W2: Ground Adjacent Current
						canSupport = canSupport || (z == pos.z && o->getType() == TileObject::Type::Ground);
						//  - W3: Feature Adjacent Below
						canSupport = canSupport || (z < pos.z && o->getType() == TileObject::Type::Feature);
						//  - W4: Wall Below
						canSupport = canSupport || (y == pos.y && z < pos.z 
							&& o->getType() == TileObject::Type::RightWall);
						//  - W5: Wall Above (if "Above" supported by) 
						canSupport = canSupport || (y == pos.y && z > pos.z 
							&& o->getType() == TileObject::Type::RightWall
							&& type->supported_by == BattleMapPartType::SupportedByType::Above);
						break;
					}
					
					if (canSupport)
					{
						auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
						// Vanilla seems to completely ignore "provides support" flag 
						if (mp != sft && mp->isAlive() && !mp->damaged/*&& (mp->type->provides_support || mp->type->type == BattleMapPartType::Type::Ground || z >= pos.z)*/)
						{
							mp->supportedParts.emplace_back(position, type->type);
							return true;
						}
					}
				}
			}
		}
	}
	
	// Step 02: Find all map parts that can support this

	// Search pattern for ground
	static const std::list<std::pair<Vec2<int>, TileObject::Type>> groundList =
	{
		{ { -1,0 },TileObject::Type::Ground },
		{ { 1,0 },TileObject::Type::Ground },
		{ { 0,-1 },TileObject::Type::Ground },
		{ { 0,1 },TileObject::Type::Ground },
	};

	// Search pattern for left wall
	static const std::list<std::pair<Vec2<int>, TileObject::Type>> leftWallList =
	{
		{ { 0,-1 },TileObject::Type::LeftWall },
		{ { 0,1 },TileObject::Type::LeftWall },
		{ { 0,-1 },TileObject::Type::Feature },
		{ { 0,1 },TileObject::Type::Feature },
	};

	// Search pattern for right wall
	static const std::list<std::pair<Vec2<int>, TileObject::Type>> rightWallList =
	{
		{ { -1,0 },TileObject::Type::RightWall },
		{ { 1,0 },TileObject::Type::RightWall },
		{ { -1,0 },TileObject::Type::Feature },
		{ { 1,0 },TileObject::Type::Feature },
	};

	// Search pattern for "WallsNorthWest" SupportedBy type
	static const std::list<std::pair<Vec2<int>, TileObject::Type>> wallsNorthWestList =
	{
		{ { 0, -1 },TileObject::Type::LeftWall },
		{ { -1,0 },TileObject::Type::RightWall },
	};

	// Search pattern for features
	static const std::list<std::pair<Vec2<int>, TileObject::Type>> featureList =
	{
		{ { -1,0 },TileObject::Type::Feature },
		{ { 1,0 },TileObject::Type::Feature },
		{ { 0,-1 },TileObject::Type::Feature },
		{ { 0,1 },TileObject::Type::Feature },
	};

	// Search pattern for features of AnotherBelow type
	static const std::list<std::pair<Vec2<int>, TileObject::Type>> anotherNorthBelowList =
	{
		{ { 0,-1 },TileObject::Type::Feature },
		{ { 0,-1 },TileObject::Type::Ground },
	};

	// Search pattern for features of AnotherBelow type
	static const std::list<std::pair<Vec2<int>, TileObject::Type>> anotherEastBelowList =
	{
		{ { 1,0 },TileObject::Type::Feature },
		{ { 1,0 },TileObject::Type::Ground },
	};

	// Search pattern for features of AnotherBelow type
	static const std::list<std::pair<Vec2<int>, TileObject::Type>> anotherSouthBelowList =
	{
		{ { 0,1 },TileObject::Type::Feature },
		{ { 0,1 },TileObject::Type::Ground },
	};
	
	// Search pattern for features of AnotherBelow type
	static const std::list<std::pair<Vec2<int>, TileObject::Type>> anotherWestBelowList =
	{
		{ { -1,0 },TileObject::Type::Feature },
		{ { -1,0 },TileObject::Type::Ground },
	};

	// Search pattern for north type
	static const std::list<std::pair<Vec2<int>, TileObject::Type>> northList =
	{
		{ { 0,-1 },TileObject::Type::Feature },
		{ { 0,0 },TileObject::Type::RightWall },
	};

	// Search pattern for east type
	static const std::list<std::pair<Vec2<int>, TileObject::Type>> eastList =
	{
		{ { 1,0 },TileObject::Type::Feature },
		{ { 1,0 },TileObject::Type::LeftWall },
	};

	// Search pattern for south type
	static const std::list<std::pair<Vec2<int>, TileObject::Type>> southList =
	{
		{ { 0,1 },TileObject::Type::Feature },
		{ { 0,1 },TileObject::Type::RightWall },
	};

	// Search pattern for west type
	static const std::list<std::pair<Vec2<int>, TileObject::Type>> westList =
	{
		{ { -1,0 },TileObject::Type::Feature },
		{ { 0,0 },TileObject::Type::LeftWall },
	};

	std::list<std::pair<Vec2<int>, TileObject::Type>> specialList;

	// Assign search pattern
	std::list<std::pair<Vec2<int>, TileObject::Type>> const * currentList = nullptr;
	switch (type->supported_by)
	{
		case BattleMapPartType::SupportedByType::North:
		case BattleMapPartType::SupportedByType::AnotherNorth:
		case BattleMapPartType::SupportedByType::NorthAbove:
		case BattleMapPartType::SupportedByType::NorthBelow:
			currentList = &specialList;
			specialList = northList;
			specialList.push_back({ { 0,-1 }, tileType });
			break;
		case BattleMapPartType::SupportedByType::SouthAbove:
		case BattleMapPartType::SupportedByType::SouthBelow:
		case BattleMapPartType::SupportedByType::South:
		case BattleMapPartType::SupportedByType::AnotherSouth:
			currentList = &specialList;
			specialList = southList;
			specialList.push_back({ { 0,1 }, tileType });
			break;
		case BattleMapPartType::SupportedByType::EastAbove:
		case BattleMapPartType::SupportedByType::EastBelow:
		case BattleMapPartType::SupportedByType::East:
		case BattleMapPartType::SupportedByType::AnotherEast:
			currentList = &specialList;
			specialList = eastList;
			specialList.push_back({ { 1, 0}, tileType });
			break;
		case BattleMapPartType::SupportedByType::WestAbove:
		case BattleMapPartType::SupportedByType::WestBelow:
		case BattleMapPartType::SupportedByType::West:
		case BattleMapPartType::SupportedByType::AnotherWest:
			currentList = &specialList;
			specialList = westList;
			specialList.push_back({ { -1, 0 }, tileType });
			break;
		case BattleMapPartType::SupportedByType::WallsNorthWest:
			currentList = &wallsNorthWestList;
			break;
		case BattleMapPartType::SupportedByType::AnotherNorthBelow:
			currentList = &anotherNorthBelowList;
			break;
		case BattleMapPartType::SupportedByType::AnotherEastBelow:
			currentList = &anotherEastBelowList;
			break;
		case BattleMapPartType::SupportedByType::AnotherSouthBelow:
			currentList = &anotherSouthBelowList;
			break;
		case BattleMapPartType::SupportedByType::AnotherWestBelow:
			currentList = &anotherWestBelowList;
			break;
		case BattleMapPartType::SupportedByType::Below:
		case BattleMapPartType::SupportedByType::Above:
		case BattleMapPartType::SupportedByType::Unknown20:
		case BattleMapPartType::SupportedByType::Unknown07:
			switch (type->type)
			{
			case BattleMapPartType::Type::Ground:
				currentList = &groundList;
				break;
			case BattleMapPartType::Type::Feature:
				currentList = &featureList;
				break;
			case BattleMapPartType::Type::LeftWall:
				currentList = &leftWallList;
				break;
			case BattleMapPartType::Type::RightWall:
				currentList = &rightWallList;
				break;
			}
			break;
	}

	// Select z level
	startZ = pos.z - 1;
	endZ = pos.z + 1;
	switch (type->supported_by)
	{
	// Look on our level
	case BattleMapPartType::SupportedByType::North:
	case BattleMapPartType::SupportedByType::AnotherNorth:
	case BattleMapPartType::SupportedByType::South:
	case BattleMapPartType::SupportedByType::AnotherSouth:
	case BattleMapPartType::SupportedByType::East:
	case BattleMapPartType::SupportedByType::AnotherEast:
	case BattleMapPartType::SupportedByType::West:
	case BattleMapPartType::SupportedByType::AnotherWest:
	case BattleMapPartType::SupportedByType::WallsNorthWest:
		startZ = pos.z;
		endZ = pos.z;
		break;
	// Look on level above
	case BattleMapPartType::SupportedByType::NorthAbove:
	case BattleMapPartType::SupportedByType::SouthAbove:
	case BattleMapPartType::SupportedByType::EastAbove:
	case BattleMapPartType::SupportedByType::WestAbove:
		//startZ = pos.z;
		endZ = pos.z;
		break;
	// Look on level below
	case BattleMapPartType::SupportedByType::Below:
	case BattleMapPartType::SupportedByType::NorthBelow:
	case BattleMapPartType::SupportedByType::EastBelow:
	case BattleMapPartType::SupportedByType::SouthBelow:
	case BattleMapPartType::SupportedByType::WestBelow:
		//		endZ = pos.z;
		startZ = pos.z;
		endZ = pos.z;
		break;
	case BattleMapPartType::SupportedByType::AnotherNorthBelow:
	case BattleMapPartType::SupportedByType::AnotherEastBelow:
	case BattleMapPartType::SupportedByType::AnotherSouthBelow:
	case BattleMapPartType::SupportedByType::AnotherWestBelow:
		endZ = pos.z;
		break;
	}

	// Search for map parts
	for (auto &p : *currentList)
	{
		int x = pos.x + p.first.x;
		int y = pos.y + p.first.y;
		for (int z = startZ; z <= endZ; z++)
		{
			if (x < 0 || x >= map.size.x
				|| y < 0 || y >= map.size.y
				|| z < 0 || z >= map.size.z)
			{
				continue;
			}
			auto tile = map.getTile(x, y, z);
			for (auto &o : tile->ownedObjects)
			{
				// Matching battle map parts that fit the criteria of axis differences
				// Also must not be equal to us and be alive
				// Also must provide support
				if (o->getType() == p.second)
				{
					auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
					if (mp != sft && mp->isAlive())
					{
						// Support provision condition:
						// a) Provides support flag
						// b) Is ground (ground has no provide support flag, but still does provide it)
						// c) Map part that satisfies its condition has same supported by type
						
						// Vanilla seems to completely ignore "provides_support" flag
						// However, ground still cannot give support to anything on a different level than it
						bool canSupport = !mp->damaged && (z == pos.z || mp->type->type != BattleMapPartType::Type::Ground)/* && mp->type->provides_support || tileType == TileObject::Type::Ground || z  >= pos.z*/;
						if (!canSupport)
						{
							switch (type->supported_by)
							{
							case BattleMapPartType::SupportedByType::North:
							case BattleMapPartType::SupportedByType::AnotherNorth:
							case BattleMapPartType::SupportedByType::NorthAbove:
							case BattleMapPartType::SupportedByType::NorthBelow:
							case BattleMapPartType::SupportedByType::AnotherNorthBelow:
								canSupport = x == pos.x && y < pos.y && mp->type->supported_by == type->supported_by;
								break;
							case BattleMapPartType::SupportedByType::South:
							case BattleMapPartType::SupportedByType::AnotherSouth:
							case BattleMapPartType::SupportedByType::SouthAbove:
							case BattleMapPartType::SupportedByType::SouthBelow:
							case BattleMapPartType::SupportedByType::AnotherSouthBelow:
								canSupport = x == pos.x && y > pos.y&& mp->type->supported_by == type->supported_by;
								break;
							case BattleMapPartType::SupportedByType::East:
							case BattleMapPartType::SupportedByType::AnotherEast:
							case BattleMapPartType::SupportedByType::EastAbove:
							case BattleMapPartType::SupportedByType::EastBelow:
							case BattleMapPartType::SupportedByType::AnotherEastBelow:
								canSupport = y == pos.y && x > pos.x && mp->type->supported_by == type->supported_by;
								break;
							case BattleMapPartType::SupportedByType::West:
							case BattleMapPartType::SupportedByType::AnotherWest:
							case BattleMapPartType::SupportedByType::WestAbove:
							case BattleMapPartType::SupportedByType::WestBelow:
							case BattleMapPartType::SupportedByType::AnotherWestBelow:
								canSupport = y == pos.y && x < pos.x && mp->type->supported_by == type->supported_by;
								break;
							}
						}
						if (canSupport)
						{
							supports[Vec3<int>(x - pos.x, y - pos.y, z - pos.z)] = mp;
							// No need to further look in this area
							break;
						}
					}
				}
			}
		}
	}
	
	// Step 04: Check if "supported by" condition is satisfied

	sp<BattleMapPart> supporter;
	switch (type->supported_by)
	{
	// We have already checked theese two
	case BattleMapPartType::SupportedByType::Below:
	case BattleMapPartType::SupportedByType::Above:
		break;
		break;
	case BattleMapPartType::SupportedByType::North:
	case BattleMapPartType::SupportedByType::AnotherNorth:
		supporter = supports[{ 0, -1, 0 }];
		if (!supporter)
		{
			supporter = supports[{ 0, 0, 0 }];
		}
		break;
	case BattleMapPartType::SupportedByType::East:
	case BattleMapPartType::SupportedByType::AnotherEast:
		supporter = supports[{ 1, 0, 0 }];
		break;
	case BattleMapPartType::SupportedByType::South:
	case BattleMapPartType::SupportedByType::AnotherSouth:
		supporter = supports[{ 0, 1, 0 }];
		break;
	case BattleMapPartType::SupportedByType::West:
	case BattleMapPartType::SupportedByType::AnotherWest:
		supporter = supports[{ -1, 0, 0 }];
		if (!supporter)
		{
			supporter = supports[{ 0, 0, 0 }];
		}
		break;
	case BattleMapPartType::SupportedByType::NorthBelow:
		//supporter = supports[{ 0, -1, -1 }];
		supporter = supports[{ 0, -1,0 }];
		break;
	case BattleMapPartType::SupportedByType::EastBelow:
		//supporter = supports[{ 1, 0, -1 }];
		supporter = supports[{ 1, 0, 0 }];
		break;
	case BattleMapPartType::SupportedByType::SouthBelow:
		//supporter = supports[{ 0, 1, -1 }];
		supporter = supports[{ 0, 1, 0 }];
		break;
	case BattleMapPartType::SupportedByType::WestBelow:
		//supporter = supports[{ -1, 0, -1 }];
		supporter = supports[{ -1, 0, 0 }];
		break;
	case BattleMapPartType::SupportedByType::WallsNorthWest:
		supporter = supports[{ 0, -1, 0 }];
		if (!supporter)
		{
			supporter = supports[{ -1, 0, 0 }];
		}
		break;
	case BattleMapPartType::SupportedByType::AnotherNorthBelow:
		supporter = supports[{ 0, -1, -1 }];
		if (!supporter)
		{
			supporter = supports[{ 0, -1, 0 }];
		}
		break;
	case BattleMapPartType::SupportedByType::AnotherEastBelow:
		supporter = supports[{ 1, 0, -1 }];
		if (!supporter)
		{
			supporter = supports[{ 1, 0, 0 }];
		}
		break;
	case BattleMapPartType::SupportedByType::AnotherSouthBelow:
		supporter = supports[{ 0, 1, -1 }];
		if (!supporter)
		{
			supporter = supports[{ 0, 1, 0 }];
		}
		break;
	case BattleMapPartType::SupportedByType::AnotherWestBelow:
		supporter = supports[{ -1, 0, -1 }];
		if (!supporter)
		{
			supporter = supports[{ -1, 0, 0 }];
		}
		break;
	case BattleMapPartType::SupportedByType::NorthAbove:
		//supporter = supports[{ 0, -1, 1 }];
		supporter = supports[{ 0, -1, -1 }];
		if (!supporter)
		{
			supporter = supports[{ 0, -1, 0 }];
		}
		break;
	case BattleMapPartType::SupportedByType::EastAbove:
		//supporter = supports[{ 1, 0, 1 }];
		supporter = supports[{ 1, 0, -1 }];
		if (!supporter)
		{
			supporter = supports[{ 1, 0, 0 }];
		}
		break;
	case BattleMapPartType::SupportedByType::SouthAbove:
		//supporter = supports[{ 0, 1, 1 }];
		supporter = supports[{ 0, 1, -1 }];
		if (!supporter)
		{
			supporter = supports[{ 0, 1, 0 }];
		}
		break;
	case BattleMapPartType::SupportedByType::WestAbove:
		//supporter = supports[{ -1, 0, 1 }];
		supporter = supports[{ -1, 0, -1 }];
		if (!supporter)
		{
			supporter = supports[{-1,0, 0 }];
		}
		break;
	default:
		break;
	}
	if (supporter)
	{
		supporter->supportedParts.emplace_back(position, type->type);
		return true;
	}
	
	// If we reached this - we can not provide hard support
	providesHardSupport = false;

	// Step 05: Cling to two adjacent objects

	int countSupportsOnSides = (supports[{ -1, 0, 0 }] ? 1 : 0) + (supports[{ 1, 0, 0 }] ? 1 : 0) + (supports[{ 0, -1, 0 }] ? 1 : 0) + (supports[{ 0, 1, 0 }] ? 1 : 0);
	if (pos.x == 0 || pos.y == 0 || pos.x == map.size.x - 1 || pos.y == map.size.y - 1)
	{
		countSupportsOnSides++;
	}
	if (countSupportsOnSides >= 2)
	{
		if (supports[{ -1, 0, 0 }])
			supports[{ -1, 0, 0 }]->supportedParts.emplace_back(position, type->type);
		if (supports[{ 1, 0, 0 }])
			supports[{ 1, 0, 0 }]->supportedParts.emplace_back(position, type->type);
		if (supports[{ 0, -1, 0 }])
			supports[{ 0, -1, 0 }]->supportedParts.emplace_back(position, type->type);
		if (supports[{ 0, 1, 0 }])
			supports[{ 0, 1, 0 }]->supportedParts.emplace_back(position, type->type);
		return true;
	}

	// Step 06: Shoot "support lines" and try to find something

	// Scan on X
	if (type->type != BattleMapPartType::Type::LeftWall)
	{
		int y = pos.y;
		int z = pos.z;
		
		bool found;
		for (int increment = -1; increment <= 1; increment += 2)
		{
			found = false;
			int x = pos.x + increment;
			do
			{
				if (x < 0 || x >= map.size.x)
				{
					found = true;
					break;
				}
				sp<BattleMapPart> mp = nullptr;
				auto tile = map.getTile(x, y, z);
				for (auto &o : tile->ownedObjects)
				{
					if (o->getType() == tileType)
					{
						mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
					}
				}
				// Could not find map part of this type or it cannot provide support
				// We ignore those that have positive "ticksUntilFalling" as those can be saved yet
				if (!mp || mp->destroyed || mp->damaged || mp->falling)
				{
					// fail
					break;
				}
				// Found map part that provides hard support
				if (mp->providesHardSupport)
				{
					// success
					found = true;
					break;
				}
				// continue
				x += increment;
			} while (true);
			if (!found)
			{
				break;
			}
		}
		// If found both ways - cling to neighbours on X
		if (found)
		{
			for (int increment = -1; increment <= 1; increment += 2)
			{
				int x = pos.x + increment;
				if (x < 0 || x >= map.size.x)
				{
					continue;
				}
				sp<BattleMapPart> mp = nullptr;
				auto tile = map.getTile(x, y, z);
				for (auto &o : tile->ownedObjects)
				{
					if (o->getType() == tileType)
					{
						mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
					}
				}
				if (!mp)
				{
					LogError("Map part disappeared? %d %d %d", x, y, z);
					return false;
				}
				mp->supportedParts.emplace_back(position, type->type);
			}
			return true;
		}
	}

	// Scan on Y
	if (type->type != BattleMapPartType::Type::RightWall)
	{
		int x = pos.x;
		int z = pos.z;

		bool found;
		for (int increment = -1; increment <= 1; increment += 2)
		{
			found = false;
			int y = pos.y + increment;
			do
			{
				if (y < 0 || y >= map.size.y)
				{
					found = true;
					break;
				}
				sp<BattleMapPart> mp = nullptr;
				auto tile = map.getTile(x, y, z);
				for (auto &o : tile->ownedObjects)
				{
					if (o->getType() == tileType)
					{
						mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
					}
				}
				// Could not find map part of this type or it cannot provide support
				// We ignore those that have positive "ticksUntilFalling" as those can be saved yet
				if (!mp || mp->destroyed || mp->damaged || mp->falling)
				{
					// fail
					break;
				}
				// Found map part that provides hard support
				if (mp->providesHardSupport)
				{
					// success
					found = true;
					break;
				}
				// continue
				y += increment;
			} while (true);
			if (!found)
			{
				break;
			}
		}
		// If found both ways - cling to neighbours on Y
		if (found)
		{
			for (int increment = -1; increment <= 1; increment += 2)
			{
				int y = pos.y + increment;
				if (y < 0 || y >= map.size.y)
				{
					continue;
				}
				sp<BattleMapPart> mp = nullptr;
				auto tile = map.getTile(x, y, z);
				for (auto &o : tile->ownedObjects)
				{
					if (o->getType() == tileType)
					{
						mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
					}
				}
				if (!mp)
				{
					LogError("Map part disappeared? %d %d %d", x, y, z);
					return false;
				}
				mp->supportedParts.emplace_back(position, type->type);
			}
			return true;
		}
	}

	return false;
}

sp<std::set<BattleMapPart*>> BattleMapPart::getSupportedParts()
{
	sp<std::set<BattleMapPart*>> collapseList = mksp<std::set<BattleMapPart*>>();
	auto &map = tileObject->map;
	for (auto &p : this->supportedParts)
	{
		auto tile = map.getTile(p.first);
		for (auto obj : tile->ownedObjects)
		{
			if (obj->getType() == TileObjectBattleMapPart::convertType(p.second))
			{
				auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(obj)->getOwner();
				collapseList->insert(mp.get());
			}
		}
	}
	return collapseList;
}

void BattleMapPart::ceaseSupportProvision()
{
	providesHardSupport = false;
	attemptReLinkSupports(getSupportedParts());
	supportedParts.clear();
	if (supportedItems)
	{
		for (auto obj : this->tileObject->getOwningTile()->ownedObjects)
		{
			if (obj->getType() == TileObject::Type::Item)
			{
				std::static_pointer_cast<TileObjectBattleItem>(obj)->getItem()->tryCollapse();
			}
		}
		supportedItems = false;
	}
}

void BattleMapPart::attemptReLinkSupports(sp<std::set<BattleMapPart*>> set)
{
	if (set->empty())
	{
		return;
	}

	UString log = "ATTEMPTING RE-LINK OF SUPPORTS";

	// First mark all those in list as about to fall
	for (auto mp : *set)
	{
		mp->ticksUntilCollapse = 4;
		mp->providesHardSupport = false;
	}

	// Then try to re-establish support links
	bool listChanged;
	do
	{
		LogWarning("%s", log.cStr());
		log = "";
		log += format("\nIteration begins. List contains %d items:", (int)set->size());
		for (auto mp : *set)
		{
			auto pos = mp->tileObject->getOwningTile()->position;
			log += format("\n %s at %d %d %d", mp->type.id, pos.x, pos.y, pos.z);
		}
		log += format("\n");

		auto nextSet = mksp<std::set<BattleMapPart*>>();
		listChanged = false;
		for (auto mp : *set)
		{
			auto supportedByThisMp = mp->getSupportedParts();
			for (auto newmp : *supportedByThisMp)
			{
				newmp->ticksUntilCollapse = mp->ticksUntilCollapse + 4;
				newmp->providesHardSupport = false;
			}
			auto pos = mp->tileObject->getOwningTile()->position;
			if (mp->findSupport())
			{
				log += format("\n Processing %s at %d %d %d: OK %s", mp->type.id, pos.x, pos.y, pos.z, mp->providesHardSupport ? "HARD" : "SOFT");
				{
					auto t = pos;
					auto &map = mp->tileObject->map;
					for (int x = t.x - 1; x <= t.x + 1; x++)
					{
						for (int y = t.y - 1; y <= t.y + 1; y++)
						{
							for (int z = t.z - 1; z <= t.z + 1; z++)
							{
								if (x < 0 || x >= map.size.x
									|| y < 0 || y >= map.size.y
									|| z < 0 || z >= map.size.z)
								{
									continue;
								}
								auto tile2 = map.getTile(x, y, z);
								for (auto &o2 : tile2->ownedObjects)
								{
									if (o2->getType() == TileObject::Type::Ground
										|| o2->getType() == TileObject::Type::Feature
										|| o2->getType() == TileObject::Type::LeftWall
										|| o2->getType() == TileObject::Type::RightWall)
									{
										auto mp2 = std::static_pointer_cast<TileObjectBattleMapPart>(o2)->getOwner();
										for (auto &p : mp2->supportedParts)
										{
											if (p.first == t && p.second == mp->type->type)
											{
												log += format("\n - Supported by %s at %d %d %d", mp2->type.id, x - t.x, y - t.y, z - t.z);
											}
										}
									}
								}
							}
						}
					}

				}
				mp->ticksUntilCollapse = 0;
				for (auto newmp : *supportedByThisMp)
				{
					newmp->ticksUntilCollapse = 0;
				}
				listChanged = true;
			}
			else
			{
				log += format("\n Processing %s at %d %d %d: FAIL, remains in next iteration", mp->type.id, pos.x, pos.y, pos.z);
				nextSet->insert(mp);
				mp->supportedParts.clear();
				for (auto newmp : *supportedByThisMp)
				{
					auto newpos = newmp->tileObject->getOwningTile()->position;
					log += format("\n - %s at %d %d %d added to next iteration", newmp->type.id, newpos.x, newpos.y, newpos.z);
					nextSet->insert(newmp);
					listChanged = true;
				}
			}
		} 
		log += format("\n");
		set = nextSet;
	} while (listChanged);

	LogWarning("%s", log.cStr());

	// At this point only those that should fall are left
	// They will fall when their time comes
	for (auto mp : *set)
	{
		auto pos = mp->tileObject->getOwningTile()->position;
		LogWarning("Map part with supported by type %d at %d %d %d is going to fall", (int)mp->type->supported_by, pos.x, pos.y, pos.z);
	}
}

void BattleMapPart::collapse()
{
	// If it's already falling or destroyed or supported do nothing
	if (falling || !tileObject)
	{
		return;
	}
	falling = true;
	ceaseSupportProvision();
	ceaseDoorFunction();
}

void BattleMapPart::update(GameState &state, unsigned int ticks)
{
	if (!tileObject)
	{
		return;
	}

	if (ticksUntilCollapse > 0)
	{
		if (ticksUntilCollapse > ticks)
		{
			ticksUntilCollapse -= ticks;
		}
		else
		{
			ticksUntilCollapse = 0;
			collapse();
		}
	}

	// Process falling
	if (falling)
	{
		auto fallTicksRemaining = ticks;
		auto newPosition = position;
		while (fallTicksRemaining-- > 0)
		{
			fallingSpeed += FALLING_ACCELERATION_MAP_PART;
			newPosition -= Vec3<float>{0.0f, 0.0f, (fallingSpeed / TICK_SCALE)} /
				VELOCITY_SCALE_BATTLE;
		}
		
		// Collision with this tile happens when map part moves from this tile to the next
		if (newPosition.z < 0 || floorf(newPosition.z) != floorf(position.z))
		{
			sp<BattleMapPart> rubble;
			for (auto &obj : tileObject->getOwningTile()->ownedObjects)
			{
				switch (obj->getType())
				{
					// If there's a live ground or map mart of our type here - die
					case TileObject::Type::Ground:
					case TileObject::Type::LeftWall:
					case TileObject::Type::RightWall:
					case TileObject::Type::Feature:
					{
						auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(obj)->getOwner();
						
						// Find if we collide into it
						if (mp->type->type == type->type || mp->type->type == BattleMapPartType::Type::Ground)
						{
							if (tileObject && mp->isAlive())
							{
								destroyed = true;
							}
						}
						
						// Find if we deposit rubble into it
						if ((type->type != BattleMapPartType::Type::Ground && mp->type->type == type->type)
							|| (type->type == BattleMapPartType::Type::Ground && mp->type->type == BattleMapPartType::Type::Feature))
						{
							if (mp->isAlive())
							{
								rubble = mp;
							}
						}

						break;
					}
					default:
						// Ignore other object types?
						break;
				}
			}

			if (destroyed)
			{
				if (!type->rubble.empty())
				{
					if (!rubble)
					{
						// If no rubble present - spawn rubble
						auto rubble = mksp<BattleMapPart>();
						Vec3<int> initialPosition = position;
						rubble->position = initialPosition;
						rubble->position += Vec3<float>(0.5f, 0.5f, 0.0f);
						rubble->type = type->rubble.front();
						state.current_battle->map_parts.push_back(rubble);
						state.current_battle->map->addObjectToMap(rubble);
						LogWarning("Implement smoke when rubble falls");
					}
					else
					{
						// If rubble present - increment it if possible
						auto it = std::find(type->rubble.begin(), type->rubble.end(), rubble->type);
						if (it != type->rubble.end() && ++it != type->rubble.end())
						{
							rubble->type = *it;
							rubble->setPosition(rubble->position);
							LogWarning("Implement smoke when rubble falls");
						}
					}
				}

				die(state);
				return;
			}
		}

		setPosition(newPosition);
		return;
	}

	// Animate non-doors
	if (!door && type->animation_frames.size() > 0)
	{
		animation_frame_ticks += ticks;
		animation_frame_ticks %= TICKS_PER_FRAME_MAP_PART * type->animation_frames.size();
	}
}

void BattleMapPart::setPosition(const Vec3<float> &pos)
{
	this->position = pos;
	if (!this->tileObject)
	{
		LogError("setPosition called on map part with no tile object");
		return;
	}
	else
	{
		this->tileObject->setPosition(pos);
	}
		
}

bool BattleMapPart::isAlive() const
{
	if (this->falling || this->destroyed || ticksUntilCollapse > 0)
		return false;
	return true;
}
}

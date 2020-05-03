#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battlehazard.h"
#include "game/state/battle/battleitem.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_battleitem.h"
#include "game/state/tilemap/tileobject_battlemappart.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "library/strings_format.h"
#include <algorithm>
#include <set>

namespace OpenApoc
{

void BattleMapPart::die(GameState &state, bool explosive, bool violently)
{
	if (violently)
	{
		if (type->explosion_power > 0)
		{
			state.current_battle->addExplosion(state, position, nullptr, type->explosion_type,
			                                   type->explosion_power,
			                                   type->explosion_depletion_rate, owner);
			// Make it die so that it doesn't blow up twice!
			explosive = false;
		}
	}

	bool mustCheckForObjective = type->missionObjective;

	// If falling just cease to be, do damage
	if (falling)
	{
		this->tileObject->removeFromMap();
		this->tileObject.reset();
		destroyed = true;
	}
	else
	{
		// Doodad
		switch (type->type)
		{
			case BattleMapPartType::Type::Ground:
				// No doodad for grounds
				break;
			case BattleMapPartType::Type::LeftWall:
				state.current_battle->placeDoodad({&state, "DOODAD_29_EXPLODING_TERRAIN"},
				                                  tileObject->getCenter() +
				                                      Vec3<float>(-0.5f, 0.0f, 0.0f));
				break;
			case BattleMapPartType::Type::RightWall:
				state.current_battle->placeDoodad({&state, "DOODAD_29_EXPLODING_TERRAIN"},
				                                  tileObject->getCenter() +
				                                      Vec3<float>(0.0f, -0.5f, 0.0f));
				break;
			case BattleMapPartType::Type::Feature:
				state.current_battle->placeDoodad({&state, "DOODAD_29_EXPLODING_TERRAIN"},
				                                  tileObject->getCenter());
				break;
		}

		// Replace with damaged
		if (type->damaged_map_part)
		{
			this->type = type->damaged_map_part;
			this->damaged = true;
		}
		// Destroy
		else
		{
			// Don't destroy bottom tiles, else everything will leak out
			// Replace ground with destroyed
			if (this->position.z == 0 && this->type->type == BattleMapPartType::Type::Ground)
			{
				this->damaged = true;
				this->type = type->destroyed_ground_tile;
			}
			// Destroy map part
			else
			{
				if (explosive)
				{
					queueCollapse();
				}
				else
				{
					destroyed = true;
				}
			}
		}

		// Queue updates
		state.current_battle->queueVisionRefresh(position);
		state.current_battle->queuePathfindingRefresh(position);

		// Cease functioning
		ceaseBeingSupported();
		ceaseDoorFunction();
		ceaseSupportProvision();

		// Re-establish support for this if still alive
		if (isAlive())
		{
			if (!findSupport())
			{
				queueCollapse();
			}
		}
		// Destroy if destroyed
		else if (destroyed)
		{
			this->tileObject->removeFromMap();
			this->tileObject.reset();
		}
	}

	if (mustCheckForObjective)
	{
		state.current_battle->checkIfBuildingDisabled(state);
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
		return type->animation_frames.empty() ? -1
		                                      : animation_frame_ticks / TICKS_PER_FRAME_MAP_PART;
	}
}

bool BattleMapPart::applyBurning(GameState &state, int age)
{
	if (this->falling)
	{
		// Already falling, just continue
		return false;
	}
	if (!canBurn(age))
	{
		// Cannot burn, ignore
		return false;
	}

	burnTicksAccumulated += TICKS_PER_HAZARD_UPDATE * 2;
	if (!canBurn(age))
	{
		die(state);
	}
	return true;
}

bool BattleMapPart::canBurn(int age)
{
	// Explanation for how fire works is at the end of battlehazard.h
	int penetrativePower = std::min(255.0f, 3.0f * std::pow(2.0f, 9.0f - age / 10.0f));

	return penetrativePower > type->fire_resist && type->fire_burn_time < 255 &&
	       burnTicksAccumulated < type->fire_burn_time * (int)TICKS_PER_SECOND;
}

bool BattleMapPart::applyDamage(GameState &state, int power, StateRef<DamageType> damageType)
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

	int damage;
	if (damageType->explosive)
	{
		// Apparently Apoc uses 100% explosive damage instead of 50% like in UFO1&2
		damage = damageType->dealDamage(power, type->damageModifier);
	}
	else
	{
		// Apparently Apoc uses 50-150 damage model for shots vs terrain,
		// unlike UFO1&2, which used 25-75
		damage = randDamage050150(state.rng, damageType->dealDamage(power, type->damageModifier));
	}
	if (damage <= type->constitution)
	{
		return false;
	}

	// If we came this far, map part has been damaged and must cease to be
	die(state, damageType->explosive);
	return false;
}

bool BattleMapPart::handleCollision(GameState &state, Collision &c)
{
	return applyDamage(state, c.projectile->damage, c.projectile->damageType);
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

bool BattleMapPart::attachToSomething(bool checkType, bool checkHard)
{
	providesHardSupport = false;
	auto pos = tileObject->getOwningTile()->position;
	auto &map = tileObject->map;
	// Cling to ceiling
	if (pos.z == map.size.z - 1)
	{
		return true;
	}
	auto tileType = tileObject->getType();
	auto sft = shared_from_this();

	// List of directions (for ground and feature)
	static const std::list<Vec3<int>> directionGDFTList = {
	    {0, 0, -1}, {0, 0, 1}, {0, -1, 0}, {1, 0, 0}, {0, 1, 0}, {-1, 0, 0},
	};

	// List of directions for left wall
	static const std::list<Vec3<int>> directionLWList = {
	    {0, 0, -1}, {0, -1, 0}, {0, 1, 0}, {0, 0, 1}, {-1, 0, 0}, {0, 0, 0},
	};

	// List of directions for right wall
	static const std::list<Vec3<int>> directionRWList = {
	    {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, -1, 0}, {0, 0, 0},
	};

	auto &directionList =
	    tileType == TileObject::Type::LeftWall
	        ? directionLWList
	        : (tileType == TileObject::Type::RightWall ? directionRWList : directionGDFTList);

	// Search for map parts
	for (auto &dir : directionList)
	{
		int x = pos.x + dir.x;
		int y = pos.y + dir.y;
		int z = pos.z + dir.z;
		if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y || z < 0 || z >= map.size.z)
		{
			continue;
		}
		auto tile = map.getTile(x, y, z);
		for (auto &o : tile->ownedObjects)
		{
			if (o->getType() != tileType && checkType)
			{
				continue;
			}
			// Even if we're not doing type checking, we cannot allow for walls to cling to other
			// type of walls through gaps
			if (tileType == TileObject::Type::RightWall)
			{
				if ((o->getType() == TileObject::Type::LeftWall && x < pos.x) ||
				    (o->getType() == TileObject::Type::RightWall && y < pos.y))
				{
					continue;
				}
			}
			if (tileType == TileObject::Type::LeftWall)
			{
				if ((o->getType() == TileObject::Type::RightWall && y < pos.y) ||
				    (o->getType() == TileObject::Type::LeftWall && x < pos.x))
				{
					continue;
				}
			}
			if (o->getType() == TileObject::Type::Ground ||
			    o->getType() == TileObject::Type::Feature ||
			    o->getType() == TileObject::Type::LeftWall ||
			    o->getType() == TileObject::Type::RightWall)
			{
				auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
				if (mp != sft && mp->isAlive())
				{
					bool canSupport =
					    !mp->damaged && (mp->providesHardSupport || !checkHard) &&
					    (mp->type->type != BattleMapPartType::Type::Ground || z == pos.z) &&
					    (mp->type->provides_support || z >= pos.z);
					if (canSupport)
					{
						mp->supportedParts.emplace_back(position, type->type);
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool BattleMapPart::findSupport(bool allowClinging)
{
	std::ignore = allowClinging;
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
	if (pos.z == map.size.z - 1 && type->supportedByAbove)
	{
		return true;
	}
	auto tileType = tileObject->getType();
	auto sft = shared_from_this();

	// There are several ways battle map part can get supported:
	//
	// (following conditions provide "hard" support)
	//
	// Ground:
	//  - G1: Feature Below
	//  - G2: Wall Adjacent Below
	//  - G3: Feature Adjacent Below
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
	// Then, there is a specified "Supported By Direction" condition:
	//  - Ground will get support from a Ground only
	//  - Feature will get support from a Feature or a matching perpendicular Wall
	//    (Right if N/S, Left if E/W)
	//  - Wall will get support from the same type of Wall
	//	  (provided the Wall's type matches direction: Left for N/S, Right for E/W)
	//
	// If "Supported By Type: is also specified, then:
	//  - Ground/Wall allows support from Ground/Wall on a current level
	//  - Feature allows support from Feature on a level below
	//
	// (following conditions provide "soft" support)
	//
	// Then, object with no direction specified can cling to two adjacent objects:
	//  - Ground and Feature can cling to objects of the same type
	//  - Walls can cling to walls of their type or a Feature
	//
	// Finally, every UNDAMAGED map part can be supported if it has established support lines
	// on both sides that connect to an object providing "hard" support
	//  - Object "shoots" a line in both directions and as long as there is an object on every tile
	//    the line continues, and if an object providing hard support is reached,
	//	  then "soft" support can be attained
	//
	// Implementation:
	//  - We will first check for special conditions
	//  - Then we will gather information about adjacent map parts and check for "Supported By"
	//  - Finally we will try to cling to two objects of the same type
	//  - If all fails, we will scan on axes in search of distant support

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
	// Do the check
	for (int x = startX; x <= endX; x++)
	{
		for (int y = startY; y <= endY; y++)
		{
			for (int z = startZ; z <= endZ; z++)
			{
				if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y || z < 0 ||
				    z >= map.size.z)
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
							//  - G1: Feature Below
							canSupport = canSupport || (x == pos.x && y == pos.y && z < pos.z &&
							                            o->getType() == TileObject::Type::Feature);
							//  - G2: Wall Adjacent Below
							if ((x >= pos.x || y >= pos.y) && z < pos.z)
							{
								canSupport =
								    canSupport ||
								    (x >= pos.x && o->getType() == TileObject::Type::LeftWall);
								canSupport =
								    canSupport ||
								    (y >= pos.y && o->getType() == TileObject::Type::RightWall);
							}
							//  - G3: Feature Adjacent Below
							if ((x == pos.x || y == pos.y) && z < pos.z)
							{
								canSupport =
								    canSupport || (o->getType() == TileObject::Type::Feature);
							}
							break;
						case BattleMapPartType::Type::Feature:
							//  - F1: Ground Current
							canSupport = canSupport ||
							             (z == pos.z && o->getType() == TileObject::Type::Ground);
							//  - F2: Feature Below
							canSupport = canSupport ||
							             (z < pos.z && o->getType() == TileObject::Type::Feature);
							//  - F3: Feature Above (if "Above" supported by)
							canSupport = canSupport ||
							             (z > pos.z && o->getType() == TileObject::Type::Feature &&
							              type->supportedByAbove);
							break;
						case BattleMapPartType::Type::LeftWall:
							//  - W1: Feature Current
							canSupport = canSupport || (x == pos.x && y == pos.y && z == pos.z &&
							                            o->getType() == TileObject::Type::Feature);
							//  - W2: Ground Adjacent Current
							canSupport = canSupport ||
							             (z == pos.z && o->getType() == TileObject::Type::Ground);
							//  - W3: Feature Adjacent Below
							canSupport = canSupport ||
							             (z < pos.z && o->getType() == TileObject::Type::Feature);
							//  - W4: Wall Below
							canSupport = canSupport || (x == pos.x && z < pos.z &&
							                            o->getType() == TileObject::Type::LeftWall);
							//  - W5: Wall Above (if "Above" supported by)
							canSupport =
							    canSupport || (x == pos.x && z > pos.z &&
							                   o->getType() == TileObject::Type::LeftWall &&
							                   type->supportedByAbove);
							break;
						case BattleMapPartType::Type::RightWall:
							//  - W1: Feature Current
							canSupport = canSupport || (x == pos.x && y == pos.y && z == pos.z &&
							                            o->getType() == TileObject::Type::Feature);
							//  - W2: Ground Adjacent Current
							canSupport = canSupport ||
							             (z == pos.z && o->getType() == TileObject::Type::Ground);
							//  - W3: Feature Adjacent Below
							canSupport = canSupport ||
							             (z < pos.z && o->getType() == TileObject::Type::Feature);
							//  - W4: Wall Below
							canSupport =
							    canSupport || (y == pos.y && z < pos.z &&
							                   o->getType() == TileObject::Type::RightWall);
							//  - W5: Wall Above (if "Above" supported by)
							canSupport =
							    canSupport || (y == pos.y && z > pos.z &&
							                   o->getType() == TileObject::Type::RightWall &&
							                   type->supportedByAbove);
							break;
					}

					if (canSupport)
					{
						auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
						// Seems that "provide support" flag only matters for providing support
						// upwards
						if (mp != sft && mp->isAlive() &&
						    (!mp->damaged || mp->type->type == BattleMapPartType::Type::Ground) &&
						    (mp->type->type != BattleMapPartType::Type::Ground || z == pos.z) &&
						    (mp->type->provides_support || z >= pos.z))
						{
							mp->supportedParts.emplace_back(position, type->type);
							return true;
						}
					}
				}
			}
		}
	}

	// Step 02: Check "supported by direction" condition
	if (!type->supportedByDirections.empty())
	{
		// List of types to search for and locations where
		std::list<std::pair<Vec3<int>, TileObject::Type>> partList;
		// List for types to search for and z-where
		std::list<std::pair<int, TileObject::Type>> typeList;
		// Every type is supported by it's type on the same level
		typeList.emplace_back(0, tileType);
		// Add other supported by types
		for (auto &t : type->supportedByTypes)
		{
			switch (t)
			{
				case BattleMapPartType::Type::Ground:
					typeList.emplace_back(0, TileObject::Type::Ground);
					break;
				case BattleMapPartType::Type::LeftWall:
					typeList.emplace_back(0, TileObject::Type::LeftWall);
					break;
				case BattleMapPartType::Type::RightWall:
					typeList.emplace_back(0, TileObject::Type::RightWall);
					break;
				case BattleMapPartType::Type::Feature:
					typeList.emplace_back(-1, TileObject::Type::Feature);
					break;
			}
		}
		// Fill parts list based on direction
		for (auto &d : type->supportedByDirections)
		{
			for (auto &p : typeList)
			{
				// Feature to feature on the same level also allows for a matching wall
				if (type->type == BattleMapPartType::Type::Feature && p.first == 0 &&
				    p.second == TileObject::Type::Feature)
				{
					switch (d)
					{
						case MapDirection::North:
							partList.emplace_back(Vec3<int>{pos.x, pos.y, pos.z + p.first},
							                      TileObject::Type::RightWall);
							break;
						case MapDirection::East:
							partList.emplace_back(Vec3<int>{pos.x + 1, pos.y, pos.z + p.first},
							                      TileObject::Type::LeftWall);
							break;
						case MapDirection::South:
							partList.emplace_back(Vec3<int>{pos.x, pos.y + 1, pos.z + p.first},
							                      TileObject::Type::RightWall);
							break;
						case MapDirection::West:
							partList.emplace_back(Vec3<int>{pos.x, pos.y, pos.z + p.first},
							                      TileObject::Type::LeftWall);
							break;
						case MapDirection::Up:
							[[fallthrough]];
						case MapDirection::Down:
							// Up and Down don't have map parts
							break;
					}
				}

				// Going N/S for Right Wall or E/W for Left Wall is impossible for same type walls
				if ((p.second == TileObject::Type::RightWall &&
				     tileType == TileObject::Type::RightWall &&
				     (d == MapDirection::North || d == MapDirection::South)) ||
				    (p.second == TileObject::Type::LeftWall &&
				     tileType == TileObject::Type::LeftWall &&
				     (d == MapDirection::East || d == MapDirection::West)))
				{
					continue;
				}
				// Going North into Right Wall and West into Left Wall means checking our own tile
				// (South for Right and East for Left is fine))
				int negInc = -1;
				if ((d == MapDirection::North && p.second == TileObject::Type::RightWall) ||
				    (d == MapDirection::West && p.second == TileObject::Type::LeftWall))
				{
					negInc = 0;
				}

				// Get part in this direction
				int dx = 0;
				int dy = 0;
				switch (d)
				{
					case MapDirection::North:
						dy = negInc;
						break;
					case MapDirection::East:
						dx = 1;
						break;
					case MapDirection::South:
						dy = 1;
						break;
					case MapDirection::West:
						dx = negInc;
						break;
					case MapDirection::Up:
						[[fallthrough]];
					case MapDirection::Down:
						// Up and Down don't have map parts
						break;
				}
				partList.emplace_back(Vec3<int>{pos.x + dx, pos.y + dy, pos.z + p.first}, p.second);

				// Get diagonal directions
				for (auto &d2 : type->supportedByDirections)
				{
					if (d2 == d || p.second == TileObject::Type::LeftWall ||
					    p.second == TileObject::Type::RightWall)
					{
						continue;
					}
					switch (d)
					{
						case MapDirection::North:
							[[fallthrough]];
						case MapDirection::South:
							switch (d)
							{
								case MapDirection::East:
									dx = 1;
									break;
								case MapDirection::West:
									dx = -1;
									break;
								case MapDirection::North:
									[[fallthrough]];
								case MapDirection::South:
									continue;
								case MapDirection::Up:
									[[fallthrough]];
								case MapDirection::Down:
									// Up and Down don't have map parts
									break;
							}
							break;
						case MapDirection::East:
							[[fallthrough]];
						case MapDirection::West:
							switch (d)
							{
								case MapDirection::North:
									dy = -1;
									break;
								case MapDirection::South:
									dy = 1;
									break;
								case MapDirection::East:
									[[fallthrough]];
								case MapDirection::West:
									continue;
								case MapDirection::Up:
									[[fallthrough]];
								case MapDirection::Down:
									// Up and Down don't have map parts
									break;
							}
							break;
						case MapDirection::Up:
							[[fallthrough]];
						case MapDirection::Down:
							// Up and Down don't have map parts
							break;
					}
					partList.emplace_back(Vec3<int>{pos.x + dx, pos.y + dy, pos.z + p.first},
					                      p.second);
				}
			}
		}
		// Look for parts
		for (auto &pair : partList)
		{
			if (pair.first.x < 0 || pair.first.x >= map.size.x || pair.first.y < 0 ||
			    pair.first.y >= map.size.y || pair.first.z < 0 || pair.first.z >= map.size.z)
			{
				continue;
			}
			auto tile = map.getTile(pair.first.x, pair.first.y, pair.first.z);
			for (auto &o : tile->ownedObjects)
			{
				// Matching battle map parts that fit the criteria of axis differences
				// Also must provide support
				if (o->getType() == pair.second)
				{
					auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
					if (mp != sft && mp->isAlive())
					{
						bool canSupport = !mp->damaged &&
						                  (mp->type->type != BattleMapPartType::Type::Ground ||
						                   pair.first.z == pos.z) &&
						                  (mp->type->provides_support || pair.first.z >= pos.z);
						if (canSupport)
						{
							mp->supportedParts.emplace_back(position, type->type);
							return true;
						}
					}
				}
			}
		}
	}

// DISABLED clinging to 2 neighbouring objects, because that can introduce circular references
// Besides, I *think* vanilla didn't have it that way
#if 0
	// If we reached this - we can not provide hard support
	providesHardSupport = false;

	// Step 03: Try to cling to two adjacent objects of the same type
	// (wall can also cling to feature)

	// List of four directions (for ground and feature)
	static const std::list<Vec3<int>> directionGDFTList = {
		{0, -1, 0}, {1, 0, 0}, {0, 1, 0}, {-1, 0, 0},
	};

	// List of directions for left wall
	static const std::list<Vec3<int>> directionLWList = {
		{0, -1, 0}, {0, 1, 0},
	};

	// List of directions for right wall
	static const std::list<Vec3<int>> directionRWList = {
		{1, 0, 0}, {-1, 0, 0},
	};

	auto &directionList =
		tileType == TileObject::Type::LeftWall
		? directionLWList
		: (tileType == TileObject::Type::RightWall ? directionRWList : directionGDFTList);

	// List of found map parts to cling on to
	std::list<sp<BattleMapPart>> supports;
	// Search for map parts
	for (auto &dir : directionList)
	{
		int x = pos.x + dir.x;
		int y = pos.y + dir.y;
		int z = pos.z + dir.z;
		if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y || z < 0 || z >= map.size.z)
		{
			continue;
		}
		auto tile = map.getTile(x, y, z);
		for (auto &o : tile->ownedObjects)
		{
			if (o->getType() == tileType || (o->getType() == TileObject::Type::Feature &&
				(tileType == TileObject::Type::LeftWall ||
					tileType == TileObject::Type::RightWall)))
			{
				auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
				if (mp != sft && mp->isAlive())
				{
					bool canSupport =
						!mp->damaged &&
						(mp->type->type != BattleMapPartType::Type::Ground || z == pos.z) &&
						(mp->type->provides_support || z >= pos.z);
					if (canSupport)
					{
						supports.emplace_back(mp);
						// No need to further look in this area
						break;
					}
				}
			}
		}
	}
	// Calculate if we have enough supports (map edge counts as support)
	auto supportCount = supports.size();
	if (pos.x == 0 || pos.x == map.size.x - 1)
	{
		supportCount++;
	}
	if (pos.y == 0 || pos.y == map.size.y - 1)
	{
		supportCount++;
	}
	// Get support if we have enough
	if (supportCount >= 2)
	{
		for (auto &mp : supports)
		{
			mp->supportedParts.emplace_back(position, type->type);
		}
		return true;
	}
#endif

	// Step 04: Shoot "support lines" and try to find something

	// Scan on X
	if (type->type != BattleMapPartType::Type::LeftWall && !damaged)
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
				// Found map part that provides hard support and won't collapse
				if (mp->providesHardSupport && !mp->willCollapse())
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
	if (type->type != BattleMapPartType::Type::RightWall && !damaged)
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
				// We ignore those that have positive "ticksUntilCollapse" as those can be saved yet
				if (!mp || mp->destroyed || mp->damaged || mp->falling)
				{
					// fail
					break;
				}
				// Found map part that provides hard support and won't collapse
				if (mp->providesHardSupport && !mp->willCollapse())
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

	providesHardSupport = false;
	return false;
}

sp<std::set<SupportedMapPart *>> BattleMapPart::getSupportedParts()
{
	sp<std::set<SupportedMapPart *>> supportedMapParts = mksp<std::set<SupportedMapPart *>>();
	auto &map = tileObject->map;
	// Since we reference supported parts by type we have to find each in it's tile by type
	for (auto &p : supportedParts)
	{
		auto tile = map.getTile(p.first);
		for (auto &obj : tile->ownedObjects)
		{
			if (obj->getType() == TileObjectBattleMapPart::convertType(p.second))
			{
				auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(obj)->getOwner();
				if (mp->destroyed)
				{
					continue;
				}
				supportedMapParts->insert(mp.get());
			}
		}
	}
	return supportedMapParts;
}

void BattleMapPart::clearSupportedParts() { supportedParts.clear(); }

void BattleMapPart::ceaseBeingSupported()
{
	auto pos = tileObject->getOwningTile()->position;
	auto &map = tileObject->map;

	// Clean support providers for this map part
	for (int x = pos.x - 1; x <= pos.x + 1; x++)
	{
		for (int y = pos.y - 1; y <= pos.y + 1; y++)
		{
			for (int z = pos.z - 1; z <= pos.z + 1; z++)
			{
				if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y || z < 0 ||
				    z >= map.size.z)
				{
					continue;
				}
				auto tile = map.getTile(x, y, z);
				for (auto &o : tile->ownedObjects)
				{
					if (o->getType() == TileObject::Type::Ground ||
					    o->getType() == TileObject::Type::Feature ||
					    o->getType() == TileObject::Type::LeftWall ||
					    o->getType() == TileObject::Type::RightWall)
					{
						auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
						auto it = mp->supportedParts.begin();
						while (it != mp->supportedParts.end())
						{
							auto &p = *it;
							if (p.first == pos && p.second == type->type)
							{
								it = mp->supportedParts.erase(it);
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
}

void BattleMapPart::ceaseSupportProvision()
{
	providesHardSupport = false;
	attemptReLinkSupports(getSupportedParts());
	supportedParts.clear();
	if (supportedItems)
	{
		for (auto &obj : this->tileObject->getOwningTile()->ownedObjects)
		{
			if (obj->getType() == TileObject::Type::Item)
			{
				std::static_pointer_cast<TileObjectBattleItem>(obj)->getItem()->tryCollapse();
			}
		}
		supportedItems = false;
	}
}

void BattleMapPart::collapse(GameState &state)
{
	// If it's already falling or destroyed or supported do nothing
	if (falling || !tileObject)
	{
		return;
	}
	// Level 0 can't collapse
	if (this->position.z == 0 && this->type->type == BattleMapPartType::Type::Ground)
	{
		this->damaged = true;
		this->type = type->destroyed_ground_tile;
	}
	else
	{
		falling = true;
		state.current_battle->queueVisionRefresh(position);
		state.current_battle->queuePathfindingRefresh(position);
		// Note: Pathfinding refresh relies on tile's battlescape parameters being updated
		// before it happens, so that battlescape parameters already account for the
		// now dysfunctional map part. Pathfinding update will only happen
		// after we call setPosition() on the map part, which will
		// call update to the battlescape parameters of the tile, which will
		// in turn make us ignore the falling map part properly
		// If we would somehow call collapse() in a way that would set falling to true but
		// would not trigger the setPosition() afterwards, this logic would fail
	}
	ceaseBeingSupported();
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
			collapse(state);
		}
	}

	// Process falling
	if (falling)
	{
		updateFalling(state, ticks);
		return;
	}

	// Animate non-doors
	if (!door && type->animation_frames.size() > 0)
	{
		animation_frame_ticks += ticks;
		animation_frame_ticks %= TICKS_PER_FRAME_MAP_PART * type->animation_frames.size();
	}
}

void BattleMapPart::updateFalling(GameState &state, unsigned int ticks)
{
	auto fallTicksRemaining = ticks;
	auto newPosition = position;
	while (fallTicksRemaining-- > 0)
	{
		fallingSpeed += FALLING_ACCELERATION_MAP_PART;
		newPosition -= Vec3<float>{0.0f, 0.0f, (fallingSpeed / TICK_SCALE)} / VELOCITY_SCALE_BATTLE;
	}

	// Collision with this tile happens when map part moves from this tile to the next
	if (newPosition.z < 0 || floorf(newPosition.z) != floorf(position.z))
	{
		sp<BattleMapPart> rubble;
		// we may kill a unit by applying fall damage, this will trigger a stance change which will
		// remove its tile and shadow objects from the ownedObjects set (invalidating iterators)
		// we work around this by iterating over a set's copy
		auto set = tileObject->getOwningTile()->ownedObjects;
		for (auto &obj : set)
		{
			if (tileObject->getOwningTile()->ownedObjects.find(obj) ==
			    tileObject->getOwningTile()->ownedObjects.end())
			{
				continue;
			}
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
					if (mp->type->type == type->type ||
					    mp->type->type == BattleMapPartType::Type::Ground)
					{
						if (tileObject && mp->isAlive())
						{
							destroyed = true;
						}
					}

					// Find if we deposit rubble into it
					if ((type->type != BattleMapPartType::Type::Ground &&
					     mp->type->type == type->type) ||
					    (type->type == BattleMapPartType::Type::Ground &&
					     mp->type->type == BattleMapPartType::Type::Feature))
					{
						if (mp->isAlive())
						{
							rubble = mp;
						}
					}

					break;
				}
				case TileObject::Type::Unit:
				{
					auto u = std::static_pointer_cast<TileObjectBattleUnit>(obj)->getUnit();
					// FIXME: Ensure falling damage is correct
					u->applyDamage(state, FALLING_MAP_PART_DAMAGE_TO_UNIT,
					               {&state, "DAMAGETYPE_FALLING_OBJECT"}, BodyPart::Helmet,
					               DamageSource::Impact);
					break;
				}
				default:
					// Ignore other object types?
					break;
			}
		}
		// Spawn smoke, more intense if we land here
		{
			StateRef<DamageType> dtSmoke = {&state, "DAMAGETYPE_SMOKE"};
			auto hazard = state.current_battle->placeHazard(
			    state, owner, nullptr, dtSmoke, position, dtSmoke->hazardType->getLifetime(state),
			    2, destroyed ? 6 : 12);
			if (hazard)
			{
				hazard->ticksUntilVisible = 0;
			}
		}
		// Cease to exist if destroyed
		if (destroyed)
		{
			if (!type->rubble.empty())
			{
				if (!rubble)
				{
					// If no rubble present - spawn rubble
					auto rubble = mksp<BattleMapPart>();
					Vec3<int> initialPosition = position;
					rubble->damaged = true;
					rubble->owner = owner;
					rubble->position = initialPosition;
					rubble->position += Vec3<float>(0.5f, 0.5f, 0.0f);
					rubble->type = type->rubble.front();
					state.current_battle->map_parts.push_back(rubble);
					state.current_battle->map->addObjectToMap(rubble);
				}
				else
				{
					// If rubble present - increment it if possible
					auto it = std::find(type->rubble.begin(), type->rubble.end(), rubble->type);
					if (it != type->rubble.end() && ++it != type->rubble.end())
					{
						rubble->type = *it;
						rubble->setPosition(state, rubble->position);
					}
				}
			}

			die(state);
			return;
		}
	}

	setPosition(state, newPosition);
}

Vec3<int> BattleMapPart::getTilePosition() const { return tileObject->getOwningTile()->position; }

const TileMap &BattleMapPart::getMap() const { return tileObject->map; }

UString BattleMapPart::getId() const { return type.id; }

int BattleMapPart::getType() const { return (int)type->type; }

UString BattleMapPart::getSupportString() const { return providesHardSupport ? "HARD" : "SOFT"; }

void BattleMapPart::setPosition(GameState &, const Vec3<float> &pos)
{
	position = pos;
	if (!this->tileObject)
	{
		LogError("setPosition called on map part with no tile object");
		return;
	}

	this->tileObject->setPosition(pos);
}

bool BattleMapPart::isAlive() const
{
	if (falling || destroyed || willCollapse())
		return false;
	return true;
}

void BattleMapPart::queueCollapse(unsigned additionalDelay)
{
	ticksUntilCollapse = TICKS_MULTIPLIER + additionalDelay;
	providesHardSupport = false;
}

void BattleMapPart::cancelCollapse() { ticksUntilCollapse = 0; }
} // namespace OpenApoc

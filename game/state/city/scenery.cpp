#include "game/state/city/scenery.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/city/agentmission.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/citycommonsamplelist.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/shared/doodad.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_doodad.h"
#include "game/state/tilemap/tileobject_scenery.h"
#include "game/state/tilemap/tileobject_vehicle.h"
#include <limits.h>

namespace OpenApoc
{

void Scenery::ceaseSupportProvision()
{
	attemptReLinkSupports(getSupportedParts());
	supportedParts.clear();
}

void Scenery::queueCollapse(unsigned additionalDelay)
{
	ticksUntilCollapse = TICKS_MULTIPLIER + additionalDelay;
}

void Scenery::cancelCollapse() { ticksUntilCollapse = 0; }

sp<std::set<SupportedMapPart *>> Scenery::getSupportedParts()
{
	sp<std::set<SupportedMapPart *>> supportedMapParts = mksp<std::set<SupportedMapPart *>>();
	auto &map = tileObject->map;
	// Since we reference supported parts by type we have to find each in it's tile by type
	for (auto &p : supportedParts)
	{
		auto tile = map.getTile(p);
		for (auto &obj : tile->ownedObjects)
		{
			if (obj->getType() == TileObject::Type::Scenery)
			{
				auto mp = std::static_pointer_cast<TileObjectScenery>(obj)->getOwner();
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

void Scenery::clearSupportedParts() { supportedParts.clear(); }

bool Scenery::attachToSomething()
{
	auto pos = tileObject->getOwningTile()->position;
	supportHardness = -10;
	auto &map = tileObject->map;
	auto thisPtr = shared_from_this();

	int startX = pos.x - 1;
	int endX = pos.x + 1;
	int startY = pos.y - 1;
	int endY = pos.y + 1;
	int startZ = pos.z - 1;
	int endZ = pos.z;
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
					if (o->getType() == TileObject::Type::Scenery)
					{
						auto mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();

						if (mp == thisPtr || !mp->isAlive())
						{
							continue;
						}
						mp->supportedParts.insert(currentPosition);
						supportedBy.emplace_back(mp->currentPosition);
						return true;
					}
				}
			} // For every x and y and z
		}
	}
	return false;
}

bool Scenery::findSupport(bool allowClinging)
{
	auto pos = tileObject->getOwningTile()->position;
	supportHardness = 1;
	if (pos.z <= 1)
	{
		return true;
	}
	auto &map = tileObject->map;
	auto thisPtr = shared_from_this();

	// Forward lookup for adding increments
	static const std::map<int, Vec2<int>> intToVec = {
	    {0, {0, -1}}, {1, {1, 0}}, {2, {0, 1}}, {3, {-1, 0}}};
	// Forward lookup for connection/hill checking
	static const std::map<Vec2<int>, int> vecToIntForward = {
	    {{0, -1}, 0}, {{1, 0}, 1}, {{0, 1}, 2}, {{-1, 0}, 3}};
	// Reverse lookup for connection checking
	static const std::map<Vec2<int>, int> vecToIntBack = {
	    {{0, -1}, 2}, {{1, 0}, 3}, {{0, 1}, 0}, {{-1, 0}, 1}};

	// Scenery gets supported in a different way than battle map parts
	//
	// - When first checked it finds what supports it in a generous way
	// and remembers it
	// - Then, for the remainder of its life, it will only accept support
	// from that location, and nothing else.
	// - This means that, for example, hanging building part will be supported by adjacent part
	// However if it was originally supported by the part below, it won't re-support when
	// part below dies, but will come crashing down
	// - However, roads and tubes are an exception and will try to re-establish their "supportedby"

	// Scenery gets (hard) support in the following way:
	//
	// - The only way to hard-support is by having non-road/tube below
	//   - Only tubes can be supported on tubes this way
	//     (in this case tube looking for support must have a downward connection)
	//	 - Only WalkMode::None Road can give support this way
	//   - Only non-WalkMode::Into General can give support this way
	// - The exception is that tube can be supported by tube below if connected
	//
	// (following conditions provide "soft" support)
	//
	// - If allowClinging is true
	//   - General/Wall/Junk can cling to one adjacent "hard" supported General/Wall
	//   - General/Wall/Junk can cling to two adjacent "soft" supported General/Wall/Junk
	//   - General can cling to one General above it
	//   - General can cling to two Generals adjacent, one above one below on same side (xy)
	//	 - Wall can cling to two adjacent Walls below it
	//   - People tube can cling onto adjacent "hard" General, adhering to its direction
	//
	// Finally, can be supported if it has established support lines
	// on both sides that connect to an object providing support (any kind)
	//  - Object "shoots" a line in both directions and as long as there is an object on every tile
	//    the line continues, and if an object providing hard support is reached,
	//	  then "soft" support can be attained
	//

	// Step 01: Check for existing support conditions
	auto lastSupportedBy = supportedBy;
	if (!supportedBy.empty())
	{
		std::set<sp<Scenery>> supports;
		for (auto &p : supportedBy)
		{
			auto tile = map.getTile(p);
			for (auto &o : tile->ownedObjects)
			{
				if (o->getType() == TileObject::Type::Scenery)
				{
					auto mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();

					if (mp == thisPtr || !mp->isAlive())
					{
						continue;
					}
					supports.insert(mp);
					break;
				}
			}
		}
		// Only 2 supports ever required
		if (supports.size() >= std::min(supportedBy.size(), (size_t)2))
		{
			for (auto &mp : supports)
			{
				mp->supportedParts.insert(currentPosition);
			}
			return true;
		}
		// Roads and tubes attempt re-support
		switch (type->tile_type)
		{
			case SceneryTileType::TileType::PeopleTube:
			case SceneryTileType::TileType::Road:
				supportedBy.clear();
				break;
			case SceneryTileType::TileType::CityWall:
			case SceneryTileType::TileType::General:
			case SceneryTileType::TileType::PeopleTubeJunction:
				return false;
		}
	}

	// Step 02: Check below
	{
		// Expecting to always have space below in city
		auto tile = map.getTile(currentPosition.x, currentPosition.y, currentPosition.z - 1);
		for (auto &o : tile->ownedObjects)
		{
			if (o->getType() == TileObject::Type::Scenery)
			{
				auto mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();
				// Can't be supported by dead or yourself
				if (mp == thisPtr || !mp->isAlive())
				{
					continue;
				}
				// Tube can only get support if it has connection directed below
				if (type->tile_type == SceneryTileType::TileType::PeopleTube && !type->tube[5])
				{
					continue;
				}
				// Tubes can only give support to tubes
				if (mp->type->tile_type == SceneryTileType::TileType::PeopleTube)
				{
					// If ain't tube or no connection down from us or no connection up from below
					if (type->tile_type != SceneryTileType::TileType::PeopleTube ||
					    !mp->type->tube[4] || !type->tube[5])
					{
						continue;
					}
				}
				// Roads can only give support if they have WalkMode::None
				if (mp->type->tile_type == SceneryTileType::TileType::Road &&
				    mp->type->walk_mode != SceneryTileType::WalkMode::None)
				{
					continue;
				}
				// General can only give support if they have WalkMode::Into
				if (mp->type->tile_type == SceneryTileType::TileType::General &&
				    mp->type->walk_mode == SceneryTileType::WalkMode::Into)
				{
					continue;
				}
				mp->supportedParts.insert(currentPosition);
				supportedBy.emplace_back(mp->currentPosition);
				return true;
			}
		}
	}

	// At this point our support will be soft

	// Step 03.01: Check adjacents (for General/Wall !INTO)
	if (allowClinging)
	{
		if (type->tile_type == SceneryTileType::TileType::General ||
		    type->tile_type == SceneryTileType::TileType::CityWall ||
		    type->tile_type == SceneryTileType::TileType::PeopleTubeJunction)
		{
			std::set<sp<Scenery>> supports;
			int startX = pos.x - 1;
			int endX = pos.x + 1;
			int startY = pos.y - 1;
			int endY = pos.y + 1;
			int z = pos.z;
			for (int x = startX; x <= endX; x++)
			{
				for (int y = startY; y <= endY; y++)
				{
					if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y)
					{
						continue;
					}
					// Cannot support diagonally on xy
					if (x != pos.x && y != pos.y)
					{
						continue;
					}
					auto tile = map.getTile(x, y, z);
					for (auto &o : tile->ownedObjects)
					{
						if (o->getType() == TileObject::Type::Scenery)
						{
							auto mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();

							if (mp == thisPtr || !mp->isAlive())
							{
								continue;
							}
							// Must not be a tube or road
							if (mp->type->tile_type == SceneryTileType::TileType::Road ||
							    mp->type->tile_type == SceneryTileType::TileType::PeopleTube)
							{
								continue;
							}
							// Must be a matching(Wall for Wall, General/Junk for General/Junk)
							if (mp->type->tile_type == SceneryTileType::TileType::CityWall &&
							    type->tile_type != SceneryTileType::TileType::CityWall)
							{
								continue;
							}
							// Remember in case there's two of them soft supported
							supports.insert(mp);
							// Cannot be supported by single adjacent people's tube junk
							if (mp->type->tile_type ==
							    SceneryTileType::TileType::PeopleTubeJunction)
							{
								continue;
							}
							// Cannot be supported by single adjacent non-hard
							if (mp->supportHardness <= 0)
							{
								continue;
							}
							// Is supported!
							supportHardness = mp->supportHardness - 1;
							mp->supportedParts.insert(currentPosition);
							supportedBy.emplace_back(mp->currentPosition);
							return true;
						}
					}
				}
			}
			// Found two or more soft supports:
			// - Cling to the best two of them
			// - Our hardness is one less than weakest of them
			if (supports.size() > 1)
			{
				std::set<sp<Scenery>> bestSupports;
				int hadrness;
				for (hadrness = 1; hadrness > INT_MIN; hadrness--)
				{
					std::set<sp<Scenery>> newSupports;
					for (auto &mp : supports)
					{
						if (mp->supportHardness == hadrness)
						{
							bestSupports.insert(mp);
							newSupports.insert(mp);
							if (bestSupports.size() > 1)
							{
								break;
							}
						}
					}
					if (bestSupports.size() > 1)
					{
						break;
					}
					for (auto &mp : newSupports)
					{
						supports.erase(mp);
					}
				}

				supportHardness = hadrness - 1;
				for (auto &mp : bestSupports)
				{
					mp->supportedParts.insert(currentPosition);
					supportedBy.emplace_back(mp->currentPosition);
				}
				return true;
			}
		}
		// Step 03.02: Check above (for General)
		if (type->tile_type == SceneryTileType::TileType::General)
		{
			// Only check one tile here
			do // Provides a "continue/break" exit as in similar loops around
			{
				int x = pos.x;
				int y = pos.y;
				int z = pos.z + 1;
				// Cannot support diagonally on xy
				if (z >= map.size.z)
				{
					continue;
				}
				auto tile = map.getTile(x, y, z);
				for (auto &o : tile->ownedObjects)
				{
					if (o->getType() == TileObject::Type::Scenery)
					{
						auto mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();

						if (mp == thisPtr || !mp->isAlive())
						{
							continue;
						}
						// Must be a general
						if (mp->type->tile_type != SceneryTileType::TileType::General)
						{
							continue;
						}
						// Is supported!
						supportHardness = mp->supportHardness - 1;
						mp->supportedParts.insert(currentPosition);
						supportedBy.emplace_back(mp->currentPosition);
						return true;
					}
				}
			} while (false);
		}
		// Step 03.03: Check adjacent above and below (for General)
		if (type->tile_type == SceneryTileType::TileType::General)
		{
			int startX = pos.x - 1;
			int endX = pos.x + 1;
			int startY = pos.y - 1;
			int endY = pos.y + 1;
			int startZ = pos.z - 1;
			int endZ = pos.z + 1;
			std::list<int> listZ = {startZ, endZ};
			for (int x = startX; x <= endX; x++)
			{
				for (int y = startY; y <= endY; y++)
				{
					// Cannot support diagonally on xy
					if (x != pos.x && y != pos.y)
					{
						continue;
					}
					// Find two supports
					std::set<sp<Scenery>> supports;
					for (auto &z : listZ)
					{
						if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y || z < 0 ||
						    z >= map.size.z)
						{
							continue;
						}
						auto tile = map.getTile(x, y, z);
						for (auto &o : tile->ownedObjects)
						{
							if (o->getType() == TileObject::Type::Scenery)
							{
								auto mp =
								    std::static_pointer_cast<TileObjectScenery>(o)->getOwner();

								if (mp == thisPtr || !mp->isAlive())
								{
									continue;
								}
								// Must be a general
								if (mp->type->tile_type != SceneryTileType::TileType::General)
								{
									continue;
								}
								supports.insert(mp);
							}
						}
					}
					// Found two supports:
					if (supports.size() == 2)
					{
						for (auto &mp : supports)
						{
							supportHardness = std::min(supportHardness, mp->supportHardness);
							mp->supportedParts.insert(currentPosition);
							supportedBy.emplace_back(mp->currentPosition);
						}
						return true;
					}
				} // For every x and y
			}
		}
		// Step 03.04: Check adjacent below (for Wall)
		if (type->tile_type == SceneryTileType::TileType::CityWall)
		{
			int startX = pos.x - 1;
			int endX = pos.x + 1;
			int startY = pos.y - 1;
			int endY = pos.y + 1;
			int z = pos.z - 1;
			// Find two supports
			std::set<sp<Scenery>> supports;
			for (int x = startX; x <= endX; x++)
			{
				for (int y = startY; y <= endY; y++)
				{
					// Cannot support diagonally on xy
					if (x != pos.x && y != pos.y)
					{
						continue;
					}
					if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y)
					{
						continue;
					}
					auto tile = map.getTile(x, y, z);
					for (auto &o : tile->ownedObjects)
					{
						if (o->getType() == TileObject::Type::Scenery)
						{
							auto mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();

							if (mp == thisPtr || !mp->isAlive())
							{
								continue;
							}
							if (mp->type->tile_type != SceneryTileType::TileType::CityWall)
							{
								continue;
							}
							supports.insert(mp);
						}
					}
				} // For every x and y
			}
			// Found two supports:
			if (supports.size() >= 2)
			{
				for (auto &mp : supports)
				{
					supportHardness = std::min(supportHardness, mp->supportHardness);
					mp->supportedParts.insert(currentPosition);
					supportedBy.emplace_back(mp->currentPosition);
				}
				return true;
			}
		}
		// Step 03.05: Check adjacents (for Tube)
		if (type->tile_type == SceneryTileType::TileType::PeopleTube)
		{
			std::set<sp<Scenery>> supports;
			int startX = pos.x - 1;
			int endX = pos.x + 1;
			int startY = pos.y - 1;
			int endY = pos.y + 1;
			for (int x = startX; x <= endX; x++)
			{
				for (int y = startY; y <= endY; y++)
				{
					if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y)
					{
						continue;
					}
					// Cannot support diagonally on xy
					if (x != pos.x && y != pos.y)
					{
						continue;
					}
					if (x == pos.x && y == pos.y)
					{
						continue;
					}
					// Cannot support if no connection
					if (!type->tube[vecToIntForward.at({x - pos.x, y - pos.y})])
					{
						continue;
					}
					auto tile = map.getTile(x, y, pos.z);
					for (auto &o : tile->ownedObjects)
					{
						if (o->getType() == TileObject::Type::Scenery)
						{
							auto mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();

							if (mp == thisPtr || !mp->isAlive())
							{
								continue;
							}
							// Must be a General
							if (mp->type->tile_type != SceneryTileType::TileType::General)
							{
								continue;
							}
							// Is supported!
							supportHardness = mp->supportHardness - 1;
							mp->supportedParts.insert(currentPosition);
							supportedBy.emplace_back(mp->currentPosition);
							return true;
						}
					}
				}
			}
		}
	}

	// At this point if we get support we will be supported by hard support, so it's hardness of 0
	supportHardness = 0;

	// Step 04: Shoot "support lines" and try to find something

	// Step 04.01: With roads and tubes we can actually shoot in any directions, not just on X or Y
	//			   and we can connect to any support.
	//			   We can also re-try support when our support is lost
	if (type->tile_type == SceneryTileType::TileType::Road ||
	    type->tile_type == SceneryTileType::TileType::PeopleTube)
	{
		std::list<std::pair<Vec2<int>, Vec2<bool>>> increments;
		std::list<std::pair<Vec2<int>, Vec2<bool>>> foundIncrements;

		for (int i = 0; i < 4; i++)
		{
			bool road = type->connection[i];
			bool tube = type->tube[i];
			if (!tube && !road)
			{
				continue;
			}
			increments.emplace_back(intToVec.at(i), Vec2<bool>{road, tube});
		}

		if (increments.size() > 1)
		{
			bool found;
			for (auto &increment : increments)
			{
				found = false;
				int x = pos.x + increment.first.x;
				int y = pos.y + increment.first.y;
				int z = pos.z;
				int lastTubeZInc = 0;
				bool clearLastTubeZInc = false;
				auto lastMp = thisPtr;
				do
				{
					if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y)
					{
						found = true;
						break;
					}
					sp<Scenery> mp = nullptr;
					auto tile = map.getTile(x, y, z);

					for (auto &o : tile->ownedObjects)
					{
						if (o->getType() == TileObject::Type::Scenery)
						{
							mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();
							// No good if destroyed or falling
							// No good if no good as both road and tube
							//  - no good as road if not road or no road connection
							//  - no good as tube if not tube or no tube connection/terminus
							//    (if tube connection is up/down we already checked it before
							//    arrival)
							if (mp->destroyed || mp->falling ||
							    ((!increment.second.x ||
							      (increment.second.x &&
							       !mp->type->connection[vecToIntBack.at(increment.first)])) &&
							     (!increment.second.y ||
							      (increment.second.y &&
							       (int)mp->currentPosition.z == (int)lastMp->currentPosition.z &&
							       !mp->type->tube[vecToIntBack.at(increment.first)] &&
							       mp->type->tile_type != SceneryTileType::TileType::General))))
							{
								mp = nullptr;
							}
						}
					}
					// Could not find map part of this type or it cannot provide support
					// We ignore those that have positive "ticksUntilFalling" as those can be saved
					if (!mp)
					{
						// If we're a road - try to go one down, look for a hill facing towards us
						if (increment.second.x)
						{
							int zDiff =
							    lastMp->type->hill[vecToIntForward.at(increment.first)] ? 1 : -1;
							if (z + zDiff < map.size.z)
							{
								tile = map.getTile(x, y, z + zDiff);
								for (auto &o : tile->ownedObjects)
								{
									if (o->getType() == TileObject::Type::Scenery)
									{
										mp = std::static_pointer_cast<TileObjectScenery>(o)
										         ->getOwner();
										if (mp->destroyed || mp->falling ||
										    mp->type->hill[vecToIntBack.at(increment.first)] !=
										        (zDiff == -1))
										{
											mp = nullptr;
										}
									}
								}
							}
							if (mp)
							{
								z += zDiff;
							}
							else
							{
								// fail for road, found no supporting hill
								break;
							}
						}
						else
						{
							// If we're a tube we can go up and down
							// Check for up
							bool success = false;
							for (int zInc = -1; zInc <= 1; zInc += 2)
							{
								if (zInc == -lastTubeZInc)
								{
									continue;
								}
								int forward = zInc == -1 ? 5 : 4;
								int backward = zInc == 1 ? 5 : 4;
								if (lastMp->type->tube[forward])
								{
									if (lastMp->currentPosition.z + zInc < map.size.z)
									{
										tile = map.getTile(lastMp->currentPosition.x,
										                   lastMp->currentPosition.y,
										                   lastMp->currentPosition.z + zInc);
										for (auto &o : tile->ownedObjects)
										{
											if (o->getType() == TileObject::Type::Scenery)
											{
												mp = std::static_pointer_cast<TileObjectScenery>(o)
												         ->getOwner();
												if (mp->destroyed || mp->falling ||
												    !mp->type->tube[backward])
												{
													mp = nullptr;
												}
											}
										}
										// Found a valid tube upwards, change z and continue
										if (mp)
										{
											x = lastMp->currentPosition.x;
											y = lastMp->currentPosition.y;
											z = lastMp->currentPosition.z + zInc;
											lastTubeZInc = zInc;
											clearLastTubeZInc = false;
											success = true;
											break;
										}
									}
								}
							}
							if (success)
							{
								continue;
							}
							// fail for tube, found nothing up or down
							break;
						}
					}
					lastMp = mp;
					// Found map part that won't collapse
					if (!mp->willCollapse())
					{
						// success
						supportHardness = std::min(supportHardness, mp->supportHardness);
						found = true;
						break;
					}
					else
					{
						// Can only link through unsupported roads or tubes
						if (mp->type->tile_type != SceneryTileType::TileType::Road &&
						    mp->type->tile_type != SceneryTileType::TileType::PeopleTube)
						{
							// failure, trying to link through unsupported non-road/tube
							break;
						}
						// Can only link through unsupported roads or tubes that
					}
					// continue
					x += increment.first.x;
					y += increment.first.y;
					if (clearLastTubeZInc)
					{
						clearLastTubeZInc = false;
						lastTubeZInc = 0;
					}
					if (lastTubeZInc != 0)
					{
						clearLastTubeZInc = true;
					}
				} while (true);
				if (found)
				{
					foundIncrements.emplace_back(increment);
				}
			}
			// If found at least two ways ways - cling to neighbours
			// (and mark them as having support so they can cling to us)
			if (foundIncrements.size() > 1)
			{
				for (auto &increment : foundIncrements)
				{
					int x = pos.x + increment.first.x;
					int y = pos.y + increment.first.y;
					int z = pos.z;
					int lastTubeZInc = 0;
					bool clearLastTubeZInc = false;
					auto lastMp = thisPtr;
					bool lastMPSupportedByFilled = false;
					do
					{
						if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y)
						{
							break;
						}
						sp<Scenery> mp = nullptr;
						auto tile = map.getTile(x, y, z);
						for (auto &o : tile->ownedObjects)
						{
							if (o->getType() == TileObject::Type::Scenery)
							{
								mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();
								// Still have to double-check this because maybe we went up/down
								// here
								// because it didn't fit
								if (mp->destroyed || mp->falling ||
								    ((!increment.second.x ||
								      (increment.second.x &&
								       !mp->type->connection[vecToIntBack.at(increment.first)])) &&
								     (!increment.second.y ||
								      (increment.second.y &&
								       (int)mp->currentPosition.z ==
								           (int)lastMp->currentPosition.z &&
								       !mp->type->tube[vecToIntBack.at(increment.first)] &&
								       mp->type->tile_type != SceneryTileType::TileType::General))))
								{
									mp = nullptr;
								}
							}
						}
						if (!mp)
						{
							if (increment.second.x)
							{
								z += lastMp->type->hill[vecToIntForward.at(increment.first)] ? 1
								                                                             : -1;
								tile = map.getTile(x, y, z);
								for (auto &o : tile->ownedObjects)
								{
									if (o->getType() == TileObject::Type::Scenery)
									{
										mp = std::static_pointer_cast<TileObjectScenery>(o)
										         ->getOwner();
										// Don't have to check this, we know it's ok
									}
								}
								if (!mp)
								{
									LogError("Map part disappeared? %d %d %d", x, y, z);
									return false;
								}
							}
							else
							{
								bool success = false;
								for (int zInc = -1; zInc <= 1; zInc += 2)
								{
									if (zInc == -lastTubeZInc)
									{
										continue;
									}
									int forward = zInc == -1 ? 5 : 4;
									int backward = zInc == 1 ? 5 : 4;
									if (lastMp->type->tube[forward])
									{
										if (lastMp->currentPosition.z + zInc < map.size.z)
										{
											tile = map.getTile(lastMp->currentPosition.x,
											                   lastMp->currentPosition.y,
											                   lastMp->currentPosition.z + zInc);
											for (auto &o : tile->ownedObjects)
											{
												if (o->getType() == TileObject::Type::Scenery)
												{
													mp =
													    std::static_pointer_cast<TileObjectScenery>(
													        o)
													        ->getOwner();
													if (mp->destroyed || mp->falling ||
													    !mp->type->tube[backward])
													{
														mp = nullptr;
													}
												}
											}
											// Found a valid tube upwards, change z and continue
											if (mp)
											{
												x = lastMp->currentPosition.x;
												y = lastMp->currentPosition.y;
												z = lastMp->currentPosition.z + zInc;
												success = true;
												lastTubeZInc = zInc;
												clearLastTubeZInc = false;
												break;
											}
										}
									}
								}
								if (success)
								{
									continue;
								}
								if (!mp)
								{
									LogError("Map part disappeared? %d %d %d", x, y, z);
									return false;
								}
							}
						}
						// We are supported by this part
						mp->supportedParts.insert(lastMp->currentPosition);
						// If lastMP's supportedBy was not full, we must make it also supported by
						// us
						if (!lastMPSupportedByFilled)
						{
							lastMp->supportedBy.emplace_back(mp->currentPosition);
						}
						// Found map part that won't collapse
						if (!mp->willCollapse())
						{
							// stop where we stopped originally
							break;
						}
						// If it's a midpart then it's also supported by us
						// Cancel midpart's collapse
						mp->cancelCollapse();
						// Last part supports this part
						lastMp->supportedParts.insert(mp->currentPosition);
						// If this part's support is not yet full then it's supported by us
						if (mp->supportedBy.size() < 2)
						{
							lastMPSupportedByFilled = false;
							mp->supportedBy.emplace_back(lastMp->currentPosition);
						}
						else
						{
							lastMPSupportedByFilled = true;
						}
						lastMp = mp;
						// continue
						x += increment.first.x;
						y += increment.first.y;
						if (clearLastTubeZInc)
						{
							clearLastTubeZInc = false;
							lastTubeZInc = 0;
						}
						if (lastTubeZInc != 0)
						{
							clearLastTubeZInc = true;
						}
					} while (true);
				} // for every connected way
				return true;
			} // if connected to more than one way
		}     // if more than 2 ways to connect to
	}         // If road or tube

	// Step 04.02: With generals, we can shoot in x or in y, but connect only to hard support
	//			   We also do not re-try support when lost
	if (type->tile_type == SceneryTileType::TileType::General)
	{
		std::list<std::list<Vec2<int>>> incrementsList;
		incrementsList.push_back({Vec2<int>{1, 0}, Vec2<int>{-1, 0}});
		incrementsList.push_back({Vec2<int>{0, 1}, Vec2<int>{0, -1}});

		for (auto &list : incrementsList)
		{
			bool found = false;
			for (auto &increment : list)
			{
				found = false;
				int x = pos.x + increment.x;
				int y = pos.y + increment.y;
				int z = pos.z;
				auto lastMp = thisPtr;
				do
				{
					if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y)
					{
						found = true;
						break;
					}
					sp<Scenery> mp = nullptr;
					auto tile = map.getTile(x, y, z);

					for (auto &o : tile->ownedObjects)
					{
						if (o->getType() == TileObject::Type::Scenery)
						{
							mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();
							// No good if destroyed or falling
							// No good if not general
							if (mp->destroyed || mp->falling ||
							    mp->type->tile_type != SceneryTileType::TileType::General)
							{
								mp = nullptr;
							}
						}
					}
					// We ignore those that have positive "ticksUntilFalling" as those can be saved
					if (!mp)
					{
						break;
					}
					lastMp = mp;
					// Found map part that won't collapse and is hard supported
					if (!mp->willCollapse() && mp->supportHardness > 0)
					{
						// success
						found = true;
						break;
					}
					// continue
					x += increment.x;
					y += increment.y;
				} while (true);
				if (!found)
				{
					break;
				}
			}
			if (!found)
			{
				continue;
			}
			// If found at least two ways ways - cling to neighbours
			// (and mark them as having support so they can cling to us)
			for (auto &increment : list)
			{
				int x = pos.x + increment.x;
				int y = pos.y + increment.y;
				int z = pos.z;
				auto lastMp = thisPtr;
				bool lastMPSupportedByFilled = false;
				do
				{
					if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y)
					{
						break;
					}
					sp<Scenery> mp = nullptr;
					auto tile = map.getTile(x, y, z);
					for (auto &o : tile->ownedObjects)
					{
						if (o->getType() == TileObject::Type::Scenery)
						{
							mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();
						}
					}
					if (!mp)
					{
						LogError("Map part disappeared? %d %d %d", x, y, z);
						return false;
					}
					mp->supportedParts.insert(lastMp->currentPosition);
					if (!lastMPSupportedByFilled)
					{
						lastMp->supportedBy.emplace_back(mp->currentPosition);
					}
					// Found map part that won't collapse
					if (!mp->willCollapse() && mp->supportHardness > 0)
					{
						// stop where we stopped originally
						break;
					}
					// If it's a midpart then it's also supported by us
					mp->cancelCollapse();
					lastMp->supportedParts.insert(mp->currentPosition);
					if (mp->supportedBy.size() < 2)
					{
						lastMPSupportedByFilled = false;
						mp->supportedBy.emplace_back(lastMp->currentPosition);
					}
					else
					{
						lastMPSupportedByFilled = true;
					}
					lastMp = mp;
					// continue
					x += increment.x;
					y += increment.y;
				} while (true);
			} // for every connected way
			return true;
		} // for every list of increments (vertical and horizontal)
	}     // If general

	// If we didn't succeed, don't clear supportedBy!
	// supportedBy = lastSupportedBy;

	return false;
}

void Scenery::ceaseBeingSupported()
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
					if (o->getType() == TileObject::Type::Scenery)
					{
						auto mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();
						if (mp->supportedParts.find(pos) != mp->supportedParts.end())
						{
							mp->supportedParts.erase(pos);
						}
					}
				}
			}
		}
	}
}

Vec3<int> Scenery::getTilePosition() const { return tileObject->getOwningTile()->position; }

const TileMap &Scenery::getMap() const { return tileObject->map; }

UString Scenery::getId() const { return type.id; }

int Scenery::getType() const { return (int)0; }

UString Scenery::getSupportString() const { return format("%d", supportHardness); }

void Scenery::setPosition(const Vec3<float> &pos)
{
	currentPosition = pos;
	if (!this->tileObject)
	{
		LogError("setPosition called on scenery with no tile object");
		return;
	}

	this->tileObject->setPosition(pos);
}

void Scenery::updateRelationWithAttacker(GameState &state, StateRef<Organisation> attackerOrg,
                                         bool killed)
{
	if (!attackerOrg)
	{
		return;
	}
	auto ourOrg = building->owner;

	// Killing scenery is 4x as influential
	float multiplier = killed ? 4.0f : 1.0f;
	// Lose 5 points
	ourOrg->adjustRelationTo(state, attackerOrg, -5.0f * multiplier);
	// Our allies lose 2.5 points, enemies gain 1 point
	for (auto &org : state.organisations)
	{
		if (org.first != attackerOrg.id && org.first != state.getCivilian().id)
		{
			if (org.second->isRelatedTo(ourOrg) == Organisation::Relation::Hostile)
			{
				org.second->adjustRelationTo(state, attackerOrg, 1.0f * multiplier);
			}
			else if (org.second->isRelatedTo(ourOrg) == Organisation::Relation::Allied)
			{
				org.second->adjustRelationTo(state, attackerOrg, -2.5f * multiplier);
			}
		}
	}
}

bool Scenery::handleCollision(GameState &state, Collision &c)
{
	StateRef<Organisation> attackerOrg;
	// Adjust relationships
	if (!type->commonProperty && building && c.projectile->firerVehicle)
	{
		attackerOrg = c.projectile->firerVehicle->owner;
		updateRelationWithAttacker(state, attackerOrg, false);
		bool intentional =
		    c.projectile->manualFire || (!c.projectile->firerVehicle->missions.empty() &&
		                                 c.projectile->firerVehicle->missions.front()->type ==
		                                     VehicleMission::MissionType::AttackBuilding);
		if (intentional || config().getBool("OpenApoc.NewFeature.ScrambleOnUnintentionalHit"))
		{
			building->underAttack(state, attackerOrg);
		}
	}

	return applyDamage(state, c.projectile->damage, attackerOrg);
}

bool Scenery::applyDamage(GameState &state, int power, StateRef<Organisation> attackerOrg)
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
	// Landing pads are invincible to direct damage
	if (type->isLandingPad)
	{
		return false;
	}

	int damage = randDamage050150(state.rng, power);

	if (type->tile_type == SceneryTileType::TileType::Road)
	{
		if (config().getBool("OpenApoc.Mod.InvulnerableRoads"))
		{
			return false;
		}
		if (config().getBool("OpenApoc.NewFeature.ArmoredRoads"))
		{
			damage -= ROAD_ARMOR;
		}
	}

	if (damage <= type->constitution)
	{
		return false;
	}

	updateRelationWithAttacker(state, attackerOrg, true);
	die(state, false);
	return false;
}

void Scenery::die(GameState &state, bool forced)
{
	if (falling)
	{
		// Spawn explosion doodad at us
		{
			auto doodad =
			    city->placeDoodad({&state, "DOODAD_3_EXPLOSION"}, tileObject->getCenter());
			fw().soundBackend->playSample(state.city_common_sample_list->sceneryExplosion,
			                              tileObject->getCenter());
		}
		// Spawn smoke and deal vehicle damage around us
		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				if (x == 0 && y == 0)
				{
					continue;
				}
				auto doodadPos = getPosition() + Vec3<float>{x, y, 0.0f};

				if (tileObject->map.tileIsValid(doodadPos))
				{
					auto doodadTile = tileObject->map.getTile(doodadPos);
					bool spawnBlocked = false;
					std::list<sp<Vehicle>> vehiclesToDamage;
					for (auto &obj : doodadTile->ownedObjects)
					{
						switch (obj->getType())
						{
							case TileObject::Type::Scenery:
							{
								auto sc =
								    std::static_pointer_cast<TileObjectScenery>(obj)->getOwner();
								if (sc->isAlive() &&
								    sc->type->walk_mode == SceneryTileType::WalkMode::None)
								{
									spawnBlocked = true;
								}
								break;
							}
							case TileObject::Type::Doodad:
							{
								auto doodad =
								    std::static_pointer_cast<TileObjectDoodad>(obj)->getOwner();
								if (doodad->type.id == "DOODAD_5_SMOKE_EXPLOSION")
								{
									spawnBlocked = true;
								}
								break;
							}
							case TileObject::Type::Vehicle:
							{
								auto v =
								    std::static_pointer_cast<TileObjectVehicle>(obj)->getVehicle();
								vehiclesToDamage.push_back(v);
								break;
							}
							default:
								// Other tiles don't get damaged or block smoke
								break;
						}
					}
					for (auto &v : vehiclesToDamage)
					{
						v->applyDamage(state,
						               (v->falling || v->crashed || v->sliding)
						                   ? FV_COLLISION_DAMAGE_LIMIT
						                   : type->strength,
						               0);
					}
					if (!spawnBlocked)
					{
						auto doodad =
						    city->placeDoodad({&state, "DOODAD_5_SMOKE_EXPLOSION"}, doodadPos);
					}
				}
			}
		}
		this->tileObject->removeFromMap();
		this->tileObject.reset();
		state.totalScore.cityDamage -= type->value;
		state.weekScore.cityDamage -= type->value;
		this->destroyed = true;
		return;
	}
	if (!forced && type->damagedTile)
	{
		if (type->tile_type == SceneryTileType::TileType::Road &&
		    type->damagedTile->tile_type != SceneryTileType::TileType::Road)
		{
			city->notifyRoadChange(initialPosition, false);
		}
		this->damaged = true;
		if (this->overlayDoodad)
		{
			this->overlayDoodad->remove(state);
			this->overlayDoodad = nullptr;
		}
		type = type->damagedTile;
		if (type->overlaySprite)
		{
			this->overlayDoodad =
			    mksp<Doodad>(this->getPosition(), type->imageOffset, false, 1, type->overlaySprite);
			city->map->addObjectToMap(this->overlayDoodad);
		}
		// Reapply tile params
		tileObject->setPosition(currentPosition);
	}
	else
	{
		// Don't destroy bottom tiles, else everything will leak out
		if (this->initialPosition.z <= 1)
		{
			this->damaged = true;
			// Reapply tile params
			tileObject->setPosition(currentPosition);
		}
		else
		{
			auto doodad =
			    city->placeDoodad({&state, "DOODAD_3_EXPLOSION"}, tileObject->getCenter());
			fw().soundBackend->playSample(state.city_common_sample_list->vehicleExplosion,
			                              tileObject->getCenter());

			this->destroyed = true;
		}
	}

	// Queue updates
	// state.current_battle->queueVisionRefresh(position);
	// state.current_battle->queuePathfindingRefresh(position);

	// Cease functioning
	ceaseBeingSupported();
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
		city->notifyRoadChange(initialPosition, false);
		if (this->overlayDoodad)
		{
			this->overlayDoodad->remove(state);
			this->overlayDoodad = nullptr;
		}
		this->tileObject->removeFromMap();
		this->tileObject.reset();
		if (building)
		{
			building->buildingPartChange(state, initialPosition, false);
		}
		state.totalScore.cityDamage -= type->value;
		state.weekScore.cityDamage -= type->value;
	}
}

void Scenery::collapse(GameState &state)
{
	// If it's already falling or destroyed or supported do nothing
	if (falling || !tileObject)
	{
		return;
	}
	// Level 0 can't collapse
	if (this->initialPosition.z <= 1)
	{
		this->damaged = true;
		LogWarning("Scenery at %s  type %s can't fall as below 2", currentPosition, type.id);
	}
	else
	{
		LogWarning("Scenery at %s type %s now falling", currentPosition, type.id);
		falling = true;
		// state.current_battle->queueVisionRefresh(position);
		// state.current_battle->queuePathfindingRefresh(position);
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
	if (building)
	{
		building->buildingPartChange(state, initialPosition, false);
	}
	city->notifyRoadChange(initialPosition, false);
}

void Scenery::update(GameState &state, unsigned int ticks)
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
}

void Scenery::updateFalling(GameState &state, unsigned int ticks)
{
	auto fallTicksRemaining = ticks;
	auto newPosition = currentPosition;
	while (fallTicksRemaining-- > 0)
	{
		fallingSpeed += FALLING_ACCELERATION_MAP_PART;
		newPosition -= Vec3<float>{0.0f, 0.0f, (fallingSpeed / TICK_SCALE)} / VELOCITY_SCALE_CITY;
	}

	if (newPosition.z < 0)
	{
		die(state);
		return;
	}

	// Collision happens when map part moves from this tile to the next
	// With this tile's Into and low Onto, as well as vehicles
	// With next tile's None and high Onto
	std::set<sp<Scenery>> killedScenery;
	if (floorf(newPosition.z) != floorf(currentPosition.z))
	{
		// Leaving tile: collide with everything left
		// NOTE: Vehicle::applyDamage() may destroy a vehicle, removing it from the map invalidating
		// the ownedObjects iterator, so keep a reference to all hit vehicles and process them
		// outside the ownedObjects iterator loop
		std::set<sp<Vehicle>> hitVehicles;
		for (auto &obj : tileObject->getOwningTile()->ownedObjects)
		{
			switch (obj->getType())
			{
				case TileObject::Type::Scenery:
				{
					auto mp = std::static_pointer_cast<TileObjectScenery>(obj)->getOwner();
					if (mp->tileObject && mp->isAlive())
					{
						if (mp->type->strength < type->mass)
						{
							killedScenery.insert(mp);
						}
						else
						{
							destroyed = true;
						}
					}
					break;
				}
				case TileObject::Type::Vehicle:
				{
					auto v = std::static_pointer_cast<TileObjectVehicle>(obj)->getVehicle();
					hitVehicles.insert(v);

					break;
				}
				default:
					// Ignore other object types?
					break;
			}
		}
		for (auto &v : hitVehicles)
		{
			v->applyDamage(state,
			               (v->falling || v->crashed || v->sliding) ? FV_COLLISION_DAMAGE_LIMIT
			                                                        : SC_COLLISION_DAMAGE,
			               0);
		}
		// New tile: If not destroyed yet collide with everything high enough
		if (!destroyed)
		{
			auto newTile = tileObject->map.getTile(newPosition);
			for (auto &obj : newTile->ownedObjects)
			{
				switch (obj->getType())
				{
					case TileObject::Type::Scenery:
					{
						auto mp = std::static_pointer_cast<TileObjectScenery>(obj)->getOwner();
						// Find if we collide into it
						if (mp->tileObject && mp->isAlive())
						{
							switch (mp->type->walk_mode)
							{
								case SceneryTileType::WalkMode::None:
									if (mp->type->height >= 12)
									{
										// Collide if high
										break;
									}
									// No collision now
									continue;
								case SceneryTileType::WalkMode::Onto:
								case SceneryTileType::WalkMode::Into:
									// No collision now
									continue;
							}
							if (mp->type->strength < type->mass)
							{
								killedScenery.insert(mp);
							}
							else
							{
								destroyed = true;
							}
						}
						break;
					}
					default:
						break;
				}
			}
		}
		for (auto &mp : killedScenery)
		{
			mp->collapse(state);
			mp->die(state);
		}

		// Spawn smoke, more intense if we land here
		{
			/*StateRef<DamageType> dtSmoke = { &state, "DAMAGETYPE_SMOKE" };
			auto hazard = state.current_battle->placeHazard(
			    state, owner, nullptr, dtSmoke, position, dtSmoke->hazardType->getLifetime(state),
			    2, destroyed ? 6 : 12);
			if (hazard)
			{
			    hazard->ticksUntilVisible = 0;
			}*/
		}
		// Cease to exist if destroyed
		if (destroyed)
		{
			die(state);
			return;
		}
	}

	setPosition(newPosition);
}

bool Scenery::canRepair() const
{
	// Don't fix it if it ain't broken

	if (this->isAlive() && !damaged)
		return false;
	// FIXME: Check how apoc repairs stuff, for now disallow repair

	return false;
}

void Scenery::repair(GameState &state)
{
	auto &map = *city->map;
	if (this->isAlive() && !damaged)
	{
		LogError("Trying to fix something that isn't broken");
	}
	damaged = false;
	falling = false;
	if (tileObject)
	{
		tileObject->removeFromMap();
	}
	tileObject = nullptr;
	if (overlayDoodad)
	{
		overlayDoodad->remove(state);
	}
	overlayDoodad = nullptr;
	type = city->initial_tiles[initialPosition];
	currentPosition = initialPosition;
	map.addObjectToMap(shared_from_this());
	if (type->overlaySprite)
	{
		overlayDoodad =
		    mksp<Doodad>(getPosition(), type->imageOffset, false, 1, type->overlaySprite);
		map.addObjectToMap(overlayDoodad);
	}
	if (building && !type->commonProperty)
	{
		building->buildingPartChange(state, initialPosition, true);
	}
	map.clearPathCaches();
}

bool Scenery::isAlive() const
{
	if (this->falling || this->destroyed || willCollapse())
		return false;
	return true;
}

} // namespace OpenApoc

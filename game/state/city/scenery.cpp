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

namespace OpenApoc
{
Scenery::Scenery() : damaged(false), falling(false), destroyed(false) {}

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

// FIXME:
// Implement linking using shooting lines for generals
// This will improve how complex buildings collapse
bool Scenery::findSupport()
{
	auto pos = tileObject->getOwningTile()->position;
	if (pos.z <= 1)
	{
		return true;
	}
	auto &map = tileObject->map;
	auto tileType = tileObject->getType();
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
	//   (in this case tube must have a downward connection)
	// - The exception is that tube can be supported by tube below if connected
	//
	// (following conditions provide "soft" support)
	//
	// - General/Wall/Junk can cling to one adjacent "hard" supported General/Wall/Junk
	// - General/Wall/Junk can cling to two adjacent "soft" supported General/Wall/Junk
	// - People tube can cling onto adjacent "hard" General, adhering to its direction
	//
	// Finally, can be supported if it has established support lines
	// on both sides that connect to an object providing support (any kind)
	//  - Object "shoots" a line in both directions and as long as there is an object on every tile
	//    the line continues, and if an object providing hard support is reached,
	//	  then "soft" support can be attained
	//

	// Step 01: Check for existing support conditions
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
	if (true)
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
				// Only support tube if has connection below
				if (type->tile_type == SceneryTileType::TileType::PeopleTube && !type->tube[5])
				{
					continue;
				}
				// Only tubes with connection can be supported by tubes
				if (mp->type->tile_type == SceneryTileType::TileType::PeopleTube)
				{
					// If ain't tube or no connection down from us or no connection up from below
					if (type->tile_type != SceneryTileType::TileType::PeopleTube ||
					    !mp->type->tube[4] || !type->tube[5])
					{
						continue;
					}
				}
				mp->supportedParts.insert(currentPosition);
				supportedBy.emplace_back(mp->currentPosition);
				return true;
			}
		}
	}

	// Step 03.01: Check adjacents (for General/Wall !INTO)
	if (type->tile_type == SceneryTileType::TileType::General ||
	    type->tile_type == SceneryTileType::TileType::CityWall ||
	    type->tile_type == SceneryTileType::TileType::PeopleTubeJunction)
	{
		std::set<sp<Scenery>> supports;
		int startX = pos.x - 1;
		int endX = pos.x + 1;
		int startY = pos.y - 1;
		int endY = pos.y + 1;
		int startZ = pos.z - 1;
		int endZ = pos.z + 1;
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
							// Must not be a tube, junk or road
							if (mp->type->tile_type == SceneryTileType::TileType::Road ||
							    mp->type->tile_type == SceneryTileType::TileType::PeopleTube ||
							    mp->type->tile_type ==
							        SceneryTileType::TileType::PeopleTubeJunction)
							{
								continue;
							}
							// Must be a matching(Wall for Wall, General for General/Junk)
							if (mp->type->tile_type == SceneryTileType::TileType::CityWall &&
							    type->tile_type != SceneryTileType::TileType::CityWall)
							{
								continue;
							}
							// Remember in case there's two of them soft supported
							supports.insert(mp);
							// Cannot get supported by single at other level
							if (z != pos.z)
							{
								continue;
							}
							// Cannot be soft-supported
							if (mp->supportedBy.size() > 1)
							{
								continue;
							}
							// Must be supported by below or by being the earth
							if (!mp->supportedBy.empty())
							{
								auto dir = mp->supportedBy.front() - tile->position;
								if (dir.x != 0 || dir.y != 0 || dir.z != -1)
								{
									continue;
								}
							}
							// Is supported!
							mp->supportedParts.insert(currentPosition);
							supportedBy.emplace_back(mp->currentPosition);
							return true;
						}
					}
				}
			}
		}
		// Found two or more soft supports
		if (supports.size() > 1)
		{
			for (auto &mp : supports)
			{
				mp->supportedParts.insert(currentPosition);
				supportedBy.emplace_back(mp->currentPosition);
			}
			return true;
		}
	}
	// Step 03.02: Check adjacents (for Tube)
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
						mp->supportedParts.insert(currentPosition);
						supportedBy.emplace_back(mp->currentPosition);
						return true;
					}
				}
			}
		}
	}

	// Step 04: Shoot "support lines" and try to find something
	// With roads and tubes we can actually shoot in any directions, not just on X or Y

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
							    (!increment.second.x ||
							     (increment.second.x &&
							      !mp->type->connection[vecToIntBack.at(increment.first)])) &&
							        (!increment.second.y ||
							         (increment.second.y &&
							          (int)mp->currentPosition.z ==
							              (int)lastMp->currentPosition.z &&
							          !mp->type->tube[vecToIntBack.at(increment.first)] &&
							          mp->type->tile_type != SceneryTileType::TileType::General)))
							{
								mp = nullptr;
							}
						}
					}
					// Could not find map part of this type or it cannot provide support
					// We ignore those that have positive "ticksUntilFalling" as those can be saved
					// yet
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
								    (!increment.second.x ||
								     (increment.second.x &&
								      !mp->type->connection[vecToIntBack.at(increment.first)])) &&
								        (!increment.second.y ||
								         (increment.second.y &&
								          (int)mp->currentPosition.z ==
								              (int)lastMp->currentPosition.z &&
								          !mp->type->tube[vecToIntBack.at(increment.first)] &&
								          mp->type->tile_type !=
								              SceneryTileType::TileType::General)))
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
						mp->supportedParts.insert(lastMp->currentPosition);
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
						mp->cancelCollapse();
						lastMp->supportedParts.insert(mp->currentPosition);
						if (mp->supportedBy.empty())
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

UString Scenery::getSupportString() const { return "NORMAL"; }

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

bool Scenery::handleCollision(GameState &state, Collision &c)
{
	// Adjust relationships
	if (!type->commonProperty && building && c.projectile->firerVehicle)
	{
		auto attackerOrg = c.projectile->firerVehicle->owner;
		auto ourOrg = building->owner;
		// Lose 5 points
		ourOrg->adjustRelationTo(state, attackerOrg, -5.0f);
		// If intentional lose additional 15 points
		bool intentional =
		    c.projectile->manualFire || (!c.projectile->firerVehicle->missions.empty() &&
		                                 c.projectile->firerVehicle->missions.front()->type ==
		                                     VehicleMission::MissionType::AttackBuilding);
		if (intentional)
		{
			ourOrg->adjustRelationTo(state, attackerOrg, -10.0f);
		}
		// Our allies lose 2.5 points, enemies gain 1 point
		for (auto &org : state.organisations)
		{
			if (org.first != attackerOrg.id && org.first != state.getCivilian().id)
			{
				if (org.second->isRelatedTo(ourOrg) == Organisation::Relation::Hostile)
				{
					org.second->adjustRelationTo(state, attackerOrg, 1.0f);
				}
				else if (org.second->isRelatedTo(ourOrg) == Organisation::Relation::Allied)
				{
					org.second->adjustRelationTo(state, attackerOrg, -2.5f);
				}
			}
		}
		if (intentional || config().getBool("OpenApoc.NewFeature.ScrambleOnUnintentionalHit"))
		{
			building->underAttack(state, attackerOrg);
		}
	}

	return applyDamage(state, c.projectile->damage);
}

bool Scenery::applyDamage(GameState &state, int power)
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

	die(state);
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
		LogWarning("Scenery at %s can't fall as below 2", currentPosition);
	}
	else
	{
		LogWarning("Scenery at %s now falling", currentPosition);
		falling = true;
		// state.current_battle->queueVisionRefresh(position);
		// state.current_battle->queuePathfindingRefresh(position);
		// Note: Pathfinding refresh relies on tile's battlescape parameters being updated
		// before it happens, so that battlescape parameters already account for the
		// now disfunctional map part. Pathfinding update will only happen
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
					v->applyDamage(state,
					               (v->falling || v->crashed || v->sliding)
					                   ? FV_COLLISION_DAMAGE_LIMIT
					                   : SC_COLLISION_DAMAGE,
					               0);
					break;
				}
				default:
					// Ignore other object types?
					break;
			}
		}
		// New tile: If not destroyed yet collid with everything high enough
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
	auto &map = *this->city->map;
	if (this->isAlive())
		LogError("Trying to fix something that isn't broken");
	this->damaged = false;
	this->falling = false;
	if (this->tileObject)
		this->tileObject->removeFromMap();
	this->tileObject = nullptr;

	if (this->overlayDoodad)
		this->overlayDoodad->remove(state);
	this->overlayDoodad = nullptr;
	map.addObjectToMap(shared_from_this());
	if (type->overlaySprite)
	{
		this->overlayDoodad =
		    mksp<Doodad>(this->getPosition(), type->imageOffset, false, 1, type->overlaySprite);
		map.addObjectToMap(this->overlayDoodad);
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

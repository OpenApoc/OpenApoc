#include "game/state/rules/supportedmappart.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/city/scenery.h"
#include "game/state/tilemap/tileobject_battlemappart.h"
#include "game/state/tilemap/tileobject_scenery.h"

// Uncomment to have verbose map part link up output
//#define MAP_PART_LINK_DEBUG_OUTPUT

namespace OpenApoc
{

void SupportedMapPart::attemptReLinkSupports(sp<std::set<SupportedMapPart *>> currentSet)
{
	if (currentSet->empty())
	{
		return;
	}

#ifdef MAP_PART_LINK_DEBUG_OUTPUT
	UString log = "ATTEMPTING RE-LINK OF SUPPORTS";
#endif

	// First mark all those in list as about to fall
	// (this prevents map parts to link to each other )
	// (they will have to find a new support outside of their now unsupported group)
	for (auto &mp : *currentSet)
	{
		mp->queueCollapse();
		mp->ceaseBeingSupported();
	}

	// Then try to re-establish support links
	// (in every iteration we add new unsupported items and remove those now supported)
	// (we do this as long as list keeps being changed)
	// (in the end we're left with map parts that are no longer supported and will fall)
	bool listChanged;
	do
	{
#ifdef MAP_PART_LINK_DEBUG_OUTPUT
		LogWarning("%s", log);
		log = "";
		log += format("\nIteration begins. List contains %d items:", (int)currentSet->size());
		for (auto &mp : *currentSet)
		{
			auto pos = mp->getTilePosition();
			log += format("\n %s at %d %d %d", mp->getId(), pos.x, pos.y, pos.z);
		}
		log += format("\n");
#endif
		// Attempt to re-link every entry in current set
		// Put those that were not re-linked into next set
		auto lastSet = currentSet;
		currentSet = mksp<std::set<SupportedMapPart *>>();
		listChanged = false;
		for (auto &curSetPart : *lastSet)
		{
			// Step 1: Maybe this part was rescued by other part linking through it in this
			// iteration
			//		   In this case, don't break it, otherwise we get into loop
			if (curSetPart->getTicksUntilCollapse() == 0)
			{
				listChanged = true;
				continue;
			}

			// Step 2: Temporary mark every map part supported by this part as collapsing
			auto supportedByThisMp = curSetPart->getSupportedParts();
			for (auto &supportedPart : *supportedByThisMp)
			{
				supportedPart->queueCollapse(curSetPart->getTicksUntilCollapse());
			}

			// Step 3: Try to find support without using map parts that were supported by this
			// (this prevents map parts supporting each other in a loop)
			// (it has to find another support that is not supported by it)
			if (curSetPart->findSupport())
			{
#ifdef MAP_PART_LINK_DEBUG_OUTPUT
				auto pos = curSetPart->getTilePosition();
				log += format("\n Processing %s at %d %d %d: OK %s", curSetPart->getId(), pos.x,
				              pos.y, pos.z, curSetPart->getSupportString());
				{
					auto &map = curSetPart->getMap();
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
								auto tile2 = map.getTile(x, y, z);
								for (auto &o2 : tile2->ownedObjects)
								{
									if (o2->getType() == TileObject::Type::Ground ||
									    o2->getType() == TileObject::Type::Feature ||
									    o2->getType() == TileObject::Type::LeftWall ||
									    o2->getType() == TileObject::Type::RightWall)
									{
										auto mp2 =
										    std::static_pointer_cast<TileObjectBattleMapPart>(o2)
										        ->getOwner();
										for (auto &p : mp2->supportedParts)
										{
											if (p.first == pos &&
											    p.second ==
											        (BattleMapPartType::Type)curSetPart->getType())
											{
												log += format("\n - Supported by %s at %d %d %d",
												              mp2->type.id, x - pos.x, y - pos.y,
												              z - pos.z);
											}
										}
									}
									if (o2->getType() == TileObject::Type::Scenery)
									{
										auto mp2 = std::static_pointer_cast<TileObjectScenery>(o2)
										               ->getOwner();
										for (auto &p : mp2->supportedParts)
										{
											if (p == pos)
											{
												log += format("\n - Supported by %s at %d %d %d",
												              mp2->type.id, x - pos.x, y - pos.y,
												              z - pos.z);
											}
										}
									}
								}
							}
						}
					}
				}
#endif
				// Step 4: Success [If support is found]
				// Resume support for every part supported by this
				for (auto &supportedPart : *supportedByThisMp)
				{
					supportedPart->cancelCollapse();
				}
				// This part is also unmarked as collapsing and not added to the next set,
				// effectively removing it from the list of map parts in need of re-support
				curSetPart->cancelCollapse();
				listChanged = true;
			}
			else
			{
#ifdef MAP_PART_LINK_DEBUG_OUTPUT
				auto pos = curSetPart->getTilePosition();
				log += format("\n Processing %s at %s: FAIL, remains in next iteration",
				              curSetPart->getId(), pos);
#endif
				// Step 4: Failure [If no support is found]
				// Finalise collapsed state of all parts supported by this
				// (was temporary since Step 1, now properly collapsing with all ties cut)
				for (auto &supportedPart : *supportedByThisMp)
				{
#ifdef MAP_PART_LINK_DEBUG_OUTPUT
					auto newpos = supportedPart->getTilePosition();
					log += format("\n - %s at %s added to next iteration", supportedPart->getId(),
					              newpos);
#endif
					supportedPart->ceaseBeingSupported();
					currentSet->insert(supportedPart);
					listChanged = true;
				}
				// Cut all ties for this part and add it to the next set,
				// meaning it will try to find support again in the next pass
				currentSet->insert(curSetPart);
				curSetPart->clearSupportedParts();
			}
		}
#ifdef MAP_PART_LINK_DEBUG_OUTPUT
		log += format("\n");
#endif
	} while (listChanged);

#ifdef MAP_PART_LINK_DEBUG_OUTPUT
	for (auto &sp : *currentSet)
	{
		log += format("\nAttempt over");
		log += format("\n%s at %s will fall", sp->getId(), sp->getTilePosition());
	}
	LogWarning("%s", log);
#endif
}

} // namespace OpenApoc

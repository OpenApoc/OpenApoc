#include "game/city/scenery.h"
#include "game/city/building.h"
#include "framework/logger.h"
#include "game/tileview/tileobject_scenery.h"
#include "game/rules/scenerytiledef.h"

namespace OpenApoc
{
Scenery::Scenery(SceneryTileDef &tileDef, Vec3<int> pos, sp<Building> bld)
    : tileDef(tileDef), pos(pos), building(bld), damaged(false)
{
}

void Scenery::handleCollision(Collision &c)
{
	// FIXME: Proper damage
	//
	// If this tile has a damaged tile, replace it with that. If it's already damaged, destroy as
	// normal
	if (!this->damaged && tileDef.getDamagedTile())
	{
		this->damaged = true;
		return;
	}
	this->tileObject->removeFromMap();
	this->tileObject.reset();
}

} // namespace OpenApoc

#include "game/city/scenery.h"
#include "game/city/building.h"
#include "framework/logger.h"
#include "game/tileview/tileobject_scenery.h"

namespace OpenApoc
{
Scenery::Scenery(SceneryTileDef &tileDef, Vec3<int> pos, sp<Building> bld)
    : tileDef(tileDef), pos(pos), building(bld)
{
}

void Scenery::handleCollision(Collision &c)
{
	// FIXME: Proper damage
	this->tileObject->removeFromMap();
	this->tileObject.reset();
}

} // namespace OpenApoc

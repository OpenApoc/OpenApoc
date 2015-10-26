#include "game/city/scenery.h"
#include "game/city/building.h"

namespace OpenApoc
{
Scenery::Scenery(SceneryTileDef &tileDef, Vec3<int> pos, sp<Building> bld)
    : tileDef(tileDef), pos(pos), building(bld)
{
}

} // namespace OpenApoc

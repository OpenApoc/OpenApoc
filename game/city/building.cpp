#include "game/city/building.h"
#include "game/organisation.h"
#include "framework/framework.h"
#include "framework/includes.h"

namespace OpenApoc
{

Building::Building(const BuildingDef &def, sp<Organisation> owner) : def(def), owner(owner) {}

}; // namespace OpenApoc

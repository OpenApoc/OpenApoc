#include "game/city/building.h"
#include "game/organisation.h"
#include "framework/framework.h"
#include "framework/includes.h"

namespace OpenApoc
{

Building::Building(BuildingDef &def, Organisation &owner) : def(def), owner(owner) {}

}; // namespace OpenApoc

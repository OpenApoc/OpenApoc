#include "game/city/vequipment.h"
#include "game/rules/vequipment.h"

namespace OpenApoc
{

VEquipment::VEquipment(const VEquipmentType &type) : type(type) {}

VEngine::VEngine(const VEngineType &type) : VEquipment(type) {}

} // namespace OpenApoc

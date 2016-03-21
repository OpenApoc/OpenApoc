#include "game/city/vequipment.h"
#include "game/rules/vequipment.h"

namespace OpenApoc
{

VEquipment::VEquipment(StateRef<VEquipmentType> type) : type(type) {}

VEngine::VEngine(StateRef<VEquipmentType> type) : VEquipment(type) {}

VGeneralEquipment::VGeneralEquipment(StateRef<VEquipmentType> type) : VEquipment(type) {}

} // namespace OpenApoc

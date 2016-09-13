#include "game/state/aequipment.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/agent.h"
#include "game/state/rules/aequipment_type.h"
#include "library/sp.h"

namespace OpenApoc
{

AEquipment::AEquipment() : equippedPosition(0, 0), ammo(0) {}

void AEquipment::update(int)
{
	// FIXME: Update equipment
}

} // namespace OpenApoc

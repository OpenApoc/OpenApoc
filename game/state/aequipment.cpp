#include "game/state/aequipment.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/agent.h"
#include "game/state/rules/aequipment_type.h"
#include "library/sp.h"

namespace OpenApoc
{

AEquipment::AEquipment() : equippedPosition(0, 0), ammo(0) {}

int AEquipment::getAccuracy(AgentType::BodyState bodyState, BattleUnit::FireAimingMode fireMode)
{
	if (!ownerAgent)
	{
		LogError("getAccuracy called on item not in agent's inventory!");
		return 0;
	}
	if (type->type != AEquipmentType::Type::Weapon)
	{
		LogError("getAccuracy called on non-weapon!");
		return 0;
	}
	// FIXME: Proper algorithm for calcualting weapon's accuracy
	StateRef<AEquipmentType> payload = getPayloadType();
	if (!payload)
	{
		payload = *type->ammo_types.begin();
	}
	return ownerAgent->modified_stats.accuracy * payload->accuracy * ( bodyState == AgentType::BodyState::Flying? 90 : (bodyState == AgentType::BodyState::Kneeling ? 110 : 100))
		/100 / 100 / ((fireMode == BattleUnit::FireAimingMode::Auto ? 4 : (fireMode == BattleUnit::FireAimingMode::Snap ? 2 : 1)));
}

void AEquipment::update(int)
{
	// FIXME: Update equipment
}

} // namespace OpenApoc

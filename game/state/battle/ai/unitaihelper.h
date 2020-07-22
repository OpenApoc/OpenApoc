#pragma once
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class GameState;
class BattleUnit;
class AIMovement;
class AEquipment;
enum class EquipmentSlotType;

class UnitAIHelper
{
  public:
	static sp<AIMovement> getFallbackMovement(GameState &state, BattleUnit &u, bool forced = false);

	static sp<AIMovement> getRetreatMovement(GameState &state, BattleUnit &u, bool forced = false);

	static sp<AIMovement> getTakeCoverMovement(GameState &state, BattleUnit &u,
	                                           bool forced = false);

	static sp<AIMovement> getKneelMovement(GameState &state, BattleUnit &u, bool forced = false);

	static sp<AIMovement> getPursueMovement(GameState &state, BattleUnit &u, Vec3<int> target,
	                                        bool forced = false);

	static sp<AIMovement> getTurnMovement(GameState &state, BattleUnit &u, Vec3<int> target);

	static void ensureItemInSlot(GameState &state, sp<AEquipment> item, EquipmentSlotType slot);
};
} // namespace OpenApoc
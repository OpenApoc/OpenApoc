#include "game/state/battle/battleexplosion.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/battle/battle.h"
#include "game/state/gamestate.h"
#include <cmath>

namespace OpenApoc
{
BattleExplosion::BattleExplosion(Vec3<int> position, StateRef<DamageType> damageType, int power, int depletionRate, StateRef<BattleUnit> ownerUnit) :
	position(position), ticksUntilExpansion(TICKS_MULTIPLIER), locationsToExpand({ position }), damageType(damageType), power(power), depletionRate(depletionRate), ownerUnit(ownerUnit) {}

void BattleExplosion::update(GameState &state, unsigned int ticks)
{


}

} // namespace OpenApoc

#include "game/state/battle/ai/unitaivanilla.h"
#include "game/state/aequipment.h"
#include "game/state/battle/ai/aidecision.h"
#include "game/state/battle/ai/unitaihelper.h"
#include "game/state/battle/battleunit.h"
#include "game/state/battle/battleunitanimationpack.h"
#include "game/state/gamestate.h"
#include "game/state/gametime.h"
#include "game/state/rules/damage.h"
#include "game/state/tileview/tileobject_battleunit.h"

namespace OpenApoc
{

namespace
{
static const std::set<TileObject::Type> mapPartSet = {
    TileObject::Type::Ground, TileObject::Type::LeftWall, TileObject::Type::RightWall,
    TileObject::Type::Feature};
static const std::set<TileObject::Type> unitSet = {TileObject::Type::Unit};
static const std::tuple<AIDecision, bool> NULLTUPLE2 = std::make_tuple(AIDecision(), false);
static const std::tuple<AIDecision, float, unsigned> NULLTUPLE3 =
    std::make_tuple(AIDecision(), -FLT_MAX, 0);
}

/* AI LOGIC: attack()

options (checked for every visible target):
- stand still and shoot at target with weapon
  (advance to target's location while shoooting at it)
- run forward to get in range for firing a weapon
- throw a grenade
- run forward to get in range for a greande throw
- use psi attack

- calculate priority for each action
- pick action with highest priority
- if "shoot at target" action wins then:
  - advance if CTH is low, stand still if high, with some added random variance

*/

// AI Logic for attacking:
//
// We go through every possibile tool:
// - first mindbender encountered if we have it
// - every weapon with loaded ammo
// - every grenade
//
// We then consider for every visible enemy:
//
// 1) Power, with resistance taken into consideration
// - For typical attacks, this will calculate avg damage done (using spread, armor, resist)
//   (Body will be used when body part hit is unknown)
// - This is then divided by unit's HP, resulting in percentage of HP taken by attack
// - For panic/mc, resultant value is hardcoded 0.5/1.0o
// - For psi stun, resultant value is just the avg stun damage done divided by HP
//
// 2) Collateral damage, for explosives we consider units in a radius of 4,
//   and simply subtract depletion rate multiplied by distane to them,
//   and repeat calculations above and add (hostile) or subtract (friendly)
//
// 3) Chance to hit, which will be determined in some simplified approximated way
// - For guided this will be 100%
// - For grenades this will be a hardcoded 80% since it's complicated to calculate all that
//   (considering a grenade can miss but still deal splash damage, and usually does so)
// - For psi this is the attack's success chance
//
// 4) Payload delivery delay, this is sum of:
// - getting in range at run speed delay
// - firing snap shot delay
// - possibly some fixed value for psionic attack
// - (RT only) projectile speed delay (or item speed, whichever)
//     Item has XY velocity of 2.0f, which is multiplied by 24, thus it's XY speed is 48
//     Projectile speed is already in tile scale, and has a multiplier of 8
//     Therefore, item can be considered to have a projectile speed of 48/8 = 6
//	   Then just divide distance by velocity (scaled) to get ticks
// - every second delay divides value gotten above by 2

UnitAIVanilla::UnitAIVanilla() { type = Type::Vanilla; }

// Priority is CTH * DAMAGE / TIME
// - Assume chance to miss is Dispersion * Distance / 10, CTH = 100 - chance to miss
// - Assume damage is 150% for armor penetration (for simplicity), hit to torso
// - Assume time to perform attack is fire delay for weapon at current attack mode
// - If out of range or cannot attack, priority is damage / 100 / distance to target
// Chance to advance is equal to chance to miss
std::tuple<AIDecision, float, unsigned>
UnitAIVanilla::getWeaponDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
                                 StateRef<BattleUnit> target)
{
	auto action = mksp<AIAction>();
	action->item = e;
	action->targetUnit = target;
	action->type = AIAction::Type::AttackWeaponUnit;

	auto movement = mksp<AIMovement>();
	movement->type = AIMovement::Type::Stop;

	// Just re-think what we're attacking every several seconds
	unsigned reThinkDelay = TICKS_PER_TURN;

	auto payload = e->getPayloadType();
	float distance = BattleUnitTileHelper::getDistanceStatic(u.position, target->position);

	float damage = (float)payload->damage;
	auto armor = target->agent->getArmor(BodyPart::Legs);
	int armorValue = 0;
	StateRef<DamageModifier> damageModifier;
	if (armor)
	{
		armorValue = armor->armor;
		damageModifier = armor->type->damage_modifier;
	}
	else
	{
		armorValue = target->agent->type->armor.at(BodyPart::Legs);
		damageModifier = target->agent->type->damage_modifier;
	}
	damage = payload->damage_type->dealDamage(damage * 1.5f, damageModifier);

	if (state, u.canAttackUnit(state, target, e) == WeaponStatus::NotFiring)
	{
		movement->type = AIMovement::Type::GetInRange;
		movement->targetLocation = target->position;
		movement->kneelingMode = KneelingMode::None;
		movement->movementMode = MovementMode::Running;

		return std::make_tuple(AIDecision(action, movement), damage / 100.0f / distance,
		                       reThinkDelay);
	}

	float time = (float)payload->fire_delay * (float)TICKS_MULTIPLIER / (float)u.fire_aiming_mode /
	             (float)TICKS_PER_SECOND;
	float cth =
	    std::max(1.0f, 100.f -
	                       (float)(100 -
	                               e->getAccuracy(u.target_body_state, u.current_movement_state,
	                                              u.fire_aiming_mode)) *
	                           distance / 40.0f);
	float priority = cth * damage / time;

	// Chance to advance is equal to chance to miss
	if (randBoundsExclusive(state.rng, 0, 100) >= cth)
	{
		movement->type = AIMovement::Type::Advance;
		movement->targetLocation = target->position;
		movement->kneelingMode = KneelingMode::None;
		movement->movementMode = MovementMode::Walking;
	}

	return std::make_tuple(AIDecision(action, movement), priority, reThinkDelay);
}

std::tuple<AIDecision, float, unsigned>
UnitAIVanilla::getPsiDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
                              StateRef<BattleUnit> target, PsiStatus status)
{
	std::ignore = state;

	int chance = u.getPsiChance(target, status, e->type);
	if (chance == 0)
	{
		return NULLTUPLE3;
	}
	unsigned reThinkDelay = TICKS_PER_SECOND / 2;
	float time = (float)reThinkDelay / (float)TICKS_PER_SECOND;
	float priority = (float)chance / time;

	auto action = mksp<AIAction>();
	action->item = e;
	action->targetUnit = target;
	action->preventOutOfTurnReThink = true;
	switch (status)
	{
		case PsiStatus::Control:
			action->type = AIAction::Type::AttackPsiMC;
			priority *= 64.0f;
			break;
		case PsiStatus::Panic:
			action->type = AIAction::Type::AttackPsiPanic;
			priority *= 8.0f;
			break;
		case PsiStatus::Stun:
			action->type = AIAction::Type::AttackPsiStun;
			priority *= 16.0f;
			break;
		default:
			LogError("Invalid psi attack state for getPsiDecision %d", (int)status);
			return NULLTUPLE3;
	}
	action->psiEnergySnapshot = u.agent->modified_stats.psi_energy;

	return std::make_tuple(AIDecision(action, nullptr), priority, reThinkDelay);
}

// Look at every unit in 4 tile AOE, approximate damage dealt to it, add up all damage to hostiles,
// subtracting damage to non-hostiles.
// Priority is CTH * DAMAGE / TIME
// - Assume CTH is 80%
// - Assume damage is 150% for armor penetration (for simplicity), hit to the legs
// - Assume time to perform attack is equal to the time to animate a throw, or 2x that if not in a
// correct posture (quick hack instead of figuring out how many frames it would take to actually do
// that)
// - If out of range, halve the priority
std::tuple<AIDecision, float, unsigned>
UnitAIVanilla::getGrenadeDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
                                  StateRef<BattleUnit> target)
{
	auto action = mksp<AIAction>();
	action->item = e;
	action->targetUnit = target;
	action->type = AIAction::Type::AttackGrenade;

	auto movement = mksp<AIMovement>();
	movement->type = AIMovement::Type::Stop;

	// In case something goes wrong with throw, rethink in a second (should be enough to change pose
	// and turn to throw
	unsigned reThinkDelay = TICKS_PER_SECOND;

	auto payload = e->getPayloadType();
	float cth = 0.8f;
	float time = (u.current_body_state == BodyState::Standing ? 1.0f : 2.0f) *
	             (float)(u.agent->getAnimationPack()->getFrameCountBody(
	                         e->type, BodyState::Standing, BodyState::Throwing, HandState::AtEase,
	                         MovementState::None, {1, 1}) +
	                     u.agent->getAnimationPack()->getFrameCountBody(
	                         e->type, BodyState::Throwing, BodyState::Standing, HandState::AtEase,
	                         MovementState::None, {1, 1})) *
	             (float)TICKS_PER_FRAME_UNIT / (float)TICKS_PER_SECOND;

	float damage = 0.0f;
	for (auto &t : state.current_battle->units)
	{
		if (t.second->isDead())
		{
			continue;
		}
		float distance = BattleUnitTileHelper::getDistanceStatic(t.second->position, u.position);
		if (distance > 16.0f)
		{
			continue;
		}
		float localDamage =
		    (float)payload->damage - (float)payload->explosion_depletion_rate * distance / 4.0f;
		if (localDamage < 0.0f)
		{
			continue;
		}
		auto armor = t.second->agent->getArmor(BodyPart::Legs);
		int armorValue = 0;
		StateRef<DamageModifier> damageModifier;
		if (armor)
		{
			armorValue = armor->armor;
			damageModifier = armor->type->damage_modifier;
		}
		else
		{
			armorValue = t.second->agent->type->armor.at(BodyPart::Legs);
			damageModifier = t.second->agent->type->damage_modifier;
		}
		localDamage = payload->damage_type->dealDamage(localDamage * 1.5f, damageModifier);
		damage +=
		    (u.owner->isRelatedTo(t.second->owner) == Organisation::Relation::Hostile ? 1.0f
		                                                                              : -1.0f) *
		    localDamage;
	}
	if (damage < 0.0f)
	{
		return NULLTUPLE3;
	}

	float priority = cth * damage / time;

	if (!e->getCanThrow(u, target->position))
	{
		movement->type = AIMovement::Type::GetInRange;
		movement->movementMode = MovementMode::Running;
		movement->targetLocation = target->position;
		// Properly get rethingdelay based on how far we must run to reach throwable point?
		reThinkDelay = TICKS_PER_TURN * 2;
		priority /= 2.0f;
	}

	return std::make_tuple(AIDecision(action, movement), priority, reThinkDelay);
}

std::tuple<AIDecision, float, unsigned> UnitAIVanilla::getBrainsuckerDecision(GameState &state,
                                                                              BattleUnit &u)
{
	auto action = mksp<AIAction>();
	action->type = AIAction::Type::AttackBrainsucker;

	unsigned reThinkDelay = TICKS_PER_SECOND;
	float distance = FLT_MAX;

	StateRef<DamageType> brainsucker = {&state, "DAMAGETYPE_BRAINSUCKER"};
	auto &visibleEnemies = state.current_battle->visibleEnemies[u.owner];
	for (auto &target : visibleEnemies)
	{
		if (brainsucker->dealDamage(100, target->agent->type->damage_modifier) == 0)
		{
			continue;
		}
		if (target->current_body_state == BodyState::Flying)
		{
			continue;
		}
		auto newDistance = BattleUnitTileHelper::getDistanceStatic(u.position, target->position);
		if (newDistance >= distance)
		{
			continue;
		}
		distance = newDistance;
		action->targetUnit = {&state, target->id};
	}

	if (!action->targetUnit)
	{
		return NULLTUPLE3;
	}

	auto movement = mksp<AIMovement>();
	movement->type = AIMovement::Type::GetInRange;
	movement->movementMode = MovementMode::Running;
	movement->targetLocation = action->targetUnit->position;
	movement->subordinate = true;

	Vec3<int> ourPos = u.position;
	Vec3<int> tarPos = action->targetUnit->position;
	if (std::abs(ourPos.x - tarPos.x) > 1 || std::abs(ourPos.y - tarPos.y) > 1 ||
	    std::abs(ourPos.z - tarPos.z) > 1)
	{
		action = nullptr;
	}

	return std::make_tuple(AIDecision(action, movement), 0.0f, reThinkDelay);
}

std::tuple<AIDecision, float, unsigned> UnitAIVanilla::getSuicideDecision(GameState &state,
                                                                          BattleUnit &u)
{
	std::tuple<AIDecision, float, unsigned> decision = NULLTUPLE3;

	auto action = mksp<AIAction>();
	action->type = AIAction::Type::AttackSuicide;

	unsigned reThinkDelay = TICKS_PER_SECOND;
	float distance = FLT_MAX;

	auto &visibleEnemies = state.current_battle->visibleEnemies[u.owner];
	for (auto &target : visibleEnemies)
	{
		if (target->current_body_state == BodyState::Flying)
		{
			continue;
		}
		auto newDistance = BattleUnitTileHelper::getDistanceStatic(u.position, target->position);
		if (newDistance >= distance)
		{
			continue;
		}
		distance = newDistance;
		action->targetUnit = {&state, target->id};
	}

	if (!action->targetUnit)
	{
		return decision;
	}

	auto movement = mksp<AIMovement>();
	movement->type = AIMovement::Type::GetInRange;
	movement->movementMode = MovementMode::Running;
	movement->targetLocation = action->targetUnit->position;
	movement->subordinate = true;

	if (BattleUnitTileHelper::getDistanceStatic(u.position, action->targetUnit->position) > 8.0f)
	{
		action = nullptr;
	}

	return std::make_tuple(AIDecision(action, movement), 0.0f, reThinkDelay);
}

std::tuple<AIDecision, float, unsigned> UnitAIVanilla::getAttackDecision(GameState &state,
                                                                         BattleUnit &u)
{
	std::tuple<AIDecision, float, unsigned> decision = NULLTUPLE3;

	auto &visibleEnemies = state.current_battle->visibleEnemies[u.owner];

	// Try each item for a weapon/grenade/psi attack
	// If out of range - advance
	// If cannot attack at all - take cover!

	// With mindbender just find first encountered mindbender
	sp<AEquipment> mindBender;

	// With grenades skip trying every single one and try only the best
	// Best being lightest with explosive damage type, and if weight is same, more powerful
	sp<AEquipment> grenade;

	// List of stuff to actually use
	std::list<sp<AEquipment>> items;

	bool brainsucker = false;
	bool suicider = false;

	for (auto &e : u.agent->equipment)
	{
		switch (e->type->type)
		{
			case AEquipmentType::Type::Weapon:
				if (!e->canFire())
				{
					break;
				}
				items.push_back(e);
				break;
			case AEquipmentType::Type::Grenade:
				// Accept if no other grenade yet sighted
				if (!grenade)
				{
					grenade = e;
					break;
				}
				// Only accept same or less weight
				if (e->getWeight() > grenade->getWeight())
				{
					break;
				}
				// Do not replace explosive one with gas one
				if (!e->type->damage_type->doesImpactDamage() &&
				    grenade->type->damage_type->doesImpactDamage())
				{
					break;
				}
				// Replace if:
				// - new one weights less
				// - new one is explsive and old one is not
				// - new one deals more damage
				if (e->getWeight() < grenade->getWeight() ||
				    (e->type->damage_type->doesImpactDamage() &&
				     !grenade->type->damage_type->doesImpactDamage()) ||
				    (e->type->damage > grenade->type->damage))
				{
					grenade = e;
					break;
				}
				break;
			case AEquipmentType::Type::MindBender:
				if (!mindBender && u.psiStatus == PsiStatus::NotEngaged)
				{
					mindBender = e;
					break;
				}
				break;
			case AEquipmentType::Type::Brainsucker:
				brainsucker = true;
				break;
			case AEquipmentType::Type::Popper:
				suicider = true;
				break;
			default:
				break;
		}
	}
	if (brainsucker)
	{
		auto newDecision = getBrainsuckerDecision(state, u);
		if (std::get<1>(newDecision) > std::get<1>(decision))
		{
			decision = newDecision;
		}
	}
	if (suicider)
	{
		auto newDecision = getSuicideDecision(state, u);
		if (std::get<1>(newDecision) > std::get<1>(decision))
		{
			decision = newDecision;
		}
	}
	if (grenade)
	{
		for (auto &target : u.visibleEnemies)
		{
			auto newDecision = getGrenadeDecision(state, u, grenade, target);
			if (std::get<1>(newDecision) > std::get<1>(decision))
			{
				decision = newDecision;
			}
		}
	}
	if (mindBender)
	{
		for (auto &target : visibleEnemies)
		{
			auto newDecision = getPsiDecision(state, u, mindBender, target, PsiStatus::Control);
			if (std::get<1>(newDecision) > std::get<1>(decision))
			{
				decision = newDecision;
			}
			newDecision = getPsiDecision(state, u, mindBender, target, PsiStatus::Panic);
			if (std::get<1>(newDecision) > std::get<1>(decision))
			{
				decision = newDecision;
			}
			newDecision = getPsiDecision(state, u, mindBender, target, PsiStatus::Stun);
			if (std::get<1>(newDecision) > std::get<1>(decision))
			{
				decision = newDecision;
			}
		}
	}
	for (auto &e : items)
	{
		for (auto &target : visibleEnemies)
		{
			auto newDecision = getWeaponDecision(state, u, e, target);
			if (std::get<1>(newDecision) > std::get<1>(decision))
			{
				decision = newDecision;
			}
		}
	}

	// If cannot attack - take cover!
	if (std::get<0>(decision).isEmpty())
	{
		decision = std::make_tuple(
		    AIDecision(nullptr, UnitAIHelper::getTakeCoverMovement(state, u, true)), 0.0f, 0);
	}

	return decision;
}

// Calculate AI's next action in case the unit is not attacking
std::tuple<AIDecision, float, unsigned> UnitAIVanilla::thinkGreen(GameState &state, BattleUnit &u)
{
	static const Vec3<int> NONE = {0, 0, 0};

	bool isMoving = lastDecision.movement && u.isMoving();
	bool isUnderAttack = attackerPosition != NONE;
	bool wasEnemyVisible = enemySpotted && lastSeenEnemyPosition != NONE;

	if (isMoving)
	{
		// If unit is moving into cover -> carry on
		if (lastDecision.movement->type == AIMovement::Type::TakeCover)
		{
			return NULLTUPLE3;
		}
	}

	if (isUnderAttack)
	{
		auto takeCover = UnitAIHelper::getTakeCoverMovement(state, u);
		if (takeCover)
		{
			return std::make_tuple(AIDecision(nullptr, takeCover), 0.0f, 0);
		}
	}

	if (wasEnemyVisible)
	{
		auto pursue = UnitAIHelper::getPursueMovement(state, u, (Vec3<int>)u.position +
		                                                            lastSeenEnemyPosition);
		if (pursue)
		{
			return std::make_tuple(AIDecision(nullptr, pursue), 0.0f, 0);
		}

		if (isMoving)
		{
			return NULLTUPLE3;
		}
	}

	if (isUnderAttack)
	{
		if (u.missions.empty() || u.missions.front()->type != BattleUnitMission::Type::Turn)
		{
			auto turn =
			    UnitAIHelper::getTurnMovement(state, u, (Vec3<int>)u.position + attackerPosition);
			return std::make_tuple(AIDecision(nullptr, turn), 0.0f, 0);
		}
	}

	auto fallback = UnitAIHelper::getFallbackMovement(state, u);
	if (fallback)
	{
		return std::make_tuple(AIDecision(nullptr, fallback), 0.0f, 0);
	}

	return NULLTUPLE3;
}

// Calculate AI's next action in case enemies are seen
std::tuple<AIDecision, float, unsigned> UnitAIVanilla::thinkRed(GameState &state, BattleUnit &u)
{
	static const Vec3<int> NONE = {0, 0, 0};

	bool isUnderAttack = attackerPosition != NONE;
	bool isInterrupted =
	    ticksUntilReThink > 0 && ticksLastThink + ticksUntilReThink > state.gameTime.getTicks();

	if (isUnderAttack)
	{
		auto fallback = UnitAIHelper::getFallbackMovement(state, u);
		if (fallback)
		{
			return std::make_tuple(AIDecision(nullptr, fallback), 0.0f, 0);
		}

		auto takeCover = UnitAIHelper::getTakeCoverMovement(state, u);
		if (takeCover)
		{
			return std::make_tuple(AIDecision(nullptr, takeCover), 0.0f, 0);
		}

		auto kneel = UnitAIHelper::getKneelMovement(state, u);
		if (kneel)
		{
			return std::make_tuple(AIDecision(nullptr, kneel), 0.0f, 0);
		}

		if (isInterrupted)
		{
			return NULLTUPLE3;
		}
	}

	return getAttackDecision(state, u);
}

AIDecision UnitAIVanilla::thinkInternal(GameState &state, BattleUnit &u)
{
	static const Vec3<int> NONE = {0, 0, 0};

	switch (u.getAIType())
	{
		case AIType::None:
		case AIType::Civilian:
			return {};
		case AIType::PanicFreeze:
		case AIType::PanicRun:
		case AIType::Berserk:
			LogError("Calling UnitAIVanilla on panic/berserk unit!?");
			return {};
		case AIType::Loner:
		case AIType::Group:
			// Go on below
			break;
	}

	// Clear actions that are done
	if (lastDecision.action && lastDecision.action->isFinished(u))
	{
		lastDecision.action = nullptr;
		// Clear subordinate movmenent
		if (lastDecision.movement && lastDecision.movement->subordinate)
		{
			lastDecision.movement = nullptr;
		}
	}
	// Clear movements that are done
	if (lastDecision.movement && lastDecision.movement->isFinished(u))
	{
		// Clear action if action is not in progress but subordinate movement finished
		if (lastDecision.movement->subordinate)
		{
			if (lastDecision.action && !lastDecision.action->inProgress(u))
			{
				lastDecision.action = nullptr;
			}
		}
		lastDecision.movement = nullptr;
	}
	// Clear actions that are outdated
	if (lastDecision.action && lastDecision.action->targetUnit &&
	    !lastDecision.action->targetUnit->isConscious())
	{
		lastDecision.action = nullptr;
	}

	//
	// See wether re-think is required
	//

	// Conditions that prevent re-thinking:

	// 1: Decision is never re-thought if currently throwing
	if (!u.missions.empty() && u.missions.front()->type == BattleUnitMission::Type::ThrowItem)
	{
		return {};
	}

	// 2: Anything else?
	//

	// Conditions that require re-thinking:

	bool reThink =
	    // Timer ran out
	    (ticksUntilReThink > 0 && ticksLastThink + ticksUntilReThink <= state.gameTime.getTicks())
	    // We just spotted a new enemy and we have no timer
	    || (!u.visibleEnemies.empty() && !enemySpottedPrevious && ticksUntilReThink == 0)
	    // We were attacked and we are not on a mission to get in range
	    || (attackerPosition != NONE &&
	        (!lastDecision.movement || lastDecision.movement->type != AIMovement::Type::GetInRange))
	    // We have enemies in sight, we are not attacking and we are not carrying out a decision
	    || (!u.visibleEnemies.empty() && !u.isAttacking() && lastDecision.isEmpty());

	// Note: if no enemies are in sight, and we're idle, we will do nothing.
	// This state is handled by tactical AI

	if (!reThink)
	{
		return lastDecision;
	}

	auto result = state.current_battle->visibleEnemies[u.owner].empty() ? thinkGreen(state, u)
	                                                                    : thinkRed(state, u);
	auto decision = std::get<0>(result);
	lastDecision = decision;

	if (decision.isEmpty())
	{
		return {};
	}

	ticksLastThink = state.gameTime.getTicks();
	ticksUntilReThink = std::get<2>(result);

	return decision;
}

std::tuple<AIDecision, bool> UnitAIVanilla::think(GameState &state, BattleUnit &u, bool interrupt)
{
	active = u.isAIControlled(state) && !interrupt;

	if (!active)
	{
		return NULLTUPLE2;
	}

	auto decision = thinkInternal(state, u);
	routine(state, u);

	if (!decision.isEmpty())
	{
		lastDecision = decision;
		if (decision.action && decision.action->preventOutOfTurnReThink)
		{
			u.aiList.ticksLastOutOfOrderThink = state.gameTime.getTicks();
		}
	}

	return std::make_tuple(decision, false);
}

void UnitAIVanilla::routine(GameState &state, BattleUnit &u)
{
	static const Vec3<int> NONE = {0, 0, 0};

	// Reload all guns
	for (auto &e : u.agent->equipment)
	{
		if (e->needsReload())
		{
			e->loadAmmo(state);
		}
	}

	// Equip a cloaking field if we don't already have one in left hand
	auto cloaking = u.agent->getFirstItemByType(AEquipmentType::Type::CloakingField);
	if (cloaking)
	{
		UnitAIHelper::ensureItemInSlot(state, cloaking, AEquipmentSlotType::LeftHand);
	}

	// Equip best weapon in right hand if we don't have something we otherwise need in that slot
	auto rhItem = u.agent->getFirstItemInSlot(AEquipmentSlotType::RightHand);
	if (!lastDecision.action || rhItem != lastDecision.action->item)
	{
		// Right now just equip most long ranged weapon, if tied most damaging
		auto newItem = rhItem;
		int maxRange = 0;
		int maxDamage = 0;
		for (auto &e : u.agent->equipment)
		{
			if (!e->canFire())
			{
				continue;
			}
			if ((e->getPayloadType()->range > maxRange) ||
			    (e->getPayloadType()->range == maxRange && e->getPayloadType()->damage > maxDamage))
			{
				maxRange = e->getPayloadType()->range;
				maxDamage = e->getPayloadType()->damage;
			}
		}
		// Equip new item into slot
		if (newItem != rhItem)
		{
			UnitAIHelper::ensureItemInSlot(state, newItem, AEquipmentSlotType::RightHand);
		}
	}

	// AI preferences
	u.fire_aiming_mode = WeaponAimingMode::Snap;
	u.fire_permission_mode = BattleUnit::FirePermissionMode::AtWill;
	if (state.current_battle->mode == Battle::Mode::TurnBased)
	{
		// Ensure at least half of move is available for moving
		u.setReserveKneelMode(KneelingMode::Kneeling);
		u.setReserveShotMode(ReserveShotMode::Aimed);
		int kneelCost = u.reserve_kneel_mode == KneelingMode::None
		                    ? 0
		                    : BattleUnitMission::getBodyStateChangeCost(u, BodyState::Standing,
		                                                                BodyState::Kneeling);
		if (u.reserveShotCost + kneelCost > u.initialTU / 2)
		{
			u.setReserveShotMode(ReserveShotMode::Snap);
		}
		if (u.reserveShotCost + kneelCost > u.initialTU / 2)
		{
			u.setReserveKneelMode(KneelingMode::None);
		}
	}

	// Update aiState
	{
		attackerPosition = NONE;
		enemySpottedPrevious = enemySpotted;
		enemySpotted = false;
		lastSeenEnemyPosition = NONE;
	}
}

void UnitAIVanilla::reset(GameState &state, BattleUnit &)
{
	lastDecision = {};
	ticksLastThink = state.gameTime.getTicks();
	ticksUntilReThink = 0;
}

void UnitAIVanilla::notifyUnderFire(Vec3<int> position) { attackerPosition = position; }

void UnitAIVanilla::notifyHit(Vec3<int> position) { attackerPosition = position; }

void UnitAIVanilla::notifyEnemySpotted(Vec3<int> position)
{
	enemySpotted = true;
	lastSeenEnemyPosition = position;
}
}
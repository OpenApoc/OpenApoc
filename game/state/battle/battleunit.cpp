#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/state/battle/battleunit.h"
#include "framework/framework.h"
#include "framework/sound.h"
#include "game/state/aequipment.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunitanimationpack.h"
#include "game/state/city/projectile.h"
#include "game/state/gamestate.h"
#include "game/state/rules/damage.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/state/tileview/tileobject_shadow.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace OpenApoc
{

template <> sp<BattleUnit> StateObject<BattleUnit>::get(const GameState &state, const UString &id)
{
	auto it = state.current_battle->units.find(id);
	if (it == state.current_battle->units.end())
	{
		LogError("No agent_type matching ID \"%s\"", id.cStr());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<BattleUnit>::getPrefix()
{
	static UString prefix = "BATTLEUNIT_";
	return prefix;
}
template <> const UString &StateObject<BattleUnit>::getTypeName()
{
	static UString name = "BattleUnit";
	return name;
}
template <>
const UString &StateObject<BattleUnit>::getId(const GameState &state, const sp<BattleUnit> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.current_battle->units)
	{
		if (a.second == ptr)
			return a.first;
	}
	LogError("No battleUnit matching pointer %p", ptr.get());
	return emptyString;
}

void BattleUnit::removeFromSquad(Battle &battle)
{
	battle.forces[owner].removeAt(squadNumber, squadPosition);
}

bool BattleUnit::assignToSquad(Battle &battle, int squad)
{
	return battle.forces[owner].insert(squad, shared_from_this());
}

void BattleUnit::moveToSquadPosition(Battle &battle, int position)
{
	battle.forces[owner].insertAt(squadNumber, position, shared_from_this());
}

bool BattleUnit::isFatallyWounded()
{
	for (auto e : fatalWounds)
	{
		if (e.second > 0)
		{
			return true;
		}
	}
	return false;
}

void BattleUnit::setPosition(const Vec3<float> &pos)
{
	this->position = pos;
	if (!this->tileObject)
	{
		LogError("setPosition called on unit with no tile object");
		return;
	}
	else
	{
		this->tileObject->setPosition(pos);
	}

	if (this->shadowObject)
	{
		this->shadowObject->setPosition(this->tileObject->getCenter());
	}
}

void BattleUnit::resetGoal()
{
	goalPosition = position;
	goalFacing = facing;
	atGoal = true;
}

void BattleUnit::setFocus(GameState &state, StateRef<BattleUnit> unit)
{
	StateRef<BattleUnit> sru = {&state, id};
	if (focusUnit)
	{
		auto it =
		    std::find(focusUnit->focusedByUnits.begin(), focusUnit->focusedByUnits.end(), sru);
		if (it != focusUnit->focusedByUnits.end())
		{
			focusUnit->focusedByUnits.erase(it);
		}
		else
		{
			LogError("Inconsistent focusUnit/focusBy!");
		}
	}
	focusUnit = unit;
	focusUnit->focusedByUnits.push_back(sru);
}

void BattleUnit::startAttacking(WeaponStatus status)
{
	switch (battleMode)
	{
		case Battle::Mode::TurnBased:
			// In Turn based we cannot override firing
			if (isAttacking())
			{
				return;
			}
			// In Turn based we cannot fire both hands
			if (status == WeaponStatus::FiringBothHands)
			{
				// Right hand has priority
				auto rhItem = agent->getFirstItemInSlot(AEquipmentSlotType::RightHand);
				if (rhItem && rhItem->canFire())
				{
					status = WeaponStatus::FiringRightHand;
				}
				else
				{
					// We don't care what's in the left hand,
					// we will just cancel firing in update() if there's nothing to fire
					status = WeaponStatus::FiringLeftHand;
				}
			}
			break;
		case Battle::Mode::RealTime:
			// Start firing both hands if added one hand to another
			if ((weaponStatus == WeaponStatus::FiringLeftHand &&
			     status == WeaponStatus::FiringRightHand) ||
			    (weaponStatus == WeaponStatus::FiringRightHand &&
			     status == WeaponStatus::FiringLeftHand))
			{
				status = WeaponStatus::FiringBothHands;
			}
			break;
	}

	weaponStatus = status;
	ticksTillNextTargetCheck = 0;
}

void BattleUnit::startAttacking(StateRef<BattleUnit> unit, WeaponStatus status)
{
	startAttacking(status);
	targetUnit = unit;
	targetingMode = TargetingMode::Unit;
}

void BattleUnit::startAttacking(Vec3<int> tile, WeaponStatus status, bool atGround)
{
	startAttacking(status);
	targetTile = tile;
	targetTile = tile;
	targetingMode = atGround ? TargetingMode::TileGround : TargetingMode::TileCenter;
}

void BattleUnit::stopAttacking()
{
	weaponStatus = WeaponStatus::NotFiring;
	targetingMode = TargetingMode::NoTarget;
	targetUnit.clear();
	ticksTillNextTargetCheck = 0;
}
bool BattleUnit::canAfford(int cost) const
{
	if (battleMode == Battle::Mode::RealTime)
	{
		return true;
	}
	return agent->modified_stats.time_units >= cost;
}

bool BattleUnit::spendTU(int cost)
{
	if (battleMode == Battle::Mode::RealTime)
	{
		return true;
	}
	if (cost > agent->modified_stats.time_units)
	{
		return false;
	}
	agent->modified_stats.time_units -= cost;
	return true;
}

int BattleUnit::getMaxHealth() const { return this->agent->current_stats.health; }

int BattleUnit::getHealth() const { return this->agent->modified_stats.health; }

int BattleUnit::getMaxShield() const
{
	int maxShield = 0;

	for (auto &e : this->agent->equipment)
	{
		if (e->type->type != AEquipmentType::Type::DisruptorShield)
			continue;
		maxShield += e->type->max_ammo;
	}

	return maxShield;
}

int BattleUnit::getShield() const
{
	int curShield = 0;

	for (auto &e : this->agent->equipment)
	{
		if (e->type->type != AEquipmentType::Type::DisruptorShield)
			continue;
		curShield += e->ammo;
	}

	return curShield;
}

int BattleUnit::getStunDamage() const
{
	// FIXME: Figure out stun damage scale
	int SCALE = TICKS_PER_SECOND;
	return stunDamageInTicks / SCALE;
}

bool BattleUnit::isDead() const { return getHealth() <= 0 || destroyed; }

bool BattleUnit::isUnconscious() const { return !isDead() && getStunDamage() >= getHealth(); }

bool BattleUnit::isConscious() const
{
	return !isDead() && getStunDamage() < getHealth() &&
	       (current_body_state != BodyState::Downed || target_body_state != BodyState::Downed);
}

bool BattleUnit::isStatic() const
{
	return current_movement_state == MovementState::None && !falling &&
	       current_body_state == target_body_state;
}

bool BattleUnit::isBusy() const { return !isStatic() || isAttacking(); }

bool BattleUnit::isAttacking() const { return weaponStatus != WeaponStatus::NotFiring; }
bool BattleUnit::isThrowing() const
{
	bool throwing = false;
	for (auto &m : missions)
	{
		if (m->type == BattleUnitMission::Type::ThrowItem)
		{
			throwing = true;
			break;
		}
	}
	return throwing;
}

bool BattleUnit::canFly() const
{
	return isConscious() && agent->isBodyStateAllowed(BodyState::Flying);
}

bool BattleUnit::canMove() const
{
	if (!isConscious())
	{
		return false;
	}
	if (agent->isMovementStateAllowed(MovementState::Normal) ||
	    agent->isMovementStateAllowed(MovementState::Running))
	{
		return true;
	}
	return false;
}

bool BattleUnit::canProne(Vec3<int> pos, Vec2<int> fac) const
{
	if (isLarge())
	{
		LogError("Large unit attempting to go prone? WTF? Should large units ever acces this?");
		return false;
	}
	// Check if agent can go prone and stand in its current tile
	if (!agent->isBodyStateAllowed(BodyState::Prone) || !tileObject->getOwningTile()->getCanStand())
		return false;
	// Check if agent can put legs in the tile behind. Conditions
	// 1) Target tile provides standing ability
	// 2) Target tile height is not too big compared to current tile
	// 3) Target tile is passable
	// 4) Target tile has no unit occupying it (other than us)
	Vec3<int> legsPos = pos - Vec3<int>{fac.x, fac.y, 0};
	if ((legsPos.x >= 0) && (legsPos.x < tileObject->map.size.x) && (legsPos.y >= 0) &&
	    (legsPos.y < tileObject->map.size.y) && (legsPos.z >= 0) &&
	    (legsPos.z < tileObject->map.size.z))
	{
		auto bodyTile = tileObject->map.getTile(pos);
		auto legsTile = tileObject->map.getTile(legsPos);
		if (legsTile->canStand && bodyTile->canStand &&
		    std::abs(legsTile->height - bodyTile->height) <= 0.25f &&
		    legsTile->getPassable(false, agent->type->bodyType->height.at(BodyState::Prone)) &&
		    (legsPos == (Vec3<int>)position || !legsTile->getUnitIfPresent(true, true)))
		{
			return true;
		}
	}
	return false;
}

bool BattleUnit::canKneel() const
{
	if (!agent->isBodyStateAllowed(BodyState::Kneeling) ||
	    !tileObject->getOwningTile()->getCanStand(isLarge()))
		return false;
	return true;
}

void BattleUnit::addFatalWound(GameState &state)
{
	switch (randBoundsInclusive(state.rng, 0, 4))
	{
		case 0:
			fatalWounds[BodyPart::Body]++;
			break;
		case 1:
			fatalWounds[BodyPart::Helmet]++;
			break;
		case 2:
			fatalWounds[BodyPart::LeftArm]++;
			break;
		case 3:
			fatalWounds[BodyPart::RightArm]++;
			break;
		case 4:
			fatalWounds[BodyPart::Helmet]++;
			break;
	}
}

void BattleUnit::dealDamage(GameState &state, int damage, bool generateFatalWounds, int stunPower)
{
	bool wasConscious = isConscious();
	bool fatal = false;

	// Deal stun damage
	if (stunPower > 0)
	{
		// FIXME: Figure out stun damage scale
		int SCALE = TICKS_PER_SECOND;

		stunDamageInTicks +=
		    clamp(damage * SCALE, 0, std::max(0, stunPower * SCALE - stunDamageInTicks));
	}
	// Deal health damage
	else
	{
		agent->modified_stats.health -= damage;
	}

	// Generate fatal wounds
	if (generateFatalWounds)
	{
		int woundDamageRemaining = damage;
		while (woundDamageRemaining > 10)
		{
			woundDamageRemaining -= 10;
			addFatalWound(state);
			fatal = true;
		}
		if (randBoundsExclusive(state.rng, 0, 10) < woundDamageRemaining)
		{
			addFatalWound(state);
			fatal = true;
		}
	}

	// Die or go unconscious
	if (isDead())
	{
		LogWarning("Handle violent deaths");
		die(state, true);
		return;
	}
	else if (!isConscious() && wasConscious)
	{
		fallUnconscious(state);
	}

	// Emit sound fatal wound
	if (fatal)
	{
		if (agent->type->fatalWoundSfx.find(agent->gender) != agent->type->fatalWoundSfx.end() &&
		    !agent->type->fatalWoundSfx.at(agent->gender).empty())
		{
			fw().soundBackend->playSample(
			    listRandomiser(state.rng, agent->type->fatalWoundSfx.at(agent->gender)), position);
		}
	}
	// Emit sound wound
	else if (stunPower == 0)
	{
		if (agent->type->damageSfx.find(agent->gender) != agent->type->damageSfx.end() &&
		    !agent->type->damageSfx.at(agent->gender).empty())
		{
			fw().soundBackend->playSample(
			    listRandomiser(state.rng, agent->type->damageSfx.at(agent->gender)), position);
		}
	}

	return;
}

bool BattleUnit::applyDamage(GameState &state, int power, StateRef<DamageType> damageType,
                             BodyPart bodyPart)
{
	if (damageType->doesImpactDamage())
	{
		fw().soundBackend->playSample(listRandomiser(state.rng, *genericHitSounds), position);
	}

	// Calculate damage
	int damage;
	bool USER_OPTION_UFO_DAMAGE_MODEL = false;
	if (damageType->effectType == DamageType::EffectType::Smoke) // smoke deals 1-3 stun damage
	{
		power = 2;
		damage = randDamage050150(state.rng, power);
	}
	else if (damageType->explosive) // explosive deals 50-150% damage
	{
		damage = randDamage050150(state.rng, power);
	}
	else if (USER_OPTION_UFO_DAMAGE_MODEL)
	{
		damage = randDamage000200(state.rng, power);
	}
	else
	{
		damage = randDamage050150(state.rng, power);
	}

	// Hit shield if present
	if (!damageType->ignore_shield)
	{
		auto shield = agent->getFirstShield();
		if (shield)
		{
			damage = damageType->dealDamage(damage, shield->type->damage_modifier);
			shield->ammo -= damage;
			// Shield destroyed
			if (shield->ammo <= 0)
			{
				agent->removeEquipment(shield);
			}
			state.current_battle->placeDoodad({&state, "DOODAD_27_SHIELD"},
			                                  tileObject->getCenter());
			return true;
		}
	}

	// Calculate damage to armor type
	auto armor = agent->getArmor(bodyPart);
	int armorValue = 0;
	StateRef<DamageModifier> damageModifier;
	if (armor)
	{
		armorValue = armor->ammo;
		damageModifier = armor->type->damage_modifier;
	}
	else
	{
		armorValue = agent->type->armor.at(bodyPart);
		damageModifier = agent->type->damage_modifier;
	}
	// Smoke ignores armor value but does not ignore damage modifier
	damage = damageType->dealDamage(damage, damageModifier) -
	         (damageType->ignoresArmorValue() ? 0 : armorValue);

	// No daamge
	if (damage <= 0)
	{
		return false;
	}

	// Smoke, fire and stun damage does not damage armor
	if (damageType->dealsArmorDamage() && armor)
	{
		// Armor damage
		int armorDamage = damage / 10 + 1;
		armor->ammo -= armorDamage;
		// Armor destroyed
		if (armor->ammo <= 0)
		{
			agent->removeEquipment(armor);
		}
	}

	// Apply damage according to type
	dealDamage(state, damage, damageType->dealsFatalWounds(),
	           damageType->dealsStunDamage() ? power : 0);

	return false;
}

BodyPart BattleUnit::determineBodyPartHit(StateRef<DamageType> damageType, Vec3<float> cposition,
                                          Vec3<float> direction)
{
	BodyPart bodyPartHit = BodyPart::Body;

	// FIXME: Ensure body part determination is correct
	// Assume top 25% is head, lower 25% is legs, and middle 50% is body/left/right
	float altitude = (cposition.z - position.z) * 40.0f / (float)getCurrentHeight();
	if (damageType->alwaysImpactsHead()) // gas deals damage to the head
	{
		bodyPartHit = BodyPart::Helmet;
	}
	else if (altitude > 0.75f)
	{
		bodyPartHit = BodyPart::Helmet;
	}
	else if (altitude < 0.25f)
	{
		bodyPartHit = BodyPart::Legs;
	}
	else
	{
		auto unitDir = glm::normalize(Vec3<float>{facing.x, facing.y, 0.0f});
		auto projectileDir = glm::normalize(Vec3<float>{direction.x, direction.y, 0.0f});
		auto cross = glm::cross(unitDir, projectileDir);
		int angle =
		    (int)((cross.z >= 0 ? -1 : 1) * glm::angle(unitDir, -projectileDir) / M_PI * 180.0f);
		if (angle > 45 && angle < 135)
		{
			bodyPartHit = BodyPart::RightArm;
		}
		else if (angle < -45 && angle > -135)
		{
			bodyPartHit = BodyPart::LeftArm;
		}
	}
	return bodyPartHit;
}

bool BattleUnit::handleCollision(GameState &state, Collision &c)
{
	std::ignore = state;

	// Corpses do not handle collision
	if (isDead())
		return false;

	if (!this->tileObject)
	{
		LogError("It's possible multiple projectiles hit the same tile in the same tick (?)");
		return false;
	}

	auto projectile = c.projectile.get();
	if (projectile)
	{
		return applyDamage(
		    state, projectile->damage, projectile->damageType,
		    determineBodyPartHit(projectile->damageType, c.position, projectile->getVelocity()));
	}
	return false;
}

void BattleUnit::update(GameState &state, unsigned int ticks)
{
	// Destroyed or retreated units do not exist in the battlescape
	if (destroyed || retreated)
	{
		return;
	}

	// Init
	//
	auto &map = tileObject->map;

	// Update other classes
	//
	for (auto item : agent->equipment)
		item->update(state, ticks);

	if (!this->missions.empty())
		this->missions.front()->update(state, *this, ticks);

	// Update our stats and state
	//

	// FIXME: Regenerate stamina

	// Stun removal
	if (stunDamageInTicks > 0)
	{
		stunDamageInTicks = std::max(0, stunDamageInTicks - (int)ticks);
	}

	// Fatal wounds / healing
	if (isFatallyWounded() && !isDead())
	{
		bool unconscious = isUnconscious();
		woundTicksAccumulated += ticks;
		while (woundTicksAccumulated > TICKS_PER_UNIT_EFFECT)
		{
			woundTicksAccumulated -= TICKS_PER_UNIT_EFFECT;
			for (auto &w : fatalWounds)
			{
				if (w.second > 0)
				{
					if (isHealing && healingBodyPart == w.first)
					{
						w.second--;
					}
					else
					{
						// FIXME: Deal damage in a unified way so we don't have to check
						// for unconscious and dead manually !
						agent->modified_stats.health -= w.second;
					}
				}
			}
		}
		// If fully healed
		if (!isFatallyWounded())
		{
			isHealing = false;
		}
		// If died or went unconscious
		if (isDead())
		{
			die(state, true, true);
		}
		if (!unconscious && isUnconscious())
		{
			fallUnconscious(state);
		}
	} // End of Fatal Wounds and Healing

	// Idling check
	if (missions.empty() && isConscious())
	{
		// Sanity checks
		if (goalFacing != facing)
		{
			LogError("Unit turning without a mission, wtf?");
		}
		if (target_body_state != current_body_state)
		{
			LogError("Unit changing body state without a mission, wtf?");
		}

		// Reach goal before everything else
		if (!atGoal)
		{
			addMission(state, BattleUnitMission::Type::ReachGoal);
		}

		// Try giving way if asked to
		// FIXME: Ensure we're not in a firefight before giving way!
		else if (giveWayRequest.size() > 0)
		{
			// If we're given a giveWay request 0, 0 it means we're asked to kneel temporarily
			if (giveWayRequest.size() == 1 && giveWayRequest.front().x == 0 &&
			    giveWayRequest.front().y == 0 &&
			    canAfford(BattleUnitMission::getBodyStateChangeCost(*this, target_body_state,
			                                                        BodyState::Kneeling)))
			{
				// Give time for that unit to pass
				addMission(state, BattleUnitMission::snooze(*this, TICKS_PER_SECOND), false);
				// Give way
				addMission(state, BattleUnitMission::changeStance(*this, BodyState::Kneeling));
			}
			else
			{
				auto from = tileObject->getOwningTile();
				for (auto newHeading : giveWayRequest)
				{
					for (int z = -1; z <= 1; z++)
					{
						if (z < 0 || z >= map.size.z)
						{
							continue;
						}
						// Try the new heading
						Vec3<int> pos = {position.x + newHeading.x, position.y + newHeading.y,
						                 position.z + z};
						auto to = map.getTile(pos);
						// Check if heading on our level is acceptable
						bool acceptable = BattleUnitTileHelper{map, *this}.canEnterTile(from, to) &&
						                  BattleUnitTileHelper{map, *this}.canEnterTile(to, from);
						// If not, check if we can go down one tile
						if (!acceptable && pos.z - 1 >= 0)
						{
							pos -= Vec3<int>{0, 0, 1};
							to = map.getTile(pos);
							acceptable = BattleUnitTileHelper{map, *this}.canEnterTile(from, to) &&
							             BattleUnitTileHelper{map, *this}.canEnterTile(to, from);
						}
						// If not, check if we can go up one tile
						if (!acceptable && pos.z + 2 < map.size.z)
						{
							pos += Vec3<int>{0, 0, 2};
							to = map.getTile(pos);
							acceptable = BattleUnitTileHelper{map, *this}.canEnterTile(from, to) &&
							             BattleUnitTileHelper{map, *this}.canEnterTile(to, from);
						}
						if (acceptable)
						{
							// 05: Turn to previous facing
							addMission(state, BattleUnitMission::turn(*this, facing), false);
							// 04: Return to our position after we're done
							addMission(state,
							           BattleUnitMission::gotoLocation(*this, position, 0),
							           false);
							// 03: Give time for that unit to pass
							addMission(state, BattleUnitMission::snooze(*this, 60), false);
							// 02: Turn to previous facing
							addMission(state, BattleUnitMission::turn(*this, facing), false);
							// 01: Give way (move 1 tile away)
							addMission(state,
							           BattleUnitMission::gotoLocation(*this, pos, 0));
						}
						if (!missions.empty())
						{
							break;
						}
					}
					if (!missions.empty())
					{
						break;
					}
				}
			}
			giveWayRequest.clear();
		}
		else // if not giving way
		{
			setMovementState(MovementState::None);
			// Kneel if not kneeling and should kneel
			if (kneeling_mode == KneelingMode::Kneeling &&
			    current_body_state != BodyState::Kneeling && canKneel() &&
			    canAfford(BattleUnitMission::getBodyStateChangeCost(*this, target_body_state,
			                                                        BodyState::Kneeling)))
			{
				setMission(state, BattleUnitMission::changeStance(*this, BodyState::Kneeling));
			}
			// Go prone if not prone and should stay prone
			else if (movement_mode == MovementMode::Prone &&
			         current_body_state != BodyState::Prone &&
			         kneeling_mode != KneelingMode::Kneeling && canProne(position, facing) &&
			         canAfford(BattleUnitMission::getBodyStateChangeCost(*this, target_body_state,
			                                                             BodyState::Prone)))
			{
				setMission(state, BattleUnitMission::changeStance(*this, BodyState::Prone));
			}
			// Stand up if not standing up and should stand up
			else if ((movement_mode == MovementMode::Walking ||
			          movement_mode == MovementMode::Running) &&
			         kneeling_mode != KneelingMode::Kneeling &&
			         current_body_state != BodyState::Standing &&
			         current_body_state != BodyState::Flying)
			{
				if (agent->isBodyStateAllowed(BodyState::Standing))
				{
					if (canAfford(BattleUnitMission::getBodyStateChangeCost(
					        *this, target_body_state, BodyState::Standing)))
					{
						setMission(state,
						           BattleUnitMission::changeStance(*this, BodyState::Standing));
					}
				}
				else
				{
					if (canAfford(BattleUnitMission::getBodyStateChangeCost(
					        *this, target_body_state, BodyState::Flying)))
					{
						setMission(state,
						           BattleUnitMission::changeStance(*this, BodyState::Flying));
					}
				}
			}
			// Stop flying if we can stand
			else if (current_body_state == BodyState::Flying &&
			         tileObject->getOwningTile()->getCanStand(isLarge()) &&
			         agent->isBodyStateAllowed(BodyState::Standing) &&
			         canAfford(BattleUnitMission::getBodyStateChangeCost(*this, target_body_state,
			                                                             BodyState::Standing)))
			{
				setMission(state, BattleUnitMission::changeStance(*this, BodyState::Standing));
			}
			// Stop being prone if legs are no longer supported and we haven't taken a mission yet
			if (current_body_state == BodyState::Prone && missions.empty())
			{
				bool hasSupport = true;
				for (auto t : tileObject->occupiedTiles)
				{
					if (!map.getTile(t)->getCanStand())
					{
						hasSupport = false;
						break;
					}
				}
				if (!hasSupport && canAfford(BattleUnitMission::getBodyStateChangeCost(
				                       *this, target_body_state, BodyState::Kneeling)))
				{
					setMission(state, BattleUnitMission::changeStance(*this, BodyState::Kneeling));
				}
			}
		}
	} // End of Idling

	// Movement and Body Animation
	{
		bool wasUsingLift = usingLift;
		usingLift = false;

		// Turn off Jetpacks
		if (current_body_state != BodyState::Flying)
		{
			flyingSpeedModifier = 0;
		}

		// If not running we will consume these twice as fast
		unsigned int moveTicksRemaining = ticks * agent->modified_stats.getActualSpeedValue() * 2;
		unsigned int bodyTicksRemaining = ticks;
		unsigned int handTicksRemaining = ticks;
		unsigned int turnTicksRemaining = ticks;

		// Unconscious units cannot move their hands or turn, they can only animate body or fall
		if (!isConscious())
		{
			handTicksRemaining = 0;
			turnTicksRemaining = 0;
		}

		unsigned int lastMoveTicksRemaining = 0;
		unsigned int lastBodyTicksRemaining = 0;
		unsigned int lastHandTicksRemaining = 0;
		unsigned int lastTurnTicksRemaining = 0;

		while (lastMoveTicksRemaining != moveTicksRemaining ||
		       lastBodyTicksRemaining != bodyTicksRemaining ||
		       lastHandTicksRemaining != handTicksRemaining ||
		       lastTurnTicksRemaining != turnTicksRemaining)
		{
			lastMoveTicksRemaining = moveTicksRemaining;
			lastBodyTicksRemaining = bodyTicksRemaining;
			lastHandTicksRemaining = handTicksRemaining;
			lastTurnTicksRemaining = turnTicksRemaining;

			// Begin falling or changing stance to flying if appropriate
			if (!falling)
			{
				// Check if should fall or start flying
				if (!canFly() || current_body_state != BodyState::Flying)
				{
					bool hasSupport = false;
					bool fullySupported = true;
					if (tileObject->getOwningTile()->getCanStand(isLarge()))
					{
						hasSupport = true;
					}
					else
					{
						fullySupported = false;
					}
					if (!atGoal)
					{
						if (map.getTile(goalPosition)->getCanStand(isLarge()))
						{
							hasSupport = true;
						}
						else
						{
							fullySupported = false;
						}
					}
					// If not flying and has no support - fall!
					if (!hasSupport && !canFly())
					{
						startFalling();
					}
					// If flying and not supported both on current and goal locations - start flying
					if (!fullySupported && canFly())
					{
						if (current_body_state == target_body_state)
						{
							setBodyState(BodyState::Flying);
							if (!missions.empty())
							{
								missions.front()->targetBodyState = current_body_state;
							}
						}
					}
				}
			}

			// Change body state
			if (bodyTicksRemaining > 0)
			{
				if (body_animation_ticks_remaining > bodyTicksRemaining)
				{
					body_animation_ticks_remaining -= bodyTicksRemaining;
					bodyTicksRemaining = 0;
				}
				else
				{
					if (body_animation_ticks_remaining > 0)
					{
						bodyTicksRemaining -= body_animation_ticks_remaining;
						setBodyState(target_body_state);
					}
					// Pop finished missions if present
					if (popFinishedMissions(state))
					{
						return;
					}
					// Try to get new body state change
					// Can do it if we're not firing and (either not changing hand state, or
					// starting to aim)
					if (firing_animation_ticks_remaining == 0 &&
					    (hand_animation_ticks_remaining == 0 ||
					     target_hand_state == HandState::Aiming))
					{
						BodyState nextState = BodyState::Downed;
						if (getNextBodyState(state, nextState))
						{
							beginBodyStateChange(nextState);
						}
					}
				}
			}

			// Change hand state
			if (handTicksRemaining > 0)
			{
				if (firing_animation_ticks_remaining > 0)
				{
					if (firing_animation_ticks_remaining > handTicksRemaining)
					{
						firing_animation_ticks_remaining -= handTicksRemaining;
						handTicksRemaining = 0;
					}
					else
					{
						handTicksRemaining -= firing_animation_ticks_remaining;
						firing_animation_ticks_remaining = 0;
						setHandState(HandState::Aiming);
					}
				}
				else
				{
					if (hand_animation_ticks_remaining > handTicksRemaining)
					{
						hand_animation_ticks_remaining -= handTicksRemaining;
						handTicksRemaining = 0;
					}
					else
					{
						if (hand_animation_ticks_remaining > 0)
						{
							handTicksRemaining -= hand_animation_ticks_remaining;
							hand_animation_ticks_remaining = 0;
							setHandState(target_hand_state);
						}
					}
				}
			}

			// Try moving
			if (moveTicksRemaining > 0)
			{
				// If falling then process falling
				if (falling)
				{
					// Falling consumes remaining move ticks
					auto fallTicksRemaining =
					    moveTicksRemaining / (agent->modified_stats.getActualSpeedValue() * 2);
					moveTicksRemaining = 0;

					// Process falling
					auto newPosition = position;
					while (fallTicksRemaining-- > 0)
					{
						fallingSpeed += FALLING_ACCELERATION_UNIT;
						newPosition -= Vec3<float>{0.0f, 0.0f, (fallingSpeed / TICK_SCALE)} /
						               VELOCITY_SCALE_BATTLE;
					}
					// Fell into a unit
					if (isConscious() &&
					    map.getTile(newPosition)->getUnitIfPresent(true, true, false, tileObject))
					{
						// FIXME: Proper stun damage (ensure it is!)
						stunDamageInTicks = 0;
						dealDamage(state, agent->current_stats.health * 3 / 2, false, 9001);
						fallUnconscious(state);
					}
					setPosition(newPosition);

					// Falling units can always turn
					goalPosition = position;
					atGoal = true;

					// Check if reached ground
					auto restingPosition =
					    tileObject->getOwningTile()->getRestingPosition(isLarge());
					if (position.z < restingPosition.z)
					{
						// Stopped falling
						falling = false;
						if (!isConscious())
						{
							// Bodies drop to the exact spot they fell upon
							setPosition({position.x, position.y, restingPosition.z});
						}
						else
						{
							setPosition(restingPosition);
						}
						resetGoal();
						// FIXME: Deal fall damage before nullifying this
						// FIXME: Play falling sound
						fallingSpeed = 0;
					}
				}

				// We are moving and not falling
				else if (current_movement_state != MovementState::None)
				{
					unsigned int speedModifier = 100;
					if (current_body_state == BodyState::Flying)
					{
						speedModifier = std::max((unsigned)1, flyingSpeedModifier);
					}

					Vec3<float> vectorToGoal = goalPosition - getPosition();
					unsigned int distanceToGoal = (unsigned)ceilf(glm::length(
					    vectorToGoal * VELOCITY_SCALE_BATTLE * (float)TICKS_PER_UNIT_TRAVELLED));
					unsigned int moveTicksConsumeRate =
					    current_movement_state == MovementState::Running ? 1 : 2;

					// Quick check, if moving strictly vertical then using lift
					if (distanceToGoal > 0 && current_body_state != BodyState::Flying &&
					    vectorToGoal.x == 0 && vectorToGoal.y == 0)
					{
						// FIXME: Actually read set option
						bool USER_OPTION_GRAVLIFT_SOUNDS = true;
						if (USER_OPTION_GRAVLIFT_SOUNDS && !wasUsingLift)
						{
							fw().soundBackend->playSample(agent->type->gravLiftSfx, getPosition(),
							                              0.25f);
						}
						usingLift = true;
						movement_ticks_passed = 0;
					}
					unsigned int movementTicksAccumulated = 0;
					if (distanceToGoal * moveTicksConsumeRate * 100 / speedModifier >
					    moveTicksRemaining)
					{
						if (flyingSpeedModifier != 100)
						{
							flyingSpeedModifier = std::min(
							    (unsigned)100, flyingSpeedModifier +
							                       moveTicksRemaining / moveTicksConsumeRate /
							                           FLYING_ACCELERATION_DIVISOR);
						}
						movementTicksAccumulated = moveTicksRemaining / moveTicksConsumeRate;
						auto dir = glm::normalize(vectorToGoal);
						Vec3<float> newPosition =
						    (float)(moveTicksRemaining / moveTicksConsumeRate) *
						    (float)(speedModifier / 100) * dir;
						newPosition /= VELOCITY_SCALE_BATTLE;
						newPosition /= (float)TICKS_PER_UNIT_TRAVELLED;
						newPosition += getPosition();
						setPosition(newPosition);
						moveTicksRemaining = moveTicksRemaining % moveTicksConsumeRate;
						atGoal = false;
					}
					else
					{
						if (distanceToGoal > 0)
						{
							movementTicksAccumulated = distanceToGoal;
							if (flyingSpeedModifier != 100)
							{
								flyingSpeedModifier =
								    std::min((unsigned)100,
								             flyingSpeedModifier +
								                 distanceToGoal / FLYING_ACCELERATION_DIVISOR);
							}
							moveTicksRemaining -= distanceToGoal * moveTicksConsumeRate;
							setPosition(goalPosition);
							goalPosition = getPosition();
						}
						atGoal = true;
						// Pop finished missions if present
						if (popFinishedMissions(state))
						{
							return;
						}
						// Try to get new destination
						Vec3<float> nextGoal;
						if (getNextDestination(state, nextGoal))
						{
							goalPosition = nextGoal;
							atGoal = false;
						}
					}

					// Scale ticks so that animations look proper on isometric sceen
					// facing down or up on screen
					if (facing.x == facing.y)
					{
						movement_ticks_passed += movementTicksAccumulated * 100 / 150;
					}
					// facing left or right on screen
					else if (facing.x == -facing.y)
					{
						movement_ticks_passed += movementTicksAccumulated * 141 / 150;
					}
					else
					{
						movement_ticks_passed += movementTicksAccumulated;
					}
					// Footsteps sound
					if (shouldPlaySoundNow() && current_body_state != BodyState::Flying)
					{
						if (agent->type->walkSfx.size() > 0)
						{
							fw().soundBackend->playSample(
							    agent->type
							        ->walkSfx[getWalkSoundIndex() % agent->type->walkSfx.size()],
							    getPosition(), 0.25f);
						}
						else
						{
							auto t = tileObject->getOwningTile();
							if (t->walkSfx && t->walkSfx->size() > 0)
							{
								fw().soundBackend->playSample(
								    t->walkSfx->at(getWalkSoundIndex() % t->walkSfx->size()),
								    getPosition(), 0.25f);
							}
						}
					}
				}
				// We are not moving and not falling
				else
				{
					// Check if we should adjust our current position
					if (goalPosition == getPosition())
					{
						goalPosition = tileObject->getOwningTile()->getRestingPosition(isLarge());
					}
					atGoal = goalPosition == getPosition();
					// If not at goal - go to goal
					if (!atGoal)
					{
						addMission(state, BattleUnitMission::Type::ReachGoal);
					}
					// If at goal - try to request new destination
					else
					{
						// Pop finished missions if present
						if (popFinishedMissions(state))
						{
							return;
						}
						// Try to get new destination
						Vec3<float> nextGoal;
						if (getNextDestination(state, nextGoal))
						{
							goalPosition = nextGoal;
							atGoal = false;
						}
					}
				}
			}

			// Try turning
			if (turnTicksRemaining > 0)
			{
				if (turning_animation_ticks_remaining > turnTicksRemaining)
				{
					turning_animation_ticks_remaining -= turnTicksRemaining;
					turnTicksRemaining = 0;
				}
				else
				{
					if (turning_animation_ticks_remaining > 0)
					{
						turnTicksRemaining -= turning_animation_ticks_remaining;
						setFacing(goalFacing);
					}
					// Pop finished missions if present
					if (popFinishedMissions(state))
					{
						return;
					}
					// Try to get new facing change
					Vec2<int> nextFacing;
					if (getNextFacing(state, nextFacing))
					{
						beginTurning(nextFacing);
					}
				}
			}

			updateDisplayedItem();
		}

	} // End of Movement and Body Animation

	// Firing

	static const Vec3<float> offsetTile = {0.5f, 0.5f, 0.0f};
	static const Vec3<float> offsetTileGround = {0.5f, 0.5f, 10.0f / 40.0f};
	Vec3<float> muzzleLocation = getMuzzleLocation();
	Vec3<float> targetPosition;
	switch (targetingMode)
	{
		case TargetingMode::Unit:
			targetPosition = targetUnit->tileObject->getVoxelCentrePosition();
			break;
		case TargetingMode::TileCenter:
		{
			// Shoot parallel to the ground
			float unitZ = muzzleLocation.z;
			unitZ -= (int)unitZ;
			targetPosition = (Vec3<float>)targetTile + offsetTile + Vec3<float>{0.0f, 0.0f, unitZ};
			break;
		}
		case TargetingMode::TileGround:
			targetPosition = (Vec3<float>)targetTile + offsetTileGround;
			break;
		case TargetingMode::NoTarget:
			// Ain't need to do anythin!
			break;
	}

	// For simplicity, prepare weapons we can use
	// We can use a weapon if we're set to fire this hand, and it's a weapon that can be fired

	auto weaponRight = agent->getFirstItemInSlot(AEquipmentSlotType::RightHand);
	auto weaponLeft = agent->getFirstItemInSlot(AEquipmentSlotType::LeftHand);
	switch (weaponStatus)
	{
		case WeaponStatus::FiringBothHands:
			if (weaponRight && weaponRight->needsReload())
			{
				weaponRight->loadAmmo(state);
			}
			if (weaponRight && !weaponRight->canFire())
			{
				weaponRight = nullptr;
			}
			if (weaponLeft && weaponLeft->needsReload())
			{
				weaponLeft->loadAmmo(state);
			}
			if (weaponLeft && !weaponLeft->canFire())
			{
				weaponLeft = nullptr;
			}
			break;
		case WeaponStatus::FiringRightHand:
			if (weaponRight && weaponRight->needsReload())
			{
				weaponRight->loadAmmo(state);
			}
			if (weaponRight && !weaponRight->canFire())
			{
				weaponRight = nullptr;
			}
			weaponLeft = nullptr;
			break;
		case WeaponStatus::FiringLeftHand:
			if (weaponLeft && weaponLeft->needsReload())
			{
				weaponLeft->loadAmmo(state);
			}
			if (weaponLeft && !weaponLeft->canFire())
			{
				weaponLeft = nullptr;
			}
			weaponRight = nullptr;
			break;
		case WeaponStatus::NotFiring:
			// Ain't need to do anythin!
			break;
	}

	// Firing - check if we should stop firing
	if (isAttacking())
	{
		if (targetingMode == TargetingMode::Unit)
		{
			if (ticksTillNextTargetCheck > ticks)
			{
				ticksTillNextTargetCheck -= ticks;
			}
			else
			{
				ticksTillNextTargetCheck = 0;
			}
		}

		// Do consequent checks, if previous is ok
		bool canFire = true;

		// We cannot fire if we have no weapon capable of firing
		canFire = canFire && (weaponLeft || weaponRight);

		// We cannot fire if it's time to check target unit and it's not in LOS anymore or not
		// conscious
		// Also, at this point we will turn to target tile if targeting tile
		if (canFire)
		{
			// Note: If not targeting a unit, this will only be done once after start,
			// and again once each time unit stops moving
			if (ticksTillNextTargetCheck == 0)
			{
				ticksTillNextTargetCheck = LOS_CHECK_INTERVAL_TRACKING;
				if (targetingMode == TargetingMode::Unit)
				{
					canFire = canFire && targetUnit->isConscious();
					// FIXME: IMPLEMENT LOS CHECKING
					canFire = canFire && true; // Here we check if target is visible
					if (canFire)
					{
						targetTile = targetUnit->position;
					}
				}
				// Check if we are in range
				if (canFire)
				{
					if (weaponRight && !weaponRight->canFire(targetPosition))
					{
						weaponRight = nullptr;
					}
					if (weaponLeft && !weaponLeft->canFire(targetPosition))
					{
						weaponLeft = nullptr;
					}
					// We cannot fire if both weapons are out of range
					canFire = canFire && (weaponLeft || weaponRight);
				}
				// Check if we should turn to target tile (only do this if stationary)
				if (canFire && current_movement_state == MovementState::None)
				{
					auto m = BattleUnitMission::turn(*this, targetTile);
					if (!m->isFinished(state, *this, false))
					{
						addMission(state, m);
					}
				}
			}
		}

		// Finally if any of the checks failed - stop firing
		if (!canFire)
		{
			stopAttacking();
		}
	}

	// Firing - process unit that is firing
	if (isAttacking())
	{
		// Should we start firing a gun?
		if (target_hand_state == HandState::Aiming)
		{
			if (weaponRight && !weaponRight->isFiring())
			{
				weaponRight->startFiring(fire_aiming_mode);
			}
			if (weaponLeft && !weaponLeft->isFiring())
			{
				weaponLeft->startFiring(fire_aiming_mode);
			}
		}

		// Is a gun ready to fire?
		bool weaponFired = false;
		if (firing_animation_ticks_remaining == 0 && target_hand_state == HandState::Aiming)
		{
			sp<AEquipment> firingWeapon = nullptr;
			if (weaponRight && weaponRight->readyToFire)
			{
				firingWeapon = weaponRight;
				weaponRight = nullptr;
			}
			else if (weaponLeft && weaponLeft->readyToFire)
			{
				firingWeapon = weaponLeft;
				weaponLeft = nullptr;
			}
			// Check if facing the right way
			if (firingWeapon)
			{
				auto targetVector = targetPosition - muzzleLocation;
				targetVector = {targetVector.x, targetVector.y, 0.0f};
				// Target must be within frontal arc
				if (glm::angle(glm::normalize(targetVector),
				               glm::normalize(Vec3<float>{facing.x, facing.y, 0})) >= M_PI / 2)
				{
					firingWeapon = nullptr;
				}
			}
			// If still OK - fire!
			if (firingWeapon)
			{
				firingWeapon->fire(state, targetPosition,
				                   targetingMode == TargetingMode::Unit ? targetUnit : nullptr);
				displayedItem = firingWeapon->type;
				setHandState(HandState::Firing);
				weaponFired = true;
			}
		}

		// If fired weapon at ground or ally - stop firing that hand
		if (weaponFired && (targetingMode != TargetingMode::Unit || targetUnit->owner == owner))
		{
			switch (weaponStatus)
			{
				case WeaponStatus::FiringBothHands:
					if (!weaponRight)
					{
						if (!weaponLeft)
						{
							stopAttacking();
						}
						else
						{
							weaponStatus = WeaponStatus::FiringLeftHand;
						}
					}
					else if (!weaponLeft)
					{
						weaponStatus = WeaponStatus::FiringRightHand;
					}
					break;
				case WeaponStatus::FiringLeftHand:
					if (!weaponLeft)
					{
						stopAttacking();
					}
					break;
				case WeaponStatus::FiringRightHand:
					if (!weaponRight)
					{
						stopAttacking();
					}
					break;
				case WeaponStatus::NotFiring:
					LogError("Weapon fired while not firing?");
					break;
			}
		}

		// Should we start aiming?
		if (firing_animation_ticks_remaining == 0 && hand_animation_ticks_remaining == 0 &&
		    body_animation_ticks_remaining == 0 && current_hand_state != HandState::Aiming &&
		    current_movement_state != MovementState::Running &&
		    current_movement_state != MovementState::Strafing &&
		    !(current_body_state == BodyState::Prone &&
		      current_movement_state != MovementState::None))
		{
			beginHandStateChange(HandState::Aiming);
		}

	} // end if Firing - process firing

	// Not Firing (or may have just stopped firing)
	if (!isAttacking())
	{
		// Should we stop aiming?
		if (aiming_ticks_remaining > 0)
		{
			aiming_ticks_remaining -= ticks;
		}
		else if (firing_animation_ticks_remaining == 0 && hand_animation_ticks_remaining == 0 &&
		         current_hand_state == HandState::Aiming)
		{
			beginHandStateChange(HandState::AtEase);
		}
	} // end if not Firing

	// FIXME: Soldier "thinking" (auto-attacking, auto-turning)
}

void BattleUnit::startFalling()
{
	setMovementState(MovementState::None);
	falling = true;
}

void BattleUnit::updateDisplayedItem()
{
	auto lastDisplayedItem = displayedItem;
	bool foundThrownItem = false;
	if (missions.size() > 0)
	{
		for (auto &m : missions)
		{
			if (m->type != BattleUnitMission::Type::ThrowItem || !m->item)
			{
				continue;
			}
			displayedItem = m->item->type;
			foundThrownItem = true;
			break;
		}
	}
	if (!foundThrownItem)
	{
		// If we're firing - try to keep last displayed item same, even if not dominant
		displayedItem = agent->getDominantItemInHands(
		    firing_animation_ticks_remaining > 0 ? lastDisplayedItem : nullptr);
	}
	// If displayed item changed or we are throwing - bring hands into "AtEase" state immediately
	if (foundThrownItem || displayedItem != lastDisplayedItem)
	{
		if (hand_animation_ticks_remaining > 0 || current_hand_state != HandState::AtEase)
		{
			setHandState(HandState::AtEase);
		}
	}
}

void BattleUnit::destroy(GameState &)
{
	this->tileObject->removeFromMap();
	this->shadowObject->removeFromMap();
	this->tileObject.reset();
	this->shadowObject.reset();
}

void BattleUnit::tryToRiseUp(GameState &state)
{
	// Do not rise up if unit is standing on us
	if (tileObject->getOwningTile()->getUnitIfPresent(true, true, false, tileObject))
		return;

	// Find state we can rise into (with animation)
	auto targetState = BodyState::Standing;
	while (targetState != BodyState::Downed &&
	       agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
	                                                    targetState, current_hand_state,
	                                                    current_movement_state, facing) == 0)
	{
		switch (targetState)
		{
			case BodyState::Standing:
				if (agent->isBodyStateAllowed(BodyState::Flying))
				{
					targetState = BodyState::Flying;
					continue;
				}
			// Intentional fall-through
			case BodyState::Flying:
				if (agent->isBodyStateAllowed(BodyState::Kneeling))
				{
					targetState = BodyState::Kneeling;
					continue;
				}
			// Intentional fall-through
			case BodyState::Kneeling:
				if (canProne(position, facing))
				{
					targetState = BodyState::Prone;
					continue;
				}
			// Intentional fall-through
			case BodyState::Prone:
				// If we arrived here then we have no animation for standing up
				targetState = BodyState::Downed;
				continue;
			case BodyState::Downed:
			case BodyState::Jumping:
			case BodyState::Throwing:
				LogError("Not possible to reach this?");
				break;
		}
	}
	// Find state we can rise into (with no animation)
	if (targetState == BodyState::Downed)
	{
		if (agent->isBodyStateAllowed(BodyState::Standing))
		{
			targetState = BodyState::Standing;
		}
		else if (agent->isBodyStateAllowed(BodyState::Flying))
		{
			targetState = BodyState::Flying;
		}
		else if (agent->isBodyStateAllowed(BodyState::Kneeling))
		{
			targetState = BodyState::Kneeling;
		}
		else if (canProne(position, facing))
		{
			targetState = BodyState::Prone;
		}
		else
		{
			LogError("Unit cannot stand up???");
		}
	}

	missions.clear();
	addMission(state, BattleUnitMission::changeStance(*this, targetState));
	// Unit will automatically move to goal after rising due to logic in update()
}

void BattleUnit::dropDown(GameState &state)
{
	resetGoal();
	setMovementState(MovementState::None);
	setHandState(HandState::AtEase);
	setBodyState(target_body_state);
	// Check if we can drop from current state
	while (agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
	                                                    BodyState::Downed, current_hand_state,
	                                                    current_movement_state, facing) == 0)
	{
		switch (current_body_state)
		{
			case BodyState::Jumping:
			case BodyState::Throwing:
			case BodyState::Flying:
				if (agent->isBodyStateAllowed(BodyState::Standing))
				{
					setBodyState(BodyState::Standing);
					continue;
				}
			// Intentional fall-through
			case BodyState::Standing:
				if (agent->isBodyStateAllowed(BodyState::Kneeling))
				{
					setBodyState(BodyState::Kneeling);
					continue;
				}
			// Intentional fall-through
			case BodyState::Kneeling:
				setBodyState(BodyState::Prone);
				continue;
			case BodyState::Prone:
			case BodyState::Downed:
				LogError("Not possible to reach this?");
				break;
		}
		break;
	}
	// Drop all gear
	while (!agent->equipment.empty())
	{
		addMission(state, BattleUnitMission::dropItem(*this, agent->equipment.front()));
	}
	// Drop gear used by missions
	std::list<sp<AEquipment>> itemsToDrop;
	for (auto &m : missions)
	{
		if (m->item && m->item->equippedSlotType != AEquipmentSlotType::None)
		{
			itemsToDrop.push_back(m->item);
		}
	}
	missions.clear();
	for (auto it : itemsToDrop)
	{
		addMission(state, BattleUnitMission::dropItem(*this, it));
	}
	addMission(state, BattleUnitMission::changeStance(*this, BodyState::Downed));
}

void BattleUnit::retreat(GameState &state)
{
	std::ignore = state;
	tileObject->removeFromMap();
	retreated = true;
	removeFromSquad(*state.current_battle);
	// FIXME: Trigger retreated event
}

void BattleUnit::die(GameState &state, bool violently, bool bledToDeath)
{
	if (violently)
	{
		// FIXME: Explode if nessecary, or spawn shit
		LogWarning("Implement violent deaths!");
	}
	// Clear focus
	for (auto u : focusedByUnits)
	{
		u->focusUnit.clear();
	}
	focusedByUnits.clear();
	// Emit sound
	if (agent->type->dieSfx.find(agent->gender) != agent->type->dieSfx.end() &&
	    !agent->type->dieSfx.at(agent->gender).empty())
	{
		fw().soundBackend->playSample(
		    listRandomiser(state.rng, agent->type->dieSfx.at(agent->gender)), position);
	}
	// FIXME: do what has to be done when unit dies
	LogWarning("Implement a UNIT DIED notification!");
	dropDown(state);
}

void BattleUnit::fallUnconscious(GameState &state)
{
	// FIXME: do what has to be done when unit goes unconscious
	dropDown(state);
}

void BattleUnit::beginBodyStateChange(BodyState state)
{
	// Cease hand animation immediately
	if (hand_animation_ticks_remaining != 0)
		setHandState(target_hand_state);

	// Find which animation is possible
	int frameCount = agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
	                                                              state, current_hand_state,
	                                                              current_movement_state, facing);
	// No such animation
	// Try stopping movement
	if (frameCount == 0 && current_movement_state != MovementState::None)
	{
		frameCount = agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
		                                                          state, current_hand_state,
		                                                          MovementState::None, facing);
		if (frameCount != 0)
		{
			setMovementState(MovementState::None);
		}
	}
	// Try stopping aiming
	if (frameCount == 0 && current_hand_state != HandState::AtEase)
	{
		frameCount = agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
		                                                          state, HandState::AtEase,
		                                                          current_movement_state, facing);
		if (frameCount != 0)
		{
			setHandState(HandState::AtEase);
		}
	}

	int ticks = frameCount * TICKS_PER_FRAME_UNIT;
	if (ticks > 0 && current_body_state != state)
	{
		target_body_state = state;
		body_animation_ticks_remaining = ticks;
		// Updates bounds etc.
		if (tileObject)
		{
			setPosition(position);
		}
	}
	else
	{
		setBodyState(state);
	}
}

void BattleUnit::setBodyState(BodyState state)
{
	current_body_state = state;
	target_body_state = state;
	body_animation_ticks_remaining = 0;
	// Updates bounds etc.
	if (tileObject)
	{
		setPosition(position);
	}
}

void BattleUnit::beginHandStateChange(HandState state)
{
	int frameCount = agent->getAnimationPack()->getFrameCountHands(
	    displayedItem, current_body_state, current_hand_state, state, current_movement_state,
	    facing);
	int ticks = frameCount * TICKS_PER_FRAME_UNIT;

	if (ticks > 0 && current_hand_state != state)
	{
		target_hand_state = state;
		hand_animation_ticks_remaining = ticks;
	}
	else
	{
		setHandState(state);
	}
	aiming_ticks_remaining = 0;
}

void BattleUnit::setHandState(HandState state)
{
	current_hand_state = state;
	target_hand_state = state;
	hand_animation_ticks_remaining = 0;
	firing_animation_ticks_remaining =
	    state != HandState::Firing
	        ? 0
	        : agent->getAnimationPack()->getFrameCountFiring(displayedItem, current_body_state,
	                                                         current_movement_state, facing) *
	              TICKS_PER_FRAME_UNIT;
	aiming_ticks_remaining = state == HandState::Aiming ? TICKS_PER_SECOND / 3 : 0;
}

void BattleUnit::beginTurning(Vec2<int> newFacing)
{
	goalFacing = newFacing;
	turning_animation_ticks_remaining = TICKS_PER_FRAME_UNIT;
}

void BattleUnit::setFacing(Vec2<int> newFacing)
{
	facing = newFacing;
	goalFacing = newFacing;
	turning_animation_ticks_remaining = 0;
}

void BattleUnit::setMovementState(MovementState state)
{
	current_movement_state = state;
	switch (state)
	{
		case MovementState::None:
			movement_ticks_passed = 0;
			movement_sounds_played = 0;
			ticksTillNextTargetCheck = 0;
			break;
		case MovementState::Running:
		case MovementState::Strafing:
			if (current_hand_state != HandState::AtEase || target_hand_state != HandState::AtEase)
			{
				setHandState(HandState::AtEase);
			}
			break;
		default:
			break;
	}
}

unsigned int BattleUnit::getWalkSoundIndex()
{
	if (current_movement_state == MovementState::Running)
	{
		return ((movement_sounds_played + UNITS_TRAVELLED_PER_SOUND_RUNNING_DIVISOR - 1) /
		        UNITS_TRAVELLED_PER_SOUND_RUNNING_DIVISOR) %
		       2;
	}
	else
	{
		return movement_sounds_played % 2;
	}
}

Vec3<float> BattleUnit::getMuzzleLocation() const
{
	return position +
	       Vec3<float>{0.0f, 0.0f,
	                   ((float)agent->type->bodyType->muzzleZPosition.at(current_body_state)) /
	                       40.0f};
}

Vec3<float> BattleUnit::getThrownItemLocation() const
{
	return position +
	       Vec3<float>{0.0f, 0.0f,
	                   ((float)agent->type->bodyType->height.at(BodyState::Throwing) - 4.0f) /
	                       2.0f / 40.0f};
}

bool BattleUnit::shouldPlaySoundNow()
{
	bool play = false;
	unsigned int sounds_to_play = getDistanceTravelled() / UNITS_TRAVELLED_PER_SOUND;
	if (sounds_to_play != movement_sounds_played)
	{
		unsigned int divisor = (current_movement_state == MovementState::Running)
		                           ? UNITS_TRAVELLED_PER_SOUND_RUNNING_DIVISOR
		                           : 1;
		play = ((sounds_to_play + divisor - 1) % divisor) == 0;
		movement_sounds_played = sounds_to_play;
	}
	return play;
}

bool BattleUnit::popFinishedMissions(GameState &state)
{
	while (missions.size() > 0 && missions.front()->isFinished(state, *this))
	{
		LogWarning("Unit mission \"%s\" finished", missions.front()->getName().cStr());
		missions.pop_front();

		// We may have retreated as a result of finished mission
		if (retreated)
			return true;

		if (!missions.empty())
		{
			missions.front()->start(state, *this);
			continue;
		}
		else
		{
			LogWarning("No next unit mission, going idle");
			break;
		}
	}
	return false;
}

bool BattleUnit::getNextDestination(GameState &state, Vec3<float> &dest)
{
	if (missions.empty())
	{
		return false;
	}
	return missions.front()->getNextDestination(state, *this, dest);
}

bool BattleUnit::getNextFacing(GameState &state, Vec2<int> &dest)
{
	if (missions.empty())
	{
		return false;
	}
	return missions.front()->getNextFacing(state, *this, dest);
}

bool BattleUnit::getNextBodyState(GameState &state, BodyState &dest)
{
	if (missions.empty())
	{
		return false;
	}
	return missions.front()->getNextBodyState(state, *this, dest);
}

bool BattleUnit::addMission(GameState &state, BattleUnitMission::Type type)
{
	switch (type)
	{
		case BattleUnitMission::Type::RestartNextMission:
			return addMission(state, BattleUnitMission::restartNextMission(*this));
		case BattleUnitMission::Type::ReachGoal:
			return addMission(state, BattleUnitMission::reachGoal(*this));
		case BattleUnitMission::Type::ThrowItem:
		case BattleUnitMission::Type::Snooze:
		case BattleUnitMission::Type::ChangeBodyState:
		case BattleUnitMission::Type::Turn:
		case BattleUnitMission::Type::AcquireTU:
		case BattleUnitMission::Type::GotoLocation:
		case BattleUnitMission::Type::Teleport:
			LogError("Cannot add mission by type if it requires parameters");
			break;
	}
	return false;
}

bool BattleUnit::cancelMissions(GameState &state)
{
	if (missions.empty())
	{
		return true;
	}

	// Figure out if we can cancel the mission in front
	bool letFinish = false;
	switch (missions.front()->type)
	{
		// Missions that cannot be cancelled
		case BattleUnitMission::Type::ThrowItem:
			return false;
		// Missions that must be let finish (unless forcing)
		case BattleUnitMission::Type::ChangeBodyState:
		case BattleUnitMission::Type::Turn:
		case BattleUnitMission::Type::GotoLocation:
		case BattleUnitMission::Type::ReachGoal:
			letFinish = true;
			break;
		// Missions that can be cancelled
		case BattleUnitMission::Type::Snooze:
		case BattleUnitMission::Type::DropItem:
		case BattleUnitMission::Type::Teleport:
		case BattleUnitMission::Type::RestartNextMission:
		case BattleUnitMission::Type::AcquireTU:
			break;
	}

	// Figure out what to do with the unfinished mission
	if (letFinish)
	{
		auto &m = missions.front();
		// If turning - downgrade to a turning mission
		if (facing != goalFacing)
		{
			m->type = BattleUnitMission::Type::Turn;
			m->targetFacing = goalFacing;
			if (m->costPaidUpFront > 0)
			{
				// Refund queued action, subtract turning cost
				agent->modified_stats.time_units += m->costPaidUpFront - 1;
			}
		}
		// If changing body - downgrade to a body state change mission
		else if (current_body_state != target_body_state)
		{
			m->type = BattleUnitMission::Type::ChangeBodyState;
			m->targetBodyState = target_body_state;
		}
		else
		{
			letFinish = false;
		}
	}

	// Cancel missions
	while (missions.size() > letFinish ? 1 : 0)
	{
		agent->modified_stats.time_units += missions.back()->costPaidUpFront;
		missions.pop_back();
	}
	if (missions.empty() && !atGoal)
	{
		addMission(state, BattleUnitMission::Type::ReachGoal);
	}
	return true;
}

bool BattleUnit::setMission(GameState &state, BattleUnitMission *mission)
{
	// Check if mission was actually passed
	// We can receive nullptr here in case mission was impossible
	if (!mission)
	{
		return false;
	}

	// Special checks and actions based on mission type
	switch (mission->type)
	{
	case BattleUnitMission::Type::Turn:
		stopAttacking();
		break;
	case BattleUnitMission::Type::ThrowItem:
		// We already checked if item is throwable inside the mission creation
		break;
	}

	if (!cancelMissions(state))
	{
		return false;
	}

	// There is a mission remaining that wants to let it finish
	if (!missions.empty())
	{
		switch (mission->type)
		{
			// Instant throw always cancels if agent can afford it
			case BattleUnitMission::Type::ThrowItem:
			{
				// FIXME: actually read the option
				bool USER_OPTION_ALLOW_INSTANT_THROWS = false;
				if (USER_OPTION_ALLOW_INSTANT_THROWS &&
				    canAfford(BattleUnitMission::getThrowCost(*this)))
				{
					setBodyState(current_body_state);
					setFacing(facing);
					missions.clear();
				}
				break;
			}
			// Turning can be cancelled if our mission will require us to turn in a different
			// direction
			case BattleUnitMission::Type::Turn:
			case BattleUnitMission::Type::GotoLocation:
			case BattleUnitMission::Type::ReachGoal:
			{
				Vec2<int> nextFacing;
				bool haveNextFacing = true;
				switch (mission->type)
				{
					case BattleUnitMission::Type::Turn:
						nextFacing = BattleUnitMission::getFacingStep(*this, mission->targetFacing);
						break;
					case BattleUnitMission::Type::GotoLocation:
						// We have to start it in order to see where we're going
						mission->start(state, *this);
						if (mission->currentPlannedPath.empty())
						{
							haveNextFacing = false;
							break;
						}
						nextFacing = BattleUnitMission::getFacingStep(
						    *this, BattleUnitMission::getFacing(
						               *this, mission->currentPlannedPath.front()));
						break;
					case BattleUnitMission::Type::ReachGoal:
						nextFacing = BattleUnitMission::getFacingStep(
						    *this, BattleUnitMission::getFacing(*this, position, goalPosition));
						break;
					default: // don't cry about unhandled case, compiler
						break;
				}
				// If we are turning towards something that will not be our next facing when we try
				// to execute our mission then we're better off canceling it
				if (haveNextFacing && nextFacing != goalFacing)
				{
					setFacing(facing);
					missions.clear();
				}
				break;
			}
			default:
				break;
		}
	}

	// Finally, add the mission
	return addMission(state, mission);
}

bool BattleUnit::addMission(GameState &state, BattleUnitMission *mission, bool start)
{
	switch (mission->type)
	{
		// Reach goal can only be added if it can overwrite the mission
		case BattleUnitMission::Type::ReachGoal:
		{
			if (missions.size() > 0)
			{
				switch (missions.front()->type)
				{
					// Missions that prevent going to goal
					case BattleUnitMission::Type::Snooze:
					case BattleUnitMission::Type::ThrowItem:
					case BattleUnitMission::Type::ChangeBodyState:
					case BattleUnitMission::Type::ReachGoal:
					case BattleUnitMission::Type::DropItem:
					case BattleUnitMission::Type::Teleport:
					case BattleUnitMission::Type::RestartNextMission:
					case BattleUnitMission::Type::GotoLocation:
					case BattleUnitMission::Type::Turn:
						return false;
					// Missions that can be overwritten
					case BattleUnitMission::Type::AcquireTU:
						break;
				}
			}
			missions.emplace_front(mission);
			if (start)
			{
				mission->start(state, *this);
			}
			break;
		}
		// Missions that can be added to the back at any time
		case BattleUnitMission::Type::Turn:
		case BattleUnitMission::Type::ChangeBodyState:
		case BattleUnitMission::Type::ThrowItem:
		case BattleUnitMission::Type::GotoLocation:
		case BattleUnitMission::Type::Teleport:
			missions.emplace_back(mission);
			// Missions added to back normally start only if they are the only mission in the queue
			// Teleport always starts immediately, even if the agent is waiting to finish something
			if (start &&
			    (missions.size() == 1 || mission->type == BattleUnitMission::Type::Teleport))
			{
				mission->start(state, *this);
			}
			break;
		// Missions that can be added to the front at any time
		case BattleUnitMission::Type::Snooze:
		case BattleUnitMission::Type::AcquireTU:
		case BattleUnitMission::Type::RestartNextMission:
		case BattleUnitMission::Type::DropItem:
			missions.emplace_front(mission);
			if (start)
			{
				mission->start(state, *this);
			}
			break;
	}
	return true;
}

void BattleUnit::groupMove(std::list<StateRef<BattleUnit>>&units, Vec3<int> targetLocation, bool demandGiveWay)
{
	static const std::list<Vec2<int>> targetOffsets = { 
		{ -1, 1}, { 1, 1 }, { 0, 2 }, { -1, -1 }, { -2, 0 }, { 0, -2 }, { 1, -1 }, { 2, 0 },
		{ -4, 0 }, { -3, -1 }, { -2, -2}, { -1, -3}, 
		{ 0, -4 }, { 1, -3 }, { 2, -2 }, { 3, -1 }, 
		{ 4, 0 }, { 3, 1 }, { 2, 2 }, { 1, 3 }, 
		{ 0, 4 }, { -1, 3 }, { -2, 2 }, { -3, 1 },
	};

	// 2 is linear move, 3 diagonal move, 4 diagonal + vertical
	static const int maxAllowance = 6;

	// Algorithm:
	//
	// - Find first unit that can reach the goal or arrive within allowed distance
	//
	// - Point iterator to first list element
	//
	// - For every unit other than the unit found:
	//	- Path from targetLocation to iterator
	//	- If path is less than 50% unoptimal, path unit to iterator
	//	  Else do nothing
	//  - Increment iterator
	//  - Break if iterator reached end
	//  - Continue if pathed to iterator (above)
	
	
}

}

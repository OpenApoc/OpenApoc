#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/state/battle/battleunit.h"
#include "framework/framework.h"
#include "framework/sound.h"
#include "game/state/aequipment.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battleunitanimationpack.h"
#include "game/state/city/projectile.h"
#include "game/state/gamestate.h"
#include "game/state/rules/damage.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/state/tileview/tileobject_shadow.h"
#include "library/line.h"
#include "library/strings_format.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace OpenApoc
{

static const std::set<TileObject::Type> mapPartSet = {
    TileObject::Type::Ground, TileObject::Type::LeftWall, TileObject::Type::RightWall,
    TileObject::Type::Feature};
static const std::set<TileObject::Type> unitSet = {TileObject::Type::Unit};

sp<BattleUnit> BattleUnit::get(const GameState &state, const UString &id)
{
	auto it = state.current_battle->units.find(id);
	if (it == state.current_battle->units.end())
	{
		LogError("No agent_type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &BattleUnit::getPrefix()
{
	static UString prefix = "BATTLEUNIT_";
	return prefix;
}
const UString &BattleUnit::getTypeName()
{
	static UString name = "BattleUnit";
	return name;
}
const UString &BattleUnit::getId(const GameState &state, const sp<BattleUnit> ptr)
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

// Called before unit is added to the map itself
void BattleUnit::init(GameState &state)
{
	owner = agent->owner;
	agent->unit = { &state, id };
	aiList.init(state, *this);
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

void BattleUnit::setPosition(GameState &state, const Vec3<float> &pos)
{
	auto oldPosition = position;
	position = pos;
	if (!tileObject)
	{
		LogError("setPosition called on unit with no tile object");
		return;
	}

	tileObject->setPosition(pos);

	if (shadowObject)
	{
		shadowObject->setPosition(tileObject->getCenter());
	}
	if ((Vec3<int>)oldPosition != (Vec3<int>)position)
	{
		refreshUnitVisibilityAndVision(state, oldPosition);
	}
}

void BattleUnit::refreshUnitVisibility(GameState &state, Vec3<float> oldPosition)
{
	// Update other units's vision of this unit
	// FIXME: Do this properly? Only update vision to this unit, not to everything?
	state.current_battle->queueVisionRefresh(position);
	state.current_battle->queueVisionRefresh(oldPosition);
}

bool BattleUnit::isWithinVision(Vec3<int> pos)
{
	auto diff = pos - (Vec3<int>)position;
	// Distance quick check
	if (diff.x * diff.x + diff.y * diff.y > VIEW_DISTANCE * VIEW_DISTANCE)
	{
		return false;
	}
	// Facing: Any, check if we're at the correct side
	if (diff.x * facing.x < 0 || diff.y * facing.y < 0)
	{
		return false;
	}
	// Facing: Diagonalley
	if (facing.x != 0 && facing.y != 0)
	{
		// Nothing to be done, we already checked above
	}
	// Facing: Along one of axes
	else
	{
		// Facing: Along X
		if (facing.x != 0)
		{
			// Already checked if we're at the correct side above
			// Now we only need to check if we're inside the cone
			if (std::abs(diff.x) < std::abs(diff.y))
			{
				return false;
			}
		}
		// Facing: Along Y
		else
		{
			// Already checked if we're at the correct side above
			// Now we only need to check if we're inside the cone
			if (std::abs(diff.x) > std::abs(diff.y))
			{
				return false;
			}
		}
	}
	return true;
}

void BattleUnit::calculateVisionToTerrain(GameState &state, Battle &battle, TileMap &map,
                                          Vec3<float> eyesPos)
{
	std::set<int> discoveredBlocks;
	auto &visibleBlocks = battle.visibleBlocks.at(owner);

	// Update unit's vision of los block he's standing in
	{
		auto idx = battle.getLosBlockID(position.x, position.y, position.z);
		if (!visibleBlocks.at(idx))
		{
			visibleBlocks.at(idx) = true;
			discoveredBlocks.insert(idx);
		}
	}

	// Calc los to other blocks we haven't seen yet
	for (int idx = 0; idx < visibleBlocks.size(); idx++)
	{
		// Block already seen
		if (visibleBlocks.at(idx))
		{
			continue;
		}

		// Get block and its center
		auto &l = *battle.losBlocks.at(idx);
		auto centerXY = Vec3<int>{(l.start.x + l.end.x) / 2, (l.start.y + l.end.y) / 2, 0};
		// Set target to center
		bool targetFound = false;
		auto target = centerXY;
		// Set target's Z to our level (or closest possible)
		int posZ = position.z;
		if (posZ >= l.start.z && posZ < l.end.z)
		{
			target.z = posZ;
		}
		else if (posZ < l.start.z)
		{
			target.z = l.start.z;
		}
		else
		{
			target.z = l.end.z - 1;
		}
		// Try to target center of LOS block (on our Z level)
		if (isWithinVision(target))
		{
			targetFound = true;
		}
		// Try to target point within that is closest to our sight's middlepoint
		else
		{
			// Get point in the middle of our sight forward
			int dist =
			    facing.x != 0 && facing.y != 0 ? VIEW_DISTANCE * 100 / 141 / 2 : VIEW_DISTANCE / 2;
			auto sightMiddleXY = (Vec3<int>)position;
			sightMiddleXY.x += facing.x * dist;
			sightMiddleXY.y += facing.y * dist;
			// Get point closest to that point
			target.x =
			    std::abs(l.start.x - sightMiddleXY.x) < std::abs(l.end.x - 1 - sightMiddleXY.x)
			        ? l.start.x
			        : l.end.x - 1;
			target.y =
			    std::abs(l.start.y - sightMiddleXY.y) < std::abs(l.end.y - 1 - sightMiddleXY.y)
			        ? l.start.y
			        : l.end.y - 1;
			if (posZ >= l.start.z && posZ < l.end.z)
			{
				target.z = posZ;
			}
			else if (posZ < l.start.z)
			{
				target.z = l.start.z;
			}
			else
			{
				target.z = l.end.z - 1;
			}
			// Try to target that point
			if (isWithinVision(target))
			{
				targetFound = true;
			}
		}

		// If target is found then we can try to los to this block
		if (targetFound)
		{
			auto c = map.findCollision(eyesPos, {target.x + 0.5f, target.y + 0.5f, target.z + 0.5f},
			                           mapPartSet, tileObject, true, false, VIEW_DISTANCE);

			// FIXME: Handle collisions with left/right/ground that prevent seeing inside
			// If going positive on axes, we must shorten our beam a little bit, so that if
			// collision was with a wall or ground, it would not consider a block as seen

			if (!c.outOfRange && (!c || l.contains(c.position)))
			{
				visibleBlocks.at(idx) = true;
				discoveredBlocks.insert(idx);
			}
		}
	}

	// Reveal all discovered blocks
	for (auto idx : discoveredBlocks)
	{
		auto l = battle.losBlocks.at(idx);
		for (int x = l->start.x; x < l->end.x; x++)
		{
			for (int y = l->start.y; y < l->end.y; y++)
			{
				for (int z = l->start.z; z < l->end.z; z++)
				{
					battle.setVisible(owner, x, y, z);
				}
			}
		}
	}
}

void BattleUnit::calculateVisionToUnits(GameState &state, Battle &battle, TileMap &map,
                                        Vec3<float> eyesPos)
{
	for (auto &entry : battle.units)
	{
		auto &u = *entry.second;
		// Unit unconscious, we own this unit or can't see it, skip
		if (!u.isConscious() || u.owner == owner || !isWithinVision(u.position))
		{
			continue;
		}
		// FIXME: This likely won't work properly for large units
		// Idea here is to LOS to the center of the occupied tile
		auto target = u.tileObject->getCenter();
		target.x = (int)target.x + 0.5f;
		target.y = (int)target.y + 0.5f;
		target.z = (int)target.z + 0.5f;
		auto c =
		    map.findCollision(eyesPos, target, mapPartSet, tileObject, true, false, VIEW_DISTANCE);
		if (c || c.outOfRange)
		{
			continue;
		}
		visibleUnits.emplace(&state, entry.first);
	}
}

void BattleUnit::refreshUnitVision(GameState &state)
{
	auto &battle = *state.current_battle;
	auto &map = *battle.map;
	auto lastVisibleUnits = visibleUnits;
	visibleUnits.clear();
	visibleEnemies.clear();

	// Vision is actually updated only if conscious, otherwise we clear visible units and that's it
	if (isConscious())
	{
		// FIXME: This likely won't work properly for large units
		// Idea here is to LOS from the center of the occupied tile
		auto eyesPos = Vec3<float>{
		    (int)position.x + 0.5f, (int)position.y + 0.5f,
		    (int)position.z +
		        ((float)agent->type->bodyType->muzzleZPosition.at(current_body_state)) / 40.0f};

		calculateVisionToTerrain(state, battle, map, eyesPos);
		calculateVisionToUnits(state, battle, map, eyesPos);
	}

	// Add newly visible units to owner's list and enemy list
	for (auto vu : visibleUnits)
	{
		// owner's visible units list
		if (lastVisibleUnits.find(vu) == lastVisibleUnits.end())
		{
			state.current_battle->visibleUnits[owner].insert(vu);
		}
		// units's visible enemies list
		if (owner->isRelatedTo(vu->owner) == Organisation::Relation::Hostile)
		{
			visibleEnemies.push_back(vu);
			state.current_battle->visibleEnemies[owner].insert(vu);
		}
	}

	// See if someone else sees a unit we stopped seeing
	for (auto lvu : lastVisibleUnits)
	{
		if (visibleUnits.find(lvu) == visibleUnits.end())
		{
			bool someoneElseSees = false;
			for (auto u : state.current_battle->units)
			{
				if (u.second->owner != owner)
				{
					continue;
				}
				if (u.second->visibleUnits.find(lvu) != u.second->visibleUnits.end())
				{
					someoneElseSees = true;
					break;
				}
			}
			if (!someoneElseSees)
			{
				state.current_battle->visibleUnits[owner].erase(lvu);
				state.current_battle->visibleEnemies[owner].erase(lvu);
			}
		}
	}
}

void BattleUnit::refreshUnitVisibilityAndVision(GameState &state, Vec3<float> oldPosition)
{
	refreshUnitVision(state);
	refreshUnitVisibility(state, oldPosition);
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

void BattleUnit::startAttacking(GameState &state, WeaponStatus status)
{
	switch (state.current_battle->mode)
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
	ticksUntillNextTargetCheck = 0;
}

void BattleUnit::startAttacking(GameState &state, StateRef<BattleUnit> unit, WeaponStatus status)
{
	startAttacking(state, status);
	targetUnit = unit;
	targetingMode = TargetingMode::Unit;
}

void BattleUnit::startAttacking(GameState &state, Vec3<int> tile, WeaponStatus status,
                                bool atGround)
{
	startAttacking(state, status);
	targetTile = tile;
	targetTile = tile;
	targetingMode = atGround ? TargetingMode::TileGround : TargetingMode::TileCenter;
}

void BattleUnit::stopAttacking()
{
	weaponStatus = WeaponStatus::NotFiring;
	targetingMode = TargetingMode::NoTarget;
	targetUnit.clear();
	ticksUntillNextTargetCheck = 0;
}
bool BattleUnit::canAfford(GameState &state, int cost) const
{
	if (state.current_battle->mode == Battle::Mode::RealTime)
	{
		return true;
	}
	return agent->modified_stats.time_units >= cost;
}

bool BattleUnit::spendTU(GameState &state, int cost)
{
	if (state.current_battle->mode == Battle::Mode::RealTime)
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
	return !falling && current_movement_state == MovementState::None;
}

bool BattleUnit::isBusy() const
{
	return !missions.empty() || !visibleEnemies.empty() || isAttacking();
}

bool BattleUnit::isAttacking() const { return weaponStatus != WeaponStatus::NotFiring; }

bool BattleUnit::isThrowing() const { return isDoing(BattleUnitMission::Type::ThrowItem); }

bool BattleUnit::isMoving() const { return isDoing(BattleUnitMission::Type::GotoLocation); }

bool BattleUnit::isDoing(BattleUnitMission::Type missionType) const
{
	bool found = false;
	for (auto &m : missions)
	{
		if (m->type == missionType)
		{
			found = true;
			break;
		}
	}
	return found;
}

BattleUnitType BattleUnit::getType() const
{
	if (isLarge())
	{
		if (canFly())
		{
			return BattleUnitType::LargeFlyer;
		}
		else
		{
			return BattleUnitType::LargeWalker;
		}
	}
	else
	{
		if (canFly())
		{
			return BattleUnitType::SmallFlyer;
		}
		else
		{
			return BattleUnitType::SmallWalker;
		}
	}
}

bool BattleUnit::isAIControlled(GameState &state) const
{
	return owner != state.current_battle->currentPlayer;
}

AIType BattleUnit::getAIType() const
{
	// FIXME: Check if unit's panicking or berserk
	return agent->type->aiType;
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

void BattleUnit::addFatalWound(BodyPart fatalWoundPart) { fatalWounds[fatalWoundPart]++; }

void BattleUnit::dealDamage(GameState &state, int damage, bool generateFatalWounds,
                            BodyPart fatalWoundPart, int stunPower)
{
	if (isDead())
	{
		return;
	}

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
			addFatalWound(fatalWoundPart);
			fatal = true;
		}
		if (randBoundsExclusive(state.rng, 0, 10) < woundDamageRemaining)
		{
			addFatalWound(fatalWoundPart);
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

	if (wasConscious)
	{
		// Emit sound fatal wound
		if (fatal)
		{
			if (agent->type->fatalWoundSfx.find(agent->gender) !=
			        agent->type->fatalWoundSfx.end() &&
			    !agent->type->fatalWoundSfx.at(agent->gender).empty())
			{
				fw().soundBackend->playSample(
				    listRandomiser(state.rng, agent->type->fatalWoundSfx.at(agent->gender)),
				    position);
			}
		}
		// Emit sound wound (unless dealing damage from a fatal wound)
		else if (stunPower == 0 && generateFatalWounds)
		{
			if (agent->type->damageSfx.find(agent->gender) != agent->type->damageSfx.end() &&
			    !agent->type->damageSfx.at(agent->gender).empty())
			{
				fw().soundBackend->playSample(
				    listRandomiser(state.rng, agent->type->damageSfx.at(agent->gender)), position);
			}
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
	dealDamage(state, damage, damageType->dealsFatalWounds(), bodyPart,
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
		attackerPosition = -projectile->getVelocity();

		return applyDamage(
		    state, projectile->damage, projectile->damageType,
		    determineBodyPartHit(projectile->damageType, c.position, projectile->getVelocity()));
	}
	return false;
}

void BattleUnit::updateStateAndStats(GameState &state, unsigned int ticks)
{
	// Ticks until auto turning and targeting
	if (ticksUntilAutoTurnAvailable > 0)
	{
		if (ticksUntilAutoTurnAvailable > ticks)
		{
			ticksUntilAutoTurnAvailable -= ticks;
		}
		else
		{
			ticksUntilAutoTurnAvailable = 0;
		}
	}
	if (ticksUntilAutoTargetAvailable > 0)
	{
		if (ticksUntilAutoTargetAvailable > ticks)
		{
			ticksUntilAutoTargetAvailable -= ticks;
		}
		else
		{
			ticksUntilAutoTargetAvailable = 0;
		}
	}

	// FIXME: Regenerate stamina

	// Stun removal
	if (stunDamageInTicks > 0)
	{
		stunDamageInTicks = std::max(0, stunDamageInTicks - (int)ticks);
	}

	// Ensure still have item if healing
	if (isHealing)
	{
		isHealing = false;
		auto e1 = agent->getFirstItemInSlot(AEquipmentSlotType::LeftHand);
		auto e2 = agent->getFirstItemInSlot(AEquipmentSlotType::RightHand);
		if (e1 && e1->type->type == AEquipmentType::Type::MediKit)
		{
			isHealing = true;
		}
		else if (e2 && e2->type->type == AEquipmentType::Type::MediKit)
		{
			isHealing = true;
		}
	}

	// Fatal wounds / healing
	if (isFatallyWounded() && !isDead())
	{
		bool unconscious = isUnconscious();
		woundTicksAccumulated += ticks;
		while (woundTicksAccumulated >= TICKS_PER_UNIT_EFFECT)
		{
			woundTicksAccumulated -= TICKS_PER_UNIT_EFFECT;
			for (auto &w : fatalWounds)
			{
				if (w.second > 0)
				{
					dealDamage(state, w.second, false, BodyPart::Body, 0);
					if (isHealing && healingBodyPart == w.first)
					{
						w.second--;
						// healing fatal wound heals 3hp, as well as 1hp we just dealt in damage
						agent->modified_stats.health += 4;
						agent->modified_stats.health =
						    std::min(agent->modified_stats.health, agent->current_stats.health);
					}
				}
			}
		}
		// If fully healed the body part
		if (isHealing && fatalWounds[healingBodyPart] == 0)
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

	// Turn off Jetpacks
	if (current_body_state != BodyState::Flying)
	{
		flyingSpeedModifier = 0;
	}
}

void BattleUnit::updateEvents(GameState &state)
{
	static const Vec3<int> NONE = {0, 0, 0};

	// Try giving way if asked to
	// FIXME: Ensure we're not in a firefight before giving way!
	if (giveWayRequestData.size() > 0)
	{
		if (!missions.empty() || !isConscious())
		{
			giveWayRequestData.clear();
		}
		else
		{
			// If we're given a giveWay request 0, 0 it means we're asked to kneel temporarily
			if (giveWayRequestData.size() == 1 && giveWayRequestData.front().x == 0 &&
			    giveWayRequestData.front().y == 0 &&
			    canAfford(state, BattleUnitMission::getBodyStateChangeCost(*this, target_body_state,
			                                                               BodyState::Kneeling)))
			{
				// Give way
				setMission(state, BattleUnitMission::changeStance(*this, BodyState::Kneeling));
				// Give time for that unit to pass
				addMission(state, BattleUnitMission::snooze(*this, TICKS_PER_SECOND), true);
			}
			else
			{
				auto from = tileObject->getOwningTile();
				for (auto newHeading : giveWayRequestData)
				{
					for (int z = -1; z <= 1; z++)
					{
						if (z < 0 || z >= tileObject->map.size.z)
						{
							continue;
						}
						// Try the new heading
						Vec3<int> pos = {position.x + newHeading.x, position.y + newHeading.y,
						                 position.z + z};
						auto to = tileObject->map.getTile(pos);
						// Check if heading on our level is acceptable
						bool acceptable =
						    BattleUnitTileHelper{tileObject->map, *this}.canEnterTile(from, to) &&
						    BattleUnitTileHelper{tileObject->map, *this}.canEnterTile(to, from);
						// If not, check if we can go down one tile
						if (!acceptable && pos.z - 1 >= 0)
						{
							pos -= Vec3<int>{0, 0, 1};
							to = tileObject->map.getTile(pos);
							acceptable =
							    BattleUnitTileHelper{tileObject->map, *this}.canEnterTile(from,
							                                                              to) &&
							    BattleUnitTileHelper{tileObject->map, *this}.canEnterTile(to, from);
						}
						// If not, check if we can go up one tile
						if (!acceptable && pos.z + 2 < tileObject->map.size.z)
						{
							pos += Vec3<int>{0, 0, 2};
							to = tileObject->map.getTile(pos);
							acceptable =
							    BattleUnitTileHelper{tileObject->map, *this}.canEnterTile(from,
							                                                              to) &&
							    BattleUnitTileHelper{tileObject->map, *this}.canEnterTile(to, from);
						}
						if (acceptable)
						{
							// 01: Give way (move 1 tile away)
							setMission(state, BattleUnitMission::gotoLocation(*this, pos, 0));
							// 02: Turn to previous facing
							addMission(state, BattleUnitMission::turn(*this, facing), true);
							// 03: Give time for that unit to pass
							addMission(state, BattleUnitMission::snooze(*this, 60), true);
							// 04: Return to our position after we're done
							addMission(state, BattleUnitMission::gotoLocation(*this, position, 0),
							           true);
							// 05: Turn to previous facing
							addMission(state, BattleUnitMission::turn(*this, facing), true);
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
			giveWayRequestData.clear();
		}
	}

	// Process being hit or under fire
	if (attackerPosition != NONE)
	{
		// Turn to attacker in real time if we're idle
		if (!isBusy() && isConscious() && state.current_battle->mode == Battle::Mode::RealTime &&
		    ticksUntilAutoTurnAvailable == 0)
		{
			setMission(state,
			           BattleUnitMission::turn(*this, position + (Vec3<float>)attackerPosition));
			ticksUntilAutoTurnAvailable = AUTO_TURN_COOLDOWN;
		}
		if (isAIControlled(state))
		{
			aiList.notifyUnderFire(attackerPosition);
		}
		attackerPosition = NONE;
	}

	// Process spotting an enemy
	if (!visibleEnemies.empty() && isAIControlled(state))
	{
		// our target has a priority over others if enemy
		auto lastSeenEnemyPosition =
		    (targetUnit &&
		             std::find(visibleEnemies.begin(), visibleEnemies.end(), targetUnit) !=
		                 visibleEnemies.end()
		         ? targetUnit->position
		         : visibleEnemies.front()->position) -
		    position;

		aiList.notifyEnemySpotted(lastSeenEnemyPosition);
	}
}

void BattleUnit::updateIdling(GameState &state)
{
	// Idling check #1
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
		else
		{
			// Kneel if not kneeling and should kneel
			if (kneeling_mode == KneelingMode::Kneeling &&
			    current_body_state != BodyState::Kneeling && canKneel() &&
			    canAfford(state, BattleUnitMission::getBodyStateChangeCost(*this, target_body_state,
			                                                               BodyState::Kneeling)))
			{
				setMission(state, BattleUnitMission::changeStance(*this, BodyState::Kneeling));
			}
			// Go prone if not prone and should stay prone
			else if (movement_mode == MovementMode::Prone &&
			         current_body_state != BodyState::Prone &&
			         kneeling_mode != KneelingMode::Kneeling && canProne(position, facing) &&
			         canAfford(state, BattleUnitMission::getBodyStateChangeCost(
			                              *this, target_body_state, BodyState::Prone)))
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
					if (canAfford(state, BattleUnitMission::getBodyStateChangeCost(
					                         *this, target_body_state, BodyState::Standing)))
					{
						setMission(state,
						           BattleUnitMission::changeStance(*this, BodyState::Standing));
					}
				}
				else
				{
					if (canAfford(state, BattleUnitMission::getBodyStateChangeCost(
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
			         canAfford(state, BattleUnitMission::getBodyStateChangeCost(
			                              *this, target_body_state, BodyState::Standing)))
			{
				setMission(state, BattleUnitMission::changeStance(*this, BodyState::Standing));
			}
			// Stop being prone if legs are no longer supported and we haven't taken a mission yet
			if (current_body_state == BodyState::Prone && missions.empty())
			{
				bool hasSupport = true;
				for (auto t : tileObject->occupiedTiles)
				{
					if (!tileObject->map.getTile(t)->getCanStand())
					{
						hasSupport = false;
						break;
					}
				}
				if (!hasSupport &&
				    canAfford(state, BattleUnitMission::getBodyStateChangeCost(
				                         *this, target_body_state, BodyState::Kneeling)))
				{
					setMission(state, BattleUnitMission::changeStance(*this, BodyState::Kneeling));
				}
			}
		}
	} // End of Idling Check #1
}

void BattleUnit::updateAcquireTarget(GameState &state, unsigned int ticks)
{
	// Still idling and not attacking? Attack or face enemy
	if (isConscious() && !isAttacking())
	{
		// See no enemies and have no mission - turn to focused or closest visible enemy
		if (visibleEnemies.empty() && missions.empty())
		{
			if (ticksUntilAutoTurnAvailable == 0)
			{
				auto &units = state.current_battle->visibleEnemies[owner];
				// Try focused unit
				StateRef<BattleUnit> closestEnemy = focusUnit;
				// If focused unit is unaccounted for, try for closest one
				if (units.find(closestEnemy) == units.end())
				{
					closestEnemy.clear();
					auto it = units.begin();
					float minDistance = FLT_MAX;
					while (it != units.end())
					{
						auto enemy = *it++;
						auto distance = glm::distance(enemy->position, position);
						if (distance < minDistance)
						{
							minDistance = distance;
							closestEnemy = enemy;
						}
					}
				}
				if (closestEnemy && glm::distance(closestEnemy->position, position) < VIEW_DISTANCE)
				{
					setMission(state, BattleUnitMission::turn(*this, closestEnemy->position));
					ticksUntilAutoTurnAvailable = AUTO_TURN_COOLDOWN;
				}
			}
		}
		// See enemy - turn or attack
		else if (!visibleEnemies.empty() &&
		         (missions.empty() || missions.front()->type != BattleUnitMission::Type::Snooze))
		{
			auto e1 = agent->getFirstItemInSlot(AEquipmentSlotType::RightHand);
			auto e2 = agent->getFirstItemInSlot(AEquipmentSlotType::LeftHand);
			// Cannot or forbidden to attack:	Turn to enemy
			if (fire_permission_mode == FirePermissionMode::CeaseFire ||
			    ((!e1 || !e1->canFire()) && (!e2 || !e2->canFire())))
			{
				if (ticksUntilAutoTurnAvailable == 0)
				{
					// Look at focused unit or find closest enemy
					auto targetEnemy = focusUnit;
					if (visibleUnits.find(targetEnemy) == visibleUnits.end())
					{
						auto it = visibleEnemies.begin();
						float minDistance = FLT_MAX;
						while (it != visibleEnemies.end())
						{
							auto enemy = *it++;
							auto distance = glm::distance(enemy->position, position);
							if (distance < minDistance)
							{
								minDistance = distance;
								targetEnemy = enemy;
							}
						}
					}
					setMission(state, BattleUnitMission::turn(*this, targetEnemy->position));
					ticksUntilAutoTurnAvailable = AUTO_TURN_COOLDOWN;
				}
			}
			// Can attack and allowed to:		Attack enemy
			else
			{
				if (ticksUntilAutoTargetAvailable == 0)
				{
					// Find enemy we can attack amongst those visible
					auto targetEnemy = focusUnit;
					// Ensure we can attack focus
					if (targetEnemy)
					{
						auto muzzleLocation = getMuzzleLocation();
						auto targetPosition = targetEnemy->tileObject->getVoxelCentrePosition();
						auto cMap = tileObject->map.findCollision(muzzleLocation, targetPosition,
						                                          mapPartSet, tileObject);
						auto cUnit = tileObject->map.findCollision(muzzleLocation, targetPosition,
						                                           unitSet, tileObject);
						if (cMap || (cUnit &&
						             owner->isRelatedTo(
						                 std::static_pointer_cast<TileObjectBattleUnit>(cUnit.obj)
						                     ->getUnit()
						                     ->owner) != Organisation::Relation::Hostile))
						{
							targetEnemy.clear();
						}
					}
					// If can't attack focus or have no focus - take closest attackable
					if (visibleUnits.find(targetEnemy) == visibleUnits.end())
					{
						targetEnemy.clear();
						// Make a list of enemies sorted by distance to them
						std::map<float, StateRef<BattleUnit>> enemiesByDistance;
						for (auto u : visibleEnemies)
						{
							// Ensure we add every unit
							auto distance = glm::distance(u->position, position);
							while (enemiesByDistance.find(distance) != enemiesByDistance.end())
							{
								distance += 0.01f;
							}
							enemiesByDistance[distance] = u;
						}
						// Pick enemy that is closest and can be attacked
						for (auto entry : enemiesByDistance)
						{
							auto muzzleLocation = getMuzzleLocation();
							auto targetPosition =
							    entry.second->tileObject->getVoxelCentrePosition();
							auto cMap = tileObject->map.findCollision(
							    muzzleLocation, targetPosition, mapPartSet, tileObject);
							auto cUnit = tileObject->map.findCollision(
							    muzzleLocation, targetPosition, unitSet, tileObject);
							if (!cMap &&
							    (!cUnit ||
							     owner->isRelatedTo(
							         std::static_pointer_cast<TileObjectBattleUnit>(cUnit.obj)
							             ->getUnit()
							             ->owner) == Organisation::Relation::Hostile))
							{
								targetEnemy = entry.second;
								break;
							}
						}
					}

					// Attack if we can
					if (targetEnemy)
					{
						startAttacking(state, targetEnemy);
					}
					else
					{
						ticksUntilAutoTargetAvailable = AUTO_TARGET_COOLDOWN;
					}
				}
			}
		}
	}
}

void BattleUnit::updateCheckIfFalling(GameState &state)
{
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
				if (tileObject->map.getTile(goalPosition)->getCanStand(isLarge()))
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
					setBodyState(state, BodyState::Flying);
					if (!missions.empty())
					{
						missions.front()->targetBodyState = current_body_state;
					}
				}
			}
		}
	}
}

void BattleUnit::updateBody(GameState &state, unsigned int &bodyTicksRemaining)
{
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
				setBodyState(state, target_body_state);
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
			    (hand_animation_ticks_remaining == 0 || target_hand_state == HandState::Aiming))
			{
				BodyState nextState = BodyState::Downed;
				if (getNextBodyState(state, nextState))
				{
					beginBodyStateChange(state, nextState);
				}
			}
		}
	}
}

void BattleUnit::updateMovement(GameState &state, unsigned int &moveTicksRemaining,
                                bool &wasUsingLift)
{
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
				newPosition -=
				    Vec3<float>{0.0f, 0.0f, (fallingSpeed / TICK_SCALE)} / VELOCITY_SCALE_BATTLE;
			}
			// Fell into a unit
			if (isConscious() &&
			    tileObject->map.getTile(newPosition)
			        ->getUnitIfPresent(true, true, false, tileObject))
			{
				// FIXME: Proper stun damage (ensure it is!)
				stunDamageInTicks = 0;
				dealDamage(state, agent->current_stats.health * 3 / 2, false, BodyPart::Body, 9001);
				fallUnconscious(state);
			}
			setPosition(state, newPosition);
			triggerProximity(state);

			// Falling units can always turn
			goalPosition = position;
			atGoal = true;

			// Check if reached ground
			auto restingPosition = tileObject->getOwningTile()->getRestingPosition(isLarge());
			if (position.z < restingPosition.z)
			{
				// Stopped falling
				falling = false;
				if (!isConscious())
				{
					// Bodies drop to the exact spot they fell upon
					setPosition(state, {position.x, position.y, restingPosition.z});
				}
				else
				{
					setPosition(state, restingPosition);
				}
				triggerProximity(state);
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
					fw().soundBackend->playSample(agent->type->gravLiftSfx, getPosition(), 0.25f);
				}
				usingLift = true;
				movement_ticks_passed = 0;
			}
			unsigned int movementTicksAccumulated = 0;
			if (distanceToGoal * moveTicksConsumeRate * 100 / speedModifier > moveTicksRemaining)
			{
				if (flyingSpeedModifier != 100)
				{
					flyingSpeedModifier =
					    std::min((unsigned)100, flyingSpeedModifier +
					                                moveTicksRemaining / moveTicksConsumeRate /
					                                    FLYING_ACCELERATION_DIVISOR);
				}
				movementTicksAccumulated = moveTicksRemaining / moveTicksConsumeRate;
				auto dir = glm::normalize(vectorToGoal);
				Vec3<float> newPosition = (float)(moveTicksRemaining / moveTicksConsumeRate) *
				                          (float)(speedModifier / 100) * dir;
				newPosition /= VELOCITY_SCALE_BATTLE;
				newPosition /= (float)TICKS_PER_UNIT_TRAVELLED;
				newPosition += getPosition();
				setPosition(state, newPosition);
				triggerProximity(state);
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
						flyingSpeedModifier = std::min(
						    (unsigned)100,
						    flyingSpeedModifier + distanceToGoal / FLYING_ACCELERATION_DIVISOR);
					}
					moveTicksRemaining -= distanceToGoal * moveTicksConsumeRate;
					setPosition(state, goalPosition);
					triggerProximity(state);
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
					startMoving();
					goalPosition = nextGoal;
					atGoal = false;
				}
				else if (!hasMovementQueued())
				{
					setMovementState(MovementState::None);
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
					    agent->type->walkSfx[getWalkSoundIndex() % agent->type->walkSfx.size()],
					    getPosition(), 0.25f);
				}
				else
				{
					auto t = tileObject->getOwningTile();
					if (t->walkSfx && t->walkSfx->size() > 0)
					{
						fw().soundBackend->playSample(
						    t->walkSfx->at(getWalkSoundIndex() % t->walkSfx->size()), getPosition(),
						    0.25f);
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
			if (atGoal)
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
					startMoving();
					goalPosition = nextGoal;
					atGoal = false;
				}
				else if (!hasMovementQueued())
				{
					setMovementState(MovementState::None);
				}
			}
		}
	}
}

void BattleUnit::updateHands(GameState &state, unsigned int &handsTicksRemaining)
{
	if (handsTicksRemaining > 0)
	{
		if (firing_animation_ticks_remaining > 0)
		{
			if (firing_animation_ticks_remaining > handsTicksRemaining)
			{
				firing_animation_ticks_remaining -= handsTicksRemaining;
				handsTicksRemaining = 0;
			}
			else
			{
				handsTicksRemaining -= firing_animation_ticks_remaining;
				firing_animation_ticks_remaining = 0;
				setHandState(HandState::Aiming);
			}
		}
		else
		{
			if (hand_animation_ticks_remaining > handsTicksRemaining)
			{
				hand_animation_ticks_remaining -= handsTicksRemaining;
				handsTicksRemaining = 0;
			}
			else
			{
				if (hand_animation_ticks_remaining > 0)
				{
					handsTicksRemaining -= hand_animation_ticks_remaining;
					hand_animation_ticks_remaining = 0;
					setHandState(target_hand_state);
				}
			}
		}
	}
}

void BattleUnit::updateTurning(GameState &state, unsigned int &turnTicksRemaining)
{
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
				setFacing(state, goalFacing);
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
				beginTurning(state, nextFacing);
			}
		}
	}
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

void BattleUnit::updateAttacking(GameState &state, unsigned int ticks)
{
	// Firing
	if (isAttacking())
	{
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
				targetPosition =
				    (Vec3<float>)targetTile + offsetTile + Vec3<float>{0.0f, 0.0f, unitZ};
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
		{
			// Do consequent checks if previous check is ok
			bool canFire = true;

			if (targetingMode == TargetingMode::Unit)
			{
				if (ticksUntillNextTargetCheck > ticks)
				{
					ticksUntillNextTargetCheck -= ticks;
				}
				else
				{
					ticksUntillNextTargetCheck = 0;
				}

				if (visibleUnits.find(targetUnit) == visibleUnits.end())
				{
					canFire = false;
				}
			}

			// We cannot fire if we have no weapon capable of firing
			canFire = canFire && (weaponLeft || weaponRight);

			// We cannot fire if it's time to check target unit and it's not in LOS anymore or not
			// conscious
			// Also, at this point we will turn to target tile if targeting tile
			if (canFire)
			{
				// Note: If not targeting a unit, this will only be done once after start,
				// and again once each time this unit stops moving
				if (ticksUntillNextTargetCheck == 0)
				{
					ticksUntillNextTargetCheck = LOS_CHECK_INTERVAL_TRACKING;
					if (targetingMode == TargetingMode::Unit)
					{
						auto cMap = tileObject->map.findCollision(muzzleLocation, targetPosition,
						                                          mapPartSet, tileObject);
						auto cUnit = tileObject->map.findCollision(muzzleLocation, targetPosition,
						                                           unitSet, tileObject);
						// Target is conscious, shot won't hit a map part, shot won't hit a
						// non-hostile unit
						canFire = canFire && targetUnit->isConscious() && !cMap &&
						          (!cUnit ||
						           owner->isRelatedTo(
						               std::static_pointer_cast<TileObjectBattleUnit>(cUnit.obj)
						                   ->getUnit()
						                   ->owner) == Organisation::Relation::Hostile);
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
						if (BattleUnitMission::getFacing(*this, targetTile) != facing)
						{
							addMission(state, BattleUnitMission::turn(*this, targetTile));
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

		// Still firing - process that
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
	}

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
}

void BattleUnit::updateAI(GameState &state, unsigned int ticks)
{
	static const Vec3<int> NONE = {0, 0, 0};

	if (!isConscious())
	{
		return;
	}
	// Check that unit is allowed to act now
	switch (state.current_battle->mode)
	{
		case Battle::Mode::TurnBased:
		{
			// FIXME: Implement TB AI properly, implement interrupts
			if (owner != state.current_battle->currentActiveOrganisation)
			{
				return;
			}
			break;
		}
		case Battle::Mode::RealTime:
		{
			// in RT the AI is always active?
			break;
		}
	}

	// Decide
	auto decision = aiList.think(state, *this);
	if (!decision.isEmpty())
	{
		LogWarning("AI %s for unit %s decided to %s", decision.ai, id, decision.getName());
		executeAIDecision(state, decision);
	}
}

void BattleUnit::update(GameState &state, unsigned int ticks)
{
	// Destroyed or retreated units do not exist in the battlescape
	if (destroyed || retreated)
	{
		return;
	}

	// Update Items
	for (auto item : agent->equipment)
		item->update(state, ticks);

	// Update Missions
	if (!this->missions.empty())
		this->missions.front()->update(state, *this, ticks);

	// Update the unit

	// Miscellaneous state updates, as well as unit's stats
	updateStateAndStats(state, ticks);
	// Unit events - was under fire, was requested to give way etc.
	updateEvents(state);
	// Idling #1: Auto-movement, auto-body change when idling
	updateIdling(state);
	// Acquire target
	updateAcquireTarget(state, ticks);
	// Main bulk - update movement, body, hands and turning
	{
		bool wasUsingLift = usingLift;
		usingLift = false;

		// If not running we will consume these twice as fast
		unsigned int moveTicksRemaining = ticks * agent->modified_stats.getActualSpeedValue() * 2;
		unsigned int bodyTicksRemaining = ticks;
		unsigned int handsTicksRemaining = ticks;
		unsigned int turnTicksRemaining = ticks;

		// Unconscious units cannot move their hands or turn, they can only animate body or fall
		if (!isConscious())
		{
			handsTicksRemaining = 0;
			turnTicksRemaining = 0;
		}

		unsigned int lastMoveTicksRemaining = 0;
		unsigned int lastBodyTicksRemaining = 0;
		unsigned int lastHandsTicksRemaining = 0;
		unsigned int lastTurnTicksRemaining = 0;

		while (lastMoveTicksRemaining != moveTicksRemaining ||
		       lastBodyTicksRemaining != bodyTicksRemaining ||
		       lastHandsTicksRemaining != handsTicksRemaining ||
		       lastTurnTicksRemaining != turnTicksRemaining)
		{
			lastMoveTicksRemaining = moveTicksRemaining;
			lastBodyTicksRemaining = bodyTicksRemaining;
			lastHandsTicksRemaining = handsTicksRemaining;
			lastTurnTicksRemaining = turnTicksRemaining;

			updateCheckIfFalling(state);
			updateBody(state, bodyTicksRemaining);
			updateHands(state, handsTicksRemaining);
			updateMovement(state, moveTicksRemaining, wasUsingLift);
			updateTurning(state, turnTicksRemaining);
			updateDisplayedItem();
		}
	}
	// Unit's attacking state
	updateAttacking(state, ticks);
	// AI
	updateAI(state, ticks);
}

void BattleUnit::triggerProximity(GameState &state)
{
	auto it = state.current_battle->items.begin();
	while (it != state.current_battle->items.end())
	{
		auto i = *it++;
		if (!i->item->primed || i->item->triggerDelay > 0)
		{
			continue;
		}
		// Proximity explosion trigger
		if ((i->item->triggerType == TriggerType::Proximity ||
		     i->item->triggerType == TriggerType::Boomeroid) &&
		    BattleUnitTileHelper::getDistanceStatic(position, i->position) <= i->item->triggerRange)
		{
			i->die(state);
		}
		// Boomeroid hopping trigger
		else if (i->item->triggerType == TriggerType::Boomeroid &&
		         BattleUnitTileHelper::getDistanceStatic(position, i->position) <= BOOMEROID_RANGE)
		{
			i->hopTo(state, position);
		}
	}
}

void BattleUnit::startFalling()
{
	setMovementState(MovementState::None);
	falling = true;
}

void BattleUnit::startMoving()
{
	// FIXME: Account for different movement ways (strafing, backwards etc.)
	if (movement_mode == MovementMode::Running && current_body_state != BodyState::Prone)
	{
		setMovementState(MovementState::Running);
	}
	else if (current_body_state != BodyState::Kneeling && current_body_state != BodyState::Throwing)
	{
		setMovementState(MovementState::Normal);
	}
	else
	{
		LogError("Trying to move while kneeling or throwing, wtf?");
	}
}

void BattleUnit::requestGiveWay(const BattleUnit &requestor,
                                const std::list<Vec3<int>> &plannedPath, Vec3<int> pos)
{
	// If asked already or busy - cannot give way
	if (!giveWayRequestData.empty() || isBusy())
	{
		return;
	}
	// If unit is prone and we're trying to go into it's legs
	if (current_body_state == BodyState::Prone && tileObject->getOwningTile()->position != pos)
	{
		// Just ask unit to kneel for a moment
		giveWayRequestData.emplace_back(0, 0);
	}
	// If unit is not prone or we're trying to go into it's body
	else
	{
		static const std::map<Vec2<int>, int> facing_dir_map = {
		    {{0, -1}, 0}, {{1, -1}, 1}, {{1, 0}, 2},  {{1, 1}, 3},
		    {{0, 1}, 4},  {{-1, 1}, 5}, {{-1, 0}, 6}, {{-1, -1}, 7}};
		static const std::map<int, Vec2<int>> dir_facing_map = {
		    {0, {0, -1}}, {1, {1, -1}}, {2, {1, 0}},  {3, {1, 1}},
		    {4, {0, 1}},  {5, {-1, 1}}, {6, {-1, 0}}, {7, {-1, -1}}};

		// Start with unit's facing, and go to the sides, adding facings
		// if they're not in our path and not our current position.
		// Next facings: [0] is clockwise, [1] is counter-clockwise from current
		std::vector<int> nextFacings = {facing_dir_map.at(facing), facing_dir_map.at(facing)};
		for (int i = 0; i <= 4; i++)
		{
			int limit = i == 0 || i == 4 ? 0 : 1;
			for (int j = 0; j <= limit; j++)
			{
				auto nextFacing = dir_facing_map.at(nextFacings[j]);
				Vec3<int> nextPos = {position.x + nextFacing.x, position.y + nextFacing.y,
				                     position.z};
				if (nextPos == (Vec3<int>)requestor.position ||
				    std::find(plannedPath.begin(), plannedPath.end(), nextPos) != plannedPath.end())
				{
					continue;
				}
				giveWayRequestData.push_back(nextFacing);
			}
			nextFacings[0] = nextFacings[0] == 7 ? 0 : nextFacings[0] + 1;
			nextFacings[1] = nextFacings[1] == 0 ? 7 : nextFacings[1] - 1;
		}
	}
}

void BattleUnit::executeGroupAIDecision(GameState &state, AIDecision &decision, std::list<StateRef<BattleUnit>> &units)
{
	if (decision.action)
	{
		for (auto u : units)
		{
			u->executeAIAction(state, *decision.action);
		}
	}
	if (decision.movement)
	{
		switch (decision.movement->type)
		{
			case AIMovement::Type::Patrol:
				Battle::groupMove(state, units,
					decision.movement->targetLocation, true);
				break;
			default:
				for (auto u : units)
				{
					u->executeAIMovement(state, *decision.movement);
				}
				break;
		}
	}
}

void BattleUnit::executeAIDecision(GameState &state, AIDecision &decision)
{
	if (decision.action)
	{
		executeAIAction(state, *decision.action);
	}
	if (decision.movement)
	{
		executeAIMovement(state, *decision.movement);
	}
}

void BattleUnit::executeAIAction(GameState &state, AIAction &action)
{
	switch (action.type)
	{
	case AIAction::Type::AttackGrenade:
		LogWarning("Implement acting on a Grenade action");
		// Throw grenade
		break;
	case AIAction::Type::AttackWeapon:
		LogWarning("Implement acting on a Weapon action");
		// Attack with weapon
		break;
	case AIAction::Type::AttackPsiMC:
	case AIAction::Type::AttackPsiStun:
	case AIAction::Type::AttackPsiPanic:
		LogWarning("Implement acting on a Psi AI action");
		break;
	}
}

void BattleUnit::executeAIMovement(GameState &state, AIMovement &movement)
{
	//FIXME: USE teleporter to move?
	// Or maybe this is done in AI?

	/*
	// Find out if we can use teleporter to move
	bool useTeleporter = false;
	switch (movement.type)
	{
		case AIMovement::Type::Pursue:
		case AIMovement::Type::Patrol:
		case AIMovement::Type::Advance:
		case AIMovement::Type::GetInRange:
		case AIMovement::Type::TakeCover:
			for (auto &e : agent->equipment)
			{
				if (e->type->type == AEquipmentType::Type::Teleporter)
				{
					if (e->type->type == AEquipmentType::Type::Teleporter &&
						e->ammo == e->type->max_ammo &&
						canAfford(state, agent->current_stats.time_units * 55 / 100))
					{
						useTeleporter = true;
						break;
					}
				}
			}
			break;
		default:
			break;
	}
	*/

	// Do movement
	switch (movement.type)
	{
		case AIMovement::Type::Stop:
			for (auto &m : missions)
			{
				if (m->type == BattleUnitMission::Type::GotoLocation)
				{
					m->targetLocation = goalPosition;
					m->currentPlannedPath.clear();
				}
			}
			break;
		case AIMovement::Type::Patrol:
			setMission(
				state, BattleUnitMission::gotoLocation(
					*this, movement.targetLocation));
			break;
		case AIMovement::Type::Retreat:
			setMission(
				state, BattleUnitMission::gotoLocation(
					*this, movement.targetLocation, 0, true, true, 1, true));
			break;
		case AIMovement::Type::Turn:
			setMission(state,
				BattleUnitMission::turn(*this, movement.targetLocation));
			break;
		case AIMovement::Type::Advance:
		case AIMovement::Type::GetInRange:
		case AIMovement::Type::TakeCover:
			LogWarning("Implement Advance/GetInRagnge/TakeCover AI movement's execution");
			break;
	}
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
	aiList.reset(state, *this);
	addMission(state, BattleUnitMission::changeStance(*this, targetState));
}

void BattleUnit::dropDown(GameState &state)
{
	aiList.reset(state, *this);
	resetGoal();
	setMovementState(MovementState::None);
	setHandState(HandState::AtEase);
	setBodyState(state, target_body_state);
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
					setBodyState(state, BodyState::Standing);
					continue;
				}
			// Intentional fall-through
			case BodyState::Standing:
				if (agent->isBodyStateAllowed(BodyState::Kneeling))
				{
					setBodyState(state, BodyState::Kneeling);
					continue;
				}
			// Intentional fall-through
			case BodyState::Kneeling:
				setBodyState(state, BodyState::Prone);
				continue;
			case BodyState::Prone:
			case BodyState::Downed:
				LogError("Not possible to reach this?");
				break;
		}
		break;
	}
	// Drop all gear
	if (agent->type->inventory)
	{
		while (!agent->equipment.empty())
		{
			addMission(state, BattleUnitMission::dropItem(*this, agent->equipment.front()));
		}
	}
	// Drop gear used by missions
	std::list<sp<AEquipment>> itemsToDrop;
	for (auto &m : missions)
	{
		if (m->item && m->item->ownerAgent)
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

	// Remove from list of visible units
	StateRef<BattleUnit> srThis = {&state, id};
	for (auto units : state.current_battle->visibleUnits)
	{
		if (units.first != owner)
		{
			units.second.erase(srThis);
		}
	}
	for (auto units : state.current_battle->visibleEnemies)
	{
		if (units.first != owner)
		{
			units.second.erase(srThis);
		}
	}
	for (auto unit : state.current_battle->units)
	{
		if (unit.second->owner != owner)
		{
			unit.second->visibleUnits.erase(srThis);
			unit.second->visibleEnemies.remove(srThis);
		}
	}
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
	std::ignore = bledToDeath;
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

void BattleUnit::beginBodyStateChange(GameState &state, BodyState bodyState)
{
	// Cease hand animation immediately
	if (hand_animation_ticks_remaining != 0)
		setHandState(target_hand_state);

	// Find which animation is possible
	int frameCount = agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
	                                                              bodyState, current_hand_state,
	                                                              current_movement_state, facing);
	// No such animation
	// Try stopping movement
	if (frameCount == 0 && current_movement_state != MovementState::None)
	{
		frameCount = agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
		                                                          bodyState, current_hand_state,
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
		                                                          bodyState, HandState::AtEase,
		                                                          current_movement_state, facing);
		if (frameCount != 0)
		{
			setHandState(HandState::AtEase);
		}
	}

	int ticks = frameCount * TICKS_PER_FRAME_UNIT;
	if (ticks > 0 && current_body_state != bodyState)
	{
		target_body_state = bodyState;
		body_animation_ticks_remaining = ticks;
		// Updates bounds etc.
		if (tileObject)
		{
			setPosition(state, position);
		}
	}
	else
	{
		setBodyState(state, bodyState);
	}
}

unsigned int BattleUnit::getBodyAnimationFrame() const
{
	return (body_animation_ticks_remaining + TICKS_PER_FRAME_UNIT - 1) / TICKS_PER_FRAME_UNIT;
}

void BattleUnit::setBodyState(GameState &state, BodyState bodyState)
{
	bool roseUp = current_body_state == BodyState::Downed;
	current_body_state = bodyState;
	target_body_state = bodyState;
	body_animation_ticks_remaining = 0;
	if (tileObject)
	{
		// Updates bounds etc.
		setPosition(state, position);
		// Update vision since our head position may have changed
		refreshUnitVision(state);
		// If rose up - update vision for units that see this
		if (roseUp)
		{
			// FIXME: Do this properly? Only update vision to this unit, not to everything?
			state.current_battle->queueVisionRefresh(position);
		}
	}
}

unsigned int BattleUnit::getHandAnimationFrame() const
{
	return ((firing_animation_ticks_remaining > 0 ? firing_animation_ticks_remaining
	                                              : hand_animation_ticks_remaining) +
	        TICKS_PER_FRAME_UNIT - 1) /
	       TICKS_PER_FRAME_UNIT;
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

void BattleUnit::beginTurning(GameState &, Vec2<int> newFacing)
{
	goalFacing = newFacing;
	turning_animation_ticks_remaining = TICKS_PER_FRAME_UNIT;
}

void BattleUnit::setFacing(GameState &state, Vec2<int> newFacing)
{
	facing = newFacing;
	goalFacing = newFacing;
	turning_animation_ticks_remaining = 0;
	refreshUnitVision(state);
}

void BattleUnit::setMovementState(MovementState state)
{
	if (current_movement_state == state)
	{
		return;
	}
	current_movement_state = state;
	switch (state)
	{
		case MovementState::None:
			movement_ticks_passed = 0;
			movement_sounds_played = 0;
			ticksUntillNextTargetCheck = 0;
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

unsigned int BattleUnit::getDistanceTravelled() const
{
	return movement_ticks_passed / TICKS_PER_UNIT_TRAVELLED;
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
		LogWarning("Unit %s mission \"%s\" finished", id, missions.front()->getName());
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

bool BattleUnit::hasMovementQueued()
{
	for (auto &m : missions)
	{
		if (m->type == BattleUnitMission::Type::GotoLocation)
		{
			return true;
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
		case BattleUnitMission::Type::DropItem:
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
	if (popFinishedMissions(state))
	{
		// Unit retreated
		return false;
	}
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
		// Drop Item can only be cancelled if item is in agent's inventory
		case BattleUnitMission::Type::DropItem:
			if (missions.front()->item && !missions.front()->item->ownerAgent)
			{
				return false;
			}
			break;
		// Missions that must be let finish (unless forcing)
		case BattleUnitMission::Type::ChangeBodyState:
		case BattleUnitMission::Type::Turn:
		case BattleUnitMission::Type::GotoLocation:
		case BattleUnitMission::Type::ReachGoal:
			letFinish = true;
			break;
		// Missions that can be cancelled
		case BattleUnitMission::Type::Snooze:
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
	while (missions.size() > (letFinish ? 1 : 0))
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
		case BattleUnitMission::Type::DropItem:
		case BattleUnitMission::Type::ThrowItem:
			if (!agent->type->inventory)
			{
				delete mission;
				return false;
			}
			break;
		default:
			// Nothing to check for in other missions
			break;
	}

	if (!cancelMissions(state))
	{
		delete mission;
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
				bool USER_OPTION_ALLOW_INSTANT_THROWS = true;
				if (USER_OPTION_ALLOW_INSTANT_THROWS &&
				    canAfford(state, BattleUnitMission::getThrowCost(*this)))
				{
					setMovementState(MovementState::None);
					setBodyState(state, BodyState::Standing);
					setFacing(state, BattleUnitMission::getFacing(*this, mission->targetLocation));
					missions.clear();
				}
				break;
			}
			// Turning can be cancelled if our mission will require us to turn in a different dir
			// Also reachGoal can be cancelled by GotoLocation
			case BattleUnitMission::Type::Turn:
			case BattleUnitMission::Type::GotoLocation:
			case BattleUnitMission::Type::ReachGoal:
			{
				if (missions.front()->type == BattleUnitMission::Type::ReachGoal &&
				    mission->type == BattleUnitMission::Type::GotoLocation)
				{
					missions.clear();
				}
				else if (facing != goalFacing)
				{
					Vec2<int> nextFacing;
					bool haveNextFacing = true;
					switch (mission->type)
					{
						case BattleUnitMission::Type::Turn:
							nextFacing =
							    BattleUnitMission::getFacingStep(*this, mission->targetFacing);
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
					// If we are turning towards something that will not be our next facing when we
					// try
					// to execute our mission then we're better off canceling it
					if (haveNextFacing && nextFacing != goalFacing)
					{
						setFacing(state, facing);
						missions.clear();
					}
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

bool BattleUnit::addMission(GameState &state, BattleUnitMission *mission, bool toBack)
{
	if (!agent->type->inventory)
	{
		if (mission->type == BattleUnitMission::Type::DropItem ||
		    mission->type == BattleUnitMission::Type::ThrowItem)
		{
			delete mission;
			return false;
		}
	}
	if (toBack)
	{
		missions.emplace_back(mission);
		return true;
	}

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
						delete mission;
						return false;
					// Missions that can be overwritten
					case BattleUnitMission::Type::AcquireTU:
						break;
				}
			}
			missions.emplace_front(mission);
			mission->start(state, *this);
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
			if (missions.size() == 1 || mission->type == BattleUnitMission::Type::Teleport)
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
			mission->start(state, *this);
			break;
	}
	return true;
}
}

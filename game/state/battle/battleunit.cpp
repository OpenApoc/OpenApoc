#define _USE_MATH_DEFINES
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
				auto rhItem =
				    agent->getFirstItemInSlot(AgentEquipmentLayout::EquipmentSlotType::RightHand);
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

void BattleUnit::dealStunDamage(int damage)
{
	// FIXME: Figure out stun damage scale
	int SCALE = TICKS_PER_SECOND;
	stunDamageInTicks += damage * SCALE;
}

bool BattleUnit::isDead() const { return getHealth() == 0 || destroyed; }

bool BattleUnit::isUnconscious() const { return !isDead() && getStunDamage() >= getHealth(); }

bool BattleUnit::isConscious() const
{
	return !isDead() && getStunDamage() < getHealth() &&
	       (current_body_state != AgentType::BodyState::Downed ||
	        target_body_state != AgentType::BodyState::Downed);
}

bool BattleUnit::isStatic() const
{
	return current_movement_state == AgentType::MovementState::None && !falling &&
	       current_body_state == target_body_state;
}

bool BattleUnit::isBusy() const { return !isStatic() || isAttacking(); }

bool BattleUnit::isAttacking() const { return weaponStatus != WeaponStatus::NotFiring; }
bool BattleUnit::isThrowing() const
{
	bool throwing = false;
	for (auto &m : missions)
	{
		if (m->type == BattleUnitMission::MissionType::ThrowItem)
		{
			throwing = true;
			break;
		}
	}
	return throwing;
}

bool BattleUnit::canFly() const
{
	return isConscious() && agent->isBodyStateAllowed(AgentType::BodyState::Flying);
}

bool BattleUnit::canMove() const
{
	if (!isConscious())
	{
		return false;
	}
	if (agent->isMovementStateAllowed(AgentType::MovementState::Normal) ||
	    agent->isMovementStateAllowed(AgentType::MovementState::Running))
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
	if (!agent->isBodyStateAllowed(AgentType::BodyState::Prone) ||
	    !tileObject->getOwningTile()->getCanStand())
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
		if (legsTile->canStand && std::abs(legsTile->height - bodyTile->height) <= 0.25f &&
		    legsTile->getPassable(false,
		                          agent->type->bodyType->height.at(AgentType::BodyState::Prone)) &&
		    (legsPos == (Vec3<int>)position || !legsTile->getUnitIfPresent(true, true)))
		{
			return true;
		}
	}
	return false;
}

bool BattleUnit::canKneel() const
{
	if (!agent->isBodyStateAllowed(AgentType::BodyState::Kneeling) ||
	    !tileObject->getOwningTile()->getCanStand(isLarge()))
		return false;
	return true;
}

// FIXME: Apply damage to the unit
void BattleUnit::applyDamage(GameState &state, int damage, StateRef<DamageType> damageType,
                             AgentType::BodyPart bodyPart)
{
	std::ignore = state;

	// FIXME: Process Disruptor shields!
	LogWarning("Unit taking damage! Implement disruptor shields!");

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
	damage = damageType->dealDamage(damage, damageModifier) - armorValue;

	// No daamge
	if (damage <= 0)
	{
		return;
	}

	// FIXME: armor damage only by non-stun
	LogWarning("Not all damage damages armor!");

	// Armor damage
	int armorDamage = damage / 10 + 1;
	armor->ammo -= armorDamage;
	if (armor->ammo <= 0)
	{
		// Armor destroyed
		agent->removeEquipment(armor);
	}

	// Health damage
	LogWarning("Implement health/stun damage properly!");
	agent->modified_stats.health -= damage;

	// dir = glm::round(dir);
	// auto armourDirection = VehicleType::ArmourDirection::Right;
	// if (dir.x == 0 && dir.y == 0 && dir.z == 0)
	//{
	//	armourDirection = VehicleType::ArmourDirection::Front;
	//}
	// else if (dir * 0.5f == vehicleDir)
	//{
	//	armourDirection = VehicleType::ArmourDirection::Rear;
	//}
	//// FIXME: vehicle Z != 0
	// else if (dir.z < 0)
	//{
	//	armourDirection = VehicleType::ArmourDirection::Top;
	//}
	// else if (dir.z > 0)
	//{
	//	armourDirection = VehicleType::ArmourDirection::Bottom;
	//}
	// else if ((vehicleDir.x == 0 && dir.x != dir.y) || (vehicleDir.y == 0 && dir.x == dir.y))
	//{
	//	armourDirection = VehicleType::ArmourDirection::Left;
	//}

	// float armourValue = 0.0f;
	// auto armour = this->type->armour.find(armourDirection);
	// if (armour != this->type->armour.end())
	//{
	//	armourValue = armour->second;
	//}

	// if (applyDamage(state, projectile->damage, armourValue))
	//{
	//	auto doodad = city->placeDoodad(StateRef<DoodadType>{&state, "DOODAD_3_EXPLOSION"},
	//		this->tileObject->getPosition());

	//	this->shadowObject->removeFromMap();
	//	this->tileObject->removeFromMap();
	//	this->shadowObject.reset();
	//	this->tileObject.reset();
	//	state.vehicles.erase(this->getId(state, this->shared_from_this()));
	//	return;
	//}

	// if (this->shield <= damage)
	//{
	//	if (this->shield > 0)
	//	{
	//		damage -= this->shield;
	//		this->shield = 0;

	//		// destroy the shield modules
	//		for (auto it = this->equipment.begin(); it != this->equipment.end();)
	//		{
	//			if ((*it)->type->type == VEquipmentType::Type::General &&
	//				(*it)->type->shielding > 0)
	//			{
	//				it = this->equipment.erase(it);
	//			}
	//			else
	//			{
	//				++it;
	//			}
	//		}
	//	}

	//	damage -= armour;
	//	if (damage > 0)
	//	{
	//		this->health -= damage;
	//		if (this->health <= 0)
	//		{
	//			this->health = 0;
	//			return true;
	//		}
	//		else if (isCrashed())
	//		{
	//			this->missions.clear();
	//			this->missions.emplace_back(VehicleMission::crashLand(*this));
	//			this->missions.front()->start(state, *this);
	//			return false;
	//		}
	//	}
	//}
	// else
	//{
	//	this->shield -= damage;
	//}
	return;
}

// FIXME: Handle unit's collision with projectile
// SHIELD = DOODAD_27_SHIELD
void BattleUnit::handleCollision(GameState &state, Collision &c)
{
	std::ignore = state;

	if (!this->tileObject)
	{
		LogError("It's possible multiple projectiles hit the same tile in the same tick (?)");
		return;
	}

	auto projectile = c.projectile.get();
	if (projectile)
	{
		// Calculate damage
		int damage;
		bool USER_OPTION_UFO_DAMAGE_MODEL = false;
		if (USER_OPTION_UFO_DAMAGE_MODEL)
		{
			damage = randDamage000200(state.rng, projectile->damage);
		}
		else
		{
			damage = randDamage050150(state.rng, projectile->damage);
		}

		// Determine body part hit
		AgentType::BodyPart bodyPartHit = AgentType::BodyPart::Body;
		// FIXME: Ensure body part determination is correct
		// Assume top 25% is head, lower 25% is legs, and middle 50% is body/left/right
		float altitude = (c.position.z - position.z) * 40.0f / (float)getCurrentHeight();
		if (altitude > 0.75f)
		{
			bodyPartHit = AgentType::BodyPart::Helmet;
		}
		else if (altitude < 0.25f)
		{
			bodyPartHit = AgentType::BodyPart::Legs;
		}
		else
		{
			auto unitDir = glm::normalize(Vec3<float>{facing.x, facing.y, 0.0f});
			auto projectileDir = glm::normalize(
			    Vec3<float>{projectile->getVelocity().x, projectile->getVelocity().y, 0.0f});
			auto cross = glm::cross(unitDir, projectileDir);
			int angle = (int)((cross.z >= 0 ? -1 : 1) * glm::angle(unitDir, -projectileDir) / M_PI *
			                  180.0f);
			if (angle > 45 && angle < 135)
			{
				bodyPartHit = AgentType::BodyPart::RightArm;
			}
			else if (angle < -45 && angle > -135)
			{
				bodyPartHit = AgentType::BodyPart::LeftArm;
			}
		}

		applyDamage(state, damage, projectile->damageType, bodyPartHit);
	}
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
		item->update(ticks);

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
		// FIXME: Get a proper fatal wound scale
		int TICKS_PER_FATAL_WOUND_DAMAGE = TICKS_PER_SECOND;
		woundTicksAccumulated += ticks;
		while (woundTicksAccumulated > TICKS_PER_FATAL_WOUND_DAMAGE)
		{
			woundTicksAccumulated -= TICKS_PER_FATAL_WOUND_DAMAGE;
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
			die(state, true);
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
		if (falling)
		{
			LogError("Unit falling without a mission, wtf?");
		}
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
			addMission(state, BattleUnitMission::MissionType::ReachGoal);
		}

		// Try giving way if asked to
		// FIXME: Ensure we're not in a firefight before giving way!
		else if (giveWayRequest.size() > 0)
		{
			// If we're given a giveWay request 0, 0 it means we're asked to kneel temporarily
			if (giveWayRequest.size() == 1 && giveWayRequest.front().x == 0 &&
			    giveWayRequest.front().y == 0 &&
			    canAfford(BattleUnitMission::getBodyStateChangeCost(
			        target_body_state, AgentType::BodyState::Kneeling)))
			{
				// Give time for that unit to pass
				addMission(state, BattleUnitMission::snooze(*this, TICKS_PER_SECOND), false);
				// Give way
				addMission(state,
				           BattleUnitMission::changeStance(*this, AgentType::BodyState::Kneeling));
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
							           BattleUnitMission::gotoLocation(*this, position, 0, false),
							           false);
							// 03: Give time for that unit to pass
							addMission(state, BattleUnitMission::snooze(*this, 60), false);
							// 02: Turn to previous facing
							addMission(state, BattleUnitMission::turn(*this, facing), false);
							// 01: Give way (move 1 tile away)
							addMission(state,
							           BattleUnitMission::gotoLocation(*this, pos, 0, false));
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
			setMovementState(AgentType::MovementState::None);
			// Kneel if not kneeling and should kneel
			if (kneeling_mode == KneelingMode::Kneeling &&
			    current_body_state != AgentType::BodyState::Kneeling && canKneel() &&
			    canAfford(BattleUnitMission::getBodyStateChangeCost(
			        target_body_state, AgentType::BodyState::Kneeling)))
			{
				addMission(state,
				           BattleUnitMission::changeStance(*this, AgentType::BodyState::Kneeling));
			}
			// Go prone if not prone and should stay prone
			else if (movement_mode == BattleUnit::MovementMode::Prone &&
			         current_body_state != AgentType::BodyState::Prone &&
			         kneeling_mode != KneelingMode::Kneeling && canProne(position, facing) &&
			         canAfford(BattleUnitMission::getBodyStateChangeCost(
			             target_body_state, AgentType::BodyState::Prone)))
			{
				addMission(state,
				           BattleUnitMission::changeStance(*this, AgentType::BodyState::Prone));
			}
			// Stand up if not standing up and should stand up
			else if ((movement_mode == BattleUnit::MovementMode::Walking ||
			          movement_mode == BattleUnit::MovementMode::Running) &&
			         kneeling_mode != KneelingMode::Kneeling &&
			         current_body_state != AgentType::BodyState::Standing &&
			         current_body_state != AgentType::BodyState::Flying)
			{
				if (agent->isBodyStateAllowed(AgentType::BodyState::Standing))
				{
					if (canAfford(BattleUnitMission::getBodyStateChangeCost(
					        target_body_state, AgentType::BodyState::Standing)))
					{
						addMission(state, BattleUnitMission::changeStance(
						                      *this, AgentType::BodyState::Standing));
					}
				}
				else
				{
					if (canAfford(BattleUnitMission::getBodyStateChangeCost(
					        target_body_state, AgentType::BodyState::Flying)))
					{
						addMission(state, BattleUnitMission::changeStance(
						                      *this, AgentType::BodyState::Flying));
					}
				}
			}
			// Stop flying if we can stand
			else if (current_body_state == AgentType::BodyState::Flying &&
			         tileObject->getOwningTile()->getCanStand(isLarge()) &&
			         agent->isBodyStateAllowed(AgentType::BodyState::Standing) &&
			         canAfford(BattleUnitMission::getBodyStateChangeCost(
			             target_body_state, AgentType::BodyState::Standing)))
			{
				addMission(state,
				           BattleUnitMission::changeStance(*this, AgentType::BodyState::Standing));
			}
			// Stop being prone if legs are no longer supported and we haven't taken a mission yet
			if (current_body_state == AgentType::BodyState::Prone && missions.empty())
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
				                       target_body_state, AgentType::BodyState::Kneeling)))
				{
					addMission(state, BattleUnitMission::changeStance(
					                      *this, AgentType::BodyState::Kneeling));
				}
			}
		}
	} // End of Idling

	// Movement and Body Animation
	{
		bool wasUsingLift = usingLift;
		usingLift = false;

		// Turn off Jetpacks
		if (current_body_state != AgentType::BodyState::Flying)
		{
			flyingSpeedModifier = 0;
		}

		// If not running we will consume these twice as fast
		unsigned int moveTicksRemaining = ticks * agent->modified_stats.getActualSpeedValue() * 2;
		unsigned int bodyTicksRemaining = ticks;
		unsigned int handTicksRemaining = ticks;
		unsigned int turnTicksRemaining = ticks;

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

			// STEP 01: Begin falling or changing stance to flying if appropriate
			if (!falling)
			{
				// Check if should fall or start flying
				if (!canFly() || current_body_state != AgentType::BodyState::Flying)
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
						addMission(state, BattleUnitMission::MissionType::Fall);
					}
					// If flying and not supported both on current and goal locations - start flying
					if (!fullySupported && canFly())
					{
						if (current_body_state == target_body_state)
						{
							setBodyState(AgentType::BodyState::Flying);
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
					     target_hand_state == AgentType::HandState::Aiming))
					{
						AgentType::BodyState nextState = AgentType::BodyState::Downed;
						if (getNextBodyState(state, nextState))
						{
							LogWarning("%d %d", firing_animation_ticks_remaining,
							           hand_animation_ticks_remaining);
							beginBodyStateChange(nextState);
						}
					}
				}
			}

			// Try changing hand state
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
						setHandState(AgentType::HandState::Aiming);
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
						dealStunDamage(agent->current_stats.health * 3 / 2);
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

				// Not falling and moving
				else if (current_movement_state != AgentType::MovementState::None)
				{
					unsigned int speedModifier = 100;
					if (current_body_state == AgentType::BodyState::Flying)
					{
						speedModifier = std::max((unsigned)1, flyingSpeedModifier);
					}

					Vec3<float> vectorToGoal = goalPosition - getPosition();
					unsigned int distanceToGoal = (unsigned)ceilf(glm::length(
					    vectorToGoal * VELOCITY_SCALE_BATTLE * (float)TICKS_PER_UNIT_TRAVELLED));
					unsigned int moveTicksConsumeRate =
					    current_movement_state == AgentType::MovementState::Running ? 1 : 2;

					// Quick check, if moving strictly vertical then using lift
					if (distanceToGoal > 0 && current_body_state != AgentType::BodyState::Flying &&
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
					if (shouldPlaySoundNow() && current_body_state != AgentType::BodyState::Flying)
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
				// Not falling and not moving
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
						addMission(state, BattleUnitMission::MissionType::ReachGoal);
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
						turning_animation_ticks_remaining = 0;
						facing = goalFacing;
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
	Vec3<float> targetPosition;
	switch (targetingMode)
	{
		case TargetingMode::Unit:
			targetPosition = targetUnit->tileObject->getVoxelCentrePosition();
			break;
		case TargetingMode::TileCenter:
		{
			float unitZ = (position + Vec3<float>{0.0f, 0.0f, (float)getCurrentHeight() / 40.0f}).z;
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

	auto weaponRight =
	    agent->getFirstItemInSlot(AgentEquipmentLayout::EquipmentSlotType::RightHand);
	auto weaponLeft = agent->getFirstItemInSlot(AgentEquipmentLayout::EquipmentSlotType::LeftHand);
	switch (weaponStatus)
	{
		case WeaponStatus::FiringBothHands:
			if (weaponRight && !weaponRight->canFire())
			{
				weaponRight = nullptr;
			}
			if (weaponLeft && !weaponLeft->canFire())
			{
				weaponLeft = nullptr;
			}
			break;
		case WeaponStatus::FiringRightHand:
			if (weaponRight && !weaponRight->canFire())
			{
				weaponRight = nullptr;
			}
			weaponLeft = nullptr;
			break;
		case WeaponStatus::FiringLeftHand:
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
					LogWarning("Implement checking LOS to targetUnit");
					canFire = canFire && true; // Here we check if target is visible
					if (canFire)
					{
						targetTile = targetUnit->position;
					}
				}
				// Check if we are in range
				if (canFire)
				{
					float distanceToTarget = glm::length(
					    position + Vec3<float>{0.0f, 0.0f, (float)getCurrentHeight() / 40.0f} -
					    targetPosition);
					if (weaponRight && !weaponRight->canFire(distanceToTarget))
					{
						weaponRight = nullptr;
					}
					if (weaponLeft && !weaponLeft->canFire(distanceToTarget))
					{
						weaponLeft = nullptr;
					}
					// We cannot fire if both weapons are out of range
					canFire = canFire && (weaponLeft || weaponRight);
				}
				// Check if we should turn to target tile (only do this if stationary)
				if (canFire && current_movement_state == AgentType::MovementState::None)
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
		if (target_hand_state == AgentType::HandState::Aiming)
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
		if (firing_animation_ticks_remaining == 0 &&
		    target_hand_state == AgentType::HandState::Aiming)
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
				auto targetVector = targetPosition - position;
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
				auto p = firingWeapon->fire(targetPosition, targetUnit);
				map.addObjectToMap(p);
				state.current_battle->projectiles.insert(p);
				displayedItem = firingWeapon->type;
				setHandState(AgentType::HandState::Firing);
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
		    body_animation_ticks_remaining == 0 &&
		    current_hand_state != AgentType::HandState::Aiming &&
		    current_movement_state != AgentType::MovementState::Running &&
		    current_movement_state != AgentType::MovementState::Strafing &&
		    !(current_body_state == AgentType::BodyState::Prone &&
		      current_movement_state != AgentType::MovementState::None))
		{
			beginHandStateChange(AgentType::HandState::Aiming);
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
		         current_hand_state == AgentType::HandState::Aiming)
		{
			beginHandStateChange(AgentType::HandState::AtEase);
		}
	} // end if not Firing

	// FIXME: Soldier "thinking" (auto-attacking, auto-turning)
}

void BattleUnit::updateDisplayedItem()
{
	auto lastDisplayedItem = displayedItem;
	bool foundThrownItem = false;
	if (missions.size() > 0)
	{
		for (auto &m : missions)
		{
			if (m->type != BattleUnitMission::MissionType::ThrowItem || !m->item)
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
		if (hand_animation_ticks_remaining > 0 ||
		    current_hand_state != AgentType::HandState::AtEase)
		{
			setHandState(AgentType::HandState::AtEase);
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
	auto targetState = AgentType::BodyState::Standing;
	while (targetState != AgentType::BodyState::Downed &&
	       agent->getAnimationPack()->getFrameCountBody(displayedItem, current_body_state,
	                                                    targetState, current_hand_state,
	                                                    current_movement_state, facing) == 0)
	{
		switch (targetState)
		{
			case AgentType::BodyState::Standing:
				if (agent->isBodyStateAllowed(AgentType::BodyState::Flying))
				{
					targetState = AgentType::BodyState::Flying;
					continue;
				}
			// Intentional fall-through
			case AgentType::BodyState::Flying:
				if (agent->isBodyStateAllowed(AgentType::BodyState::Kneeling))
				{
					targetState = AgentType::BodyState::Kneeling;
					continue;
				}
			// Intentional fall-through
			case AgentType::BodyState::Kneeling:
				if (canProne(position, facing))
				{
					targetState = AgentType::BodyState::Prone;
					continue;
				}
			// Intentional fall-through
			case AgentType::BodyState::Prone:
				// If we arrived here then we have no animation for standing up
				targetState = AgentType::BodyState::Downed;
				continue;
			case AgentType::BodyState::Downed:
			case AgentType::BodyState::Jumping:
			case AgentType::BodyState::Throwing:
				LogError("Not possible to reach this?");
				break;
		}
	}
	// Find state we can rise into (with no animation)
	if (targetState == AgentType::BodyState::Downed)
	{
		if (agent->isBodyStateAllowed(AgentType::BodyState::Standing))
		{
			targetState = AgentType::BodyState::Standing;
		}
		else if (agent->isBodyStateAllowed(AgentType::BodyState::Flying))
		{
			targetState = AgentType::BodyState::Flying;
		}
		else if (agent->isBodyStateAllowed(AgentType::BodyState::Kneeling))
		{
			targetState = AgentType::BodyState::Kneeling;
		}
		else if (canProne(position, facing))
		{
			targetState = AgentType::BodyState::Prone;
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
	setMovementState(AgentType::MovementState::None);
	setHandState(AgentType::HandState::AtEase);
	setBodyState(target_body_state);
	// Check if we can drop from current state
	while (agent->getAnimationPack()->getFrameCountBody(
	           displayedItem, current_body_state, AgentType::BodyState::Downed, current_hand_state,
	           current_movement_state, facing) == 0)
	{
		switch (current_body_state)
		{
			case AgentType::BodyState::Jumping:
			case AgentType::BodyState::Throwing:
			case AgentType::BodyState::Flying:
				if (agent->isBodyStateAllowed(AgentType::BodyState::Standing))
				{
					setBodyState(AgentType::BodyState::Standing);
					continue;
				}
			// Intentional fall-through
			case AgentType::BodyState::Standing:
				if (agent->isBodyStateAllowed(AgentType::BodyState::Kneeling))
				{
					setBodyState(AgentType::BodyState::Kneeling);
					continue;
				}
			// Intentional fall-through
			case AgentType::BodyState::Kneeling:
				setBodyState(AgentType::BodyState::Prone);
				continue;
			case AgentType::BodyState::Prone:
			case AgentType::BodyState::Downed:
				LogError("Not possible to reach this?");
				break;
		}
		break;
	}
	std::list<sp<AEquipment>> itemsToDrop;
	for (auto &m : missions)
	{
		if (m->item && m->item->equippedSlotType != AgentEquipmentLayout::EquipmentSlotType::None)
		{
			itemsToDrop.push_back(m->item);
		}
	}
	missions.clear();
	for (auto it : itemsToDrop)
	{
		addMission(state, BattleUnitMission::dropItem(*this, it));
	}
	addMission(state, BattleUnitMission::changeStance(*this, AgentType::BodyState::Downed));
}

void BattleUnit::retreat(GameState &state)
{
	std::ignore = state;
	tileObject->removeFromMap();
	retreated = true;
	removeFromSquad(*state.current_battle);
	// FIXME: Trigger retreated event
}

void BattleUnit::die(GameState &state, bool violently)
{
	if (violently)
	{
		// FIXME: Explode if nessecary, or spawn shit
	}
	// FIXME: do what has to be done when unit dies
	// Drop equipment, make notification,...
	dropDown(state);
}

void BattleUnit::fallUnconscious(GameState &state)
{
	// FIXME: do what has to be done when unit goes unconscious
	dropDown(state);
}

void BattleUnit::beginBodyStateChange(AgentType::BodyState state)
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
	if (frameCount == 0 && current_movement_state != AgentType::MovementState::None)
	{
		frameCount = agent->getAnimationPack()->getFrameCountBody(
		    displayedItem, current_body_state, state, current_hand_state,
		    AgentType::MovementState::None, facing);
		if (frameCount != 0)
		{
			setMovementState(AgentType::MovementState::None);
		}
	}
	// Try stopping aiming
	if (frameCount == 0 && current_hand_state != AgentType::HandState::AtEase)
	{
		frameCount = agent->getAnimationPack()->getFrameCountBody(
		    displayedItem, current_body_state, state, AgentType::HandState::AtEase,
		    current_movement_state, facing);
		if (frameCount != 0)
		{
			setHandState(AgentType::HandState::AtEase);
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

void BattleUnit::setBodyState(AgentType::BodyState state)
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

void BattleUnit::beginHandStateChange(AgentType::HandState state)
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

void BattleUnit::setHandState(AgentType::HandState state)
{
	current_hand_state = state;
	target_hand_state = state;
	hand_animation_ticks_remaining = 0;
	firing_animation_ticks_remaining =
	    state != AgentType::HandState::Firing
	        ? 0
	        : agent->getAnimationPack()->getFrameCountFiring(displayedItem, current_body_state,
	                                                         current_movement_state, facing) *
	              TICKS_PER_FRAME_UNIT;
	aiming_ticks_remaining = state == AgentType::HandState::Aiming ? TICKS_PER_SECOND / 3 : 0;
}

void BattleUnit::beginTurning(Vec2<int> newFacing)
{
	goalFacing = newFacing;
	turning_animation_ticks_remaining = TICKS_PER_FRAME_UNIT;
}

void BattleUnit::setMovementState(AgentType::MovementState state)
{
	current_movement_state = state;
	switch (state)
	{
		case AgentType::MovementState::None:
			movement_ticks_passed = 0;
			movement_sounds_played = 0;
			ticksTillNextTargetCheck = 0;
			break;
		case AgentType::MovementState::Running:
		case AgentType::MovementState::Strafing:
			if (current_hand_state != AgentType::HandState::AtEase ||
			    target_hand_state != AgentType::HandState::AtEase)
			{
				setHandState(AgentType::HandState::AtEase);
			}
			break;
		default:
			break;
	}
}

unsigned int BattleUnit::getWalkSoundIndex()
{
	if (current_movement_state == AgentType::MovementState::Running)
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

// Alexey Andronov: Istrebitel
// Made up values calculated by trying several throws in game
// This formula closely resembles results I've gotten
// But it may be completely wrong
float BattleUnit::getMaxThrowDistance(int weight, int heightDifference)
{
	float max = 30.0f;
	if (weight <= 2)
	{
		return max;
	}
	int mod = heightDifference > 0 ? heightDifference : heightDifference * 2;
	return std::max(
	    0.0f, std::min(max, (float)agent->modified_stats.strength / ((float)weight - 1) - 2 + mod));
}

bool BattleUnit::shouldPlaySoundNow()
{
	bool play = false;
	unsigned int sounds_to_play = getDistanceTravelled() / UNITS_TRAVELLED_PER_SOUND;
	if (sounds_to_play != movement_sounds_played)
	{
		unsigned int divisor = (current_movement_state == AgentType::MovementState::Running)
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

bool BattleUnit::getNextBodyState(GameState &state, AgentType::BodyState &dest)
{
	if (missions.empty())
	{
		return false;
	}
	return missions.front()->getNextBodyState(state, *this, dest);
}

bool BattleUnit::addMission(GameState &state, BattleUnitMission::MissionType type)
{
	switch (type)
	{
		case BattleUnitMission::MissionType::Fall:
			return addMission(state, BattleUnitMission::fall(*this));
		case BattleUnitMission::MissionType::RestartNextMission:
			return addMission(state, BattleUnitMission::restartNextMission(*this));
		case BattleUnitMission::MissionType::ReachGoal:
			if (!addMission(state, BattleUnitMission::reachGoal(*this), false))
			{
				return false;
			}
			if (!addMission(state, BattleUnitMission::turn(*this, goalPosition)))
			{
				LogError("Could not add turn in front of reachGoal? WTF?");
				return false;
			}
			return true;
		case BattleUnitMission::MissionType::ThrowItem:
		case BattleUnitMission::MissionType::Snooze:
		case BattleUnitMission::MissionType::ChangeBodyState:
		case BattleUnitMission::MissionType::Turn:
		case BattleUnitMission::MissionType::AcquireTU:
		case BattleUnitMission::MissionType::GotoLocation:
		case BattleUnitMission::MissionType::Teleport:
			LogError("Cannot add mission by type if it requires parameters");
			break;
		default:
			LogError("Unimplemented");
			break;
	}
	return false;
}

bool BattleUnit::addMission(GameState &state, BattleUnitMission *mission, bool start)
{
	switch (mission->type)
	{
		// Falling:
		// - If waiting for TUs, cancel everything and fall
		// - If throwing, append after throw
		// - Otherwise, append in front
		case BattleUnitMission::MissionType::Fall:
			if (falling)
			{
				LogError("Falling when already falling? WTF?");
				return false;
			}
			if (missions.empty())
			{
				missions.emplace_front(mission);
				if (start)
				{
					mission->start(state, *this);
				}
			}
			else if (missions.front()->type == BattleUnitMission::MissionType::AcquireTU)
			{
				missions.clear();
				missions.emplace_front(mission);
				if (start)
				{
					mission->start(state, *this);
				}
			}
			else
			{
				auto it = missions.begin();
				while (it != missions.end() &&
				       (*it)->type != BattleUnitMission::MissionType::ThrowItem &&
				       ++it != missions.end())
				{
				}
				if (it == missions.end())
				{
					missions.emplace_front(mission);
					if (start)
					{
						mission->start(state, *this);
					}
				}
				else
				{
					missions.emplace(++it, mission);
					if (start)
					{
						mission->start(state, *this);
					}
				}
			}
			break;
		// Reach goal can only be added if it can overwrite the mission
		case BattleUnitMission::MissionType::ReachGoal:
		{
			// If current mission does not prevent moving - then move to goal
			bool shouldMoveToGoal = true;
			if (missions.size() > 0)
			{
				switch (missions.front()->type)
				{
					// Missions that prevent going to goal
					case BattleUnitMission::MissionType::Fall:
					case BattleUnitMission::MissionType::Snooze:
					case BattleUnitMission::MissionType::ThrowItem:
					case BattleUnitMission::MissionType::ChangeBodyState:
					case BattleUnitMission::MissionType::ReachGoal:
					case BattleUnitMission::MissionType::DropItem:
					case BattleUnitMission::MissionType::Teleport:
						shouldMoveToGoal = false;
						break;
					// Turn prevents moving to goal if it does not require goal
					case BattleUnitMission::MissionType::Turn:
						shouldMoveToGoal = missions.front()->requireGoal;
						break;
					// Missions that can be overwritten
					case BattleUnitMission::MissionType::AcquireTU:
					case BattleUnitMission::MissionType::GotoLocation:
					case BattleUnitMission::MissionType::RestartNextMission:
						shouldMoveToGoal = true;
						break;
				}
			}
			if (!shouldMoveToGoal)
			{
				return false;
			}
			missions.emplace_front(mission);
			if (start)
			{
				mission->start(state, *this);
			}
			break;
		}
		// Turn that requires goal can only be added
		// if it's not added in front of "reachgoal" mission
		case BattleUnitMission::MissionType::Turn:
		{
			if (mission->requireGoal)
			{
				auto it = missions.begin();
				while (it != missions.end() &&
				       (*it)->type != BattleUnitMission::MissionType::ReachGoal &&
				       ++it != missions.end())
				{
				}
				if (it != missions.end())
				{
					return false;
				}
			}
			missions.emplace_front(mission);
			if (start)
			{
				mission->start(state, *this);
			}
			break;
		}
		// Snooze, change body state, acquire TU, restart and teleport can be added at any time
		case BattleUnitMission::MissionType::Snooze:
		case BattleUnitMission::MissionType::ChangeBodyState:
		case BattleUnitMission::MissionType::AcquireTU:
		case BattleUnitMission::MissionType::RestartNextMission:
		case BattleUnitMission::MissionType::Teleport:
		case BattleUnitMission::MissionType::DropItem:
			missions.emplace_front(mission);
			if (start)
			{
				mission->start(state, *this);
			}
			break;
		// FIXME: Implement this
		case BattleUnitMission::MissionType::ThrowItem:
		case BattleUnitMission::MissionType::GotoLocation:
			LogWarning("Adding Throw/GoTo : Ensure implemented correctly!");
			missions.emplace_front(mission);
			if (start)
			{
				mission->start(state, *this);
			}
			break;
	}
	return true;
}

// FIXME: When unit dies, gets destroyed, retreats or changes ownership, remove it from squad
}

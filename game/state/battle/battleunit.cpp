#include "game/state/battle/battleunit.h"
#include "framework/framework.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunitanimationpack.h"
#include "game/state/gamestate.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/state/tileview/tileobject_shadow.h"
#include <cmath>

namespace OpenApoc
{

void BattleUnit::removeFromSquad()
{
	auto b = battle.lock();
	if (!b)
	{
		LogError("removeFromSquad - Battle disappeared");
	}
	b->forces[owner].removeAt(squadNumber, squadPosition);
}

bool BattleUnit::assignToSquad(int squad)
{
	auto b = battle.lock();
	if (!b)
	{
		LogError("assignToSquad - Battle disappeared");
	}
	return b->forces[owner].insert(squad, shared_from_this());
}

void BattleUnit::moveToSquadPosition(int position)
{
	auto b = battle.lock();
	if (!b)
	{
		LogError("moveToSquadPosition - Battle disappeared");
	}
	b->forces[owner].insertAt(squadNumber, position, shared_from_this());
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

StateRef<AEquipmentType> BattleUnit::getDisplayedItem() const
{
	if (missions.size() > 0)
	{
		for (auto &m : missions)
		{
			auto item = m->item;
			if (item)
			{
				return item->type;
			}
		}
	}
	return agent->getDominantItemInHands();
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

bool BattleUnit::isBusy() const
{
	// FIXME: handle units busy with firing, aiming or other stuff
	return !isStatic() || false;
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

bool BattleUnit::canGoProne(Vec3<int> pos, Vec2<int> fac) const
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
		                          agent->type->bodyType->height.at(AgentType::BodyState::Prone))
			&& (legsPos == (Vec3<int>)position || !legsTile->getUnitIfPresent(true, true)))
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
bool BattleUnit::applyDamage(GameState &state, int damage, float armour)
{
	std::ignore = state;
	std::ignore = damage;
	std::ignore = armour;
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
	return false;
}

// FIXME: Handle unit's collision with projectile
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
		// auto vehicleDir = glm::round(this->facing);
		// auto projectileDir = glm::normalize(projectile->getVelocity());
		// auto dir = vehicleDir + projectileDir;
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
		//	auto doodad = city->placeDoodad(StateRef<DoodadType>{&state, "DOODAD_EXPLOSION_2"},
		//		this->tileObject->getPosition());

		//	this->shadowObject->removeFromMap();
		//	this->tileObject->removeFromMap();
		//	this->shadowObject.reset();
		//	this->tileObject.reset();
		//	state.vehicles.erase(this->getId(state, this->shared_from_this()));
		//	return;
		//}
	}
}

void BattleUnit::update(GameState &state, unsigned int ticks)
{
	static const std::set<TileObject::Type> mapPartSet = {
	    TileObject::Type::Ground, TileObject::Type::LeftWall, TileObject::Type::RightWall,
	    TileObject::Type::Feature};

	if (destroyed || retreated)
	{
		return;
	}

	auto b = battle.lock();
	if (!b)
		return;

	if (b->mode == Battle::Mode::RealTime)
		agent->modified_stats.restoreTU();

	if (!this->missions.empty())
		this->missions.front()->update(state, *this, ticks);

	// FIXME: Regenerate stamina

	// Stun removal
	if (stunDamageInTicks > 0)
	{
		stunDamageInTicks = std::max(0, stunDamageInTicks - (int)ticks);
	}

	// Attempt rising if laying on the ground and not unconscious
	if (!isUnconscious() && !isConscious())
	{
		tryToRiseUp(state);
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
	if (missions.empty() && !isUnconscious() && !isDead())
	{
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
		// Try giving way if asked to
		// FIXME: Ensure we're not in a firefight before giving way!
		if (giveWayRequest.size() > 0)
		{
			// If we're given a giveWay request 0, 0 it means we're asked to kneel temporarily
			if (giveWayRequest.size() == 1 && giveWayRequest.front().x == 0 &&
			    giveWayRequest.front().y == 0)
			{
				// Give time for that unit to pass
				missions.emplace_front(BattleUnitMission::snooze(*this, TICKS_PER_SECOND));
				// Give way
				missions.emplace_front(
				    BattleUnitMission::changeStance(*this, AgentType::BodyState::Kneeling));
				missions.front()->start(state, *this);
			}
			else
			{
				auto from = tileObject->getOwningTile();
				for (auto newHeading : giveWayRequest)
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
						// If heading is acceptable
						if (BattleUnitTileHelper{tileObject->map, *this}.canEnterTile(from, to) &&
						    BattleUnitTileHelper{tileObject->map, *this}.canEnterTile(to, from))
						{
							// Give way (move 1 off)
							missions.emplace_back(
							    BattleUnitMission::gotoLocation(*this, pos, 0, false));
							// Turn to previous facing
							missions.emplace_back(BattleUnitMission::turn(*this, facing));
							// Give time for that unit to pass
							missions.emplace_back(
							    BattleUnitMission::snooze(*this, TICKS_PER_SECOND));
							// Return to our position after we're done
							missions.emplace_back(
							    BattleUnitMission::gotoLocation(*this, position, 0, false));
							// Turn to previous facing
							missions.emplace_back(BattleUnitMission::turn(*this, facing));
							// Start giving way
							missions.front()->start(state, *this);
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
		else
		{
			setMovementState(AgentType::MovementState::None);
			// Kneel if not kneeling and should kneel
			if (kneeling_mode == KneelingMode::Kneeling &&
			    current_body_state != AgentType::BodyState::Kneeling && canKneel())
			{
				missions.emplace_front(
				    BattleUnitMission::changeStance(*this, AgentType::BodyState::Kneeling));
				missions.front()->start(state, *this);
			}
			// Go prone if not prone and should stay prone
			else if (movement_mode == BattleUnit::MovementMode::Prone &&
			         current_body_state != AgentType::BodyState::Prone &&
			         kneeling_mode != KneelingMode::Kneeling && canGoProne(position, facing))
			{
				missions.emplace_front(
				    BattleUnitMission::changeStance(*this, AgentType::BodyState::Prone));
				missions.front()->start(state, *this);
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
					missions.emplace_front(
					    BattleUnitMission::changeStance(*this, AgentType::BodyState::Standing));
					missions.front()->start(state, *this);
				}
				else
				{
					missions.emplace_front(
					    BattleUnitMission::changeStance(*this, AgentType::BodyState::Flying));
					missions.front()->start(state, *this);
				}
			}
			// Stop flying if we can stand
			else if (current_body_state == AgentType::BodyState::Flying &&
			         tileObject->getOwningTile()->getCanStand(isLarge()) &&
			         agent->isBodyStateAllowed(AgentType::BodyState::Standing))
			{
				missions.emplace_front(
				    BattleUnitMission::changeStance(*this, AgentType::BodyState::Standing));
				missions.front()->start(state, *this);
			}
			// Stop being prone if legs are no longer supported and we haven't taken a mission yet
			if (current_body_state == AgentType::BodyState::Prone && missions.empty())
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
				if (!hasSupport)
				{
					missions.emplace_front(
					    BattleUnitMission::changeStance(*this, AgentType::BodyState::Kneeling));
					missions.front()->start(state, *this);
				}
			}
		}
	} // End of Idling

	// Movement and Body Animation
	{
		atGoal = false;
		bool wasUsingLift = usingLift;
		usingLift = false;

		// Turn off Jetpacks
		if (current_body_state != AgentType::BodyState::Flying)
		{
			flyingSpeedModifier = 0;
		}

		// If not running we will consume this twice as fast
		int moveTicksRemaining = ticks * agent->modified_stats.getActualSpeedValue() * 2;
		int bodyTicksRemaining = ticks;
		int handTicksRemaining = ticks;
		int turnTicksRemaining = ticks;

		// Bodies can only change their body state
		if (isUnconscious() || isDead())
		{
			moveTicksRemaining = 0;
			handTicksRemaining = 0;
			turnTicksRemaining = 0;
		}

		int lastMoveTicksRemaining = 0;
		int lastBodyTicksRemaining = 0;
		int lastHandTicksRemaining = 0;
		int lastTurnTicksRemaining = 0;

		while (lastMoveTicksRemaining != moveTicksRemaining ||
		       lastBodyTicksRemaining != bodyTicksRemaining ||
		       lastHandTicksRemaining != handTicksRemaining ||
		       lastTurnTicksRemaining != turnTicksRemaining)
		{
			lastMoveTicksRemaining = moveTicksRemaining;
			lastBodyTicksRemaining = bodyTicksRemaining;
			lastHandTicksRemaining = handTicksRemaining;
			lastTurnTicksRemaining = turnTicksRemaining;

			// Try changing body state
			if (body_animation_ticks_remaining > 0)
			{
				if (body_animation_ticks_remaining > bodyTicksRemaining)
				{
					body_animation_ticks_remaining -= bodyTicksRemaining;
					bodyTicksRemaining = 0;
				}
				else
				{
					bodyTicksRemaining -= body_animation_ticks_remaining;
					setBodyState(target_body_state);
				}
			}

			// Try changing hand state
			if (hand_animation_ticks_remaining > 0)
			{
				if (hand_animation_ticks_remaining > handTicksRemaining)
				{
					hand_animation_ticks_remaining -= handTicksRemaining;
					handTicksRemaining = 0;
				}
				else
				{
					handTicksRemaining -= hand_animation_ticks_remaining;
					hand_animation_ticks_remaining = 0;
					setHandState(target_hand_state);
				}
			}

			// Try moving
			if (moveTicksRemaining > 0)
			{
				// If not falling see if we shouldn't start falling?
				if (!falling)
				{
					// Check if should fall or start flying
					if (!canFly() || current_body_state != AgentType::BodyState::Flying)
					{
						bool hasSupport = false;
						for (auto t : tileObject->occupiedTiles)
						{
							if (tileObject->map.getTile(t)->getCanStand())
							{
								hasSupport = true;
								break;
							}
						}
						if (!hasSupport)
						{
							if (canFly())
							{
								if (body_animation_ticks_remaining == 0)
								{
									setBodyState(AgentType::BodyState::Flying);
								}
							}
							else
							{
								missions.emplace_front(BattleUnitMission::fall(*this));
								missions.front()->start(state, *this);
							}
						}
					}
				}

				// If falling then process falling
				if (falling)
				{
					// Falling always consumes all our movement ticks
					moveTicksRemaining = 0;
					// Falling units can always turn
					atGoal = true;

					// Handle falling soldiers
					fallingSpeed += static_cast<float>(ticks) / TICK_SCALE;
					fallingSpeed = std::min(fallingSpeed, FALLING_SPEED_CAP);

					setPosition(position -
					            Vec3<float>{0.0f, 0.0f, ((static_cast<float>(ticks) / TICK_SCALE) *
					                                     fallingSpeed)} /
					                VELOCITY_SCALE_BATTLE);
					goalPosition = position;
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
						// FIXME: Fall unconscious if dropped on another unit!
						// if (fallen on another unit) then (deal stun damage enough to knock us
						// down for some time)
					}
				}
				// Not falling and moving
				else if (current_movement_state != AgentType::MovementState::None)
				{
					int speedModifier = 100;
					if (current_body_state == AgentType::BodyState::Flying)
					{
						speedModifier = std::max(1, flyingSpeedModifier);
					}

					Vec3<float> vectorToGoal = goalPosition - getPosition();
					int distanceToGoal = ceilf(glm::length(vectorToGoal * VELOCITY_SCALE_BATTLE *
					                                       (float)TICKS_PER_UNIT_TRAVELLED));
					int moveTicksConsumeRate =
					    current_movement_state == AgentType::MovementState::Running ? 1 : 2;

					if (distanceToGoal > 0 && current_body_state != AgentType::BodyState::Flying &&
					    vectorToGoal.x == 0 && vectorToGoal.y == 0)
					{
						// FIXME: Actually read set option
						bool USER_OPTION_GRAVLIFT_SOUNDS = true;
						if (!wasUsingLift)
						{
							fw().soundBackend->playSample(b->common_sample_list->gravlift,
							                              getPosition(), 0.25f);
						}
						usingLift = true;
						movement_ticks_passed = 0;
					}
					int movementTicksAccumulated = 0;
					if (distanceToGoal * moveTicksConsumeRate * 100 / speedModifier <=
					    moveTicksRemaining)
					{
						if (distanceToGoal > 0)
						{
							movementTicksAccumulated = distanceToGoal;
							if (flyingSpeedModifier != 100)
							{
								flyingSpeedModifier =
								    std::min(100, flyingSpeedModifier +
								                      distanceToGoal / FLYING_ACCELERATION_DIVISOR);
							}
							moveTicksRemaining -= distanceToGoal * moveTicksConsumeRate;
							setPosition(goalPosition);
							goalPosition = getPosition();
						}
						atGoal = true;
					}
					else
					{
						if (flyingSpeedModifier != 100)
						{
							flyingSpeedModifier =
							    std::min(100, flyingSpeedModifier +
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
					}
					// Scale ticks so that animations look proper on isometric sceen
					// facing down or up on screen
					if (facing.x == facing.y)
					{
						movement_ticks_passed += movementTicksAccumulated * 2 / 3;
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
					// Footsteps
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
					// Check if we should adjust our goal position
					if (goalPosition == getPosition())
					{
						goalPosition = tileObject->getOwningTile()->getRestingPosition(isLarge());
					}
					atGoal = goalPosition == getPosition();
					if (!atGoal)
					{
						// If current mission does not prevent moving - then move to goal
						bool shouldMoveToGoal = true;
						if (missions.size() > 0)
						{
							switch (missions.front()->type)
							{
								case BattleUnitMission::MissionType::Fall:
								case BattleUnitMission::MissionType::Snooze:
								case BattleUnitMission::MissionType::ThrowItem:
								case BattleUnitMission::MissionType::ChangeBodyState:
								case BattleUnitMission::MissionType::Turn:
								case BattleUnitMission::MissionType::ReachGoal:
									shouldMoveToGoal = false;
									break;
								// Missions that can be overwritten
								case BattleUnitMission::MissionType::AcquireTU:
								case BattleUnitMission::MissionType::GotoLocation:
								case BattleUnitMission::MissionType::RestartNextMission:
									shouldMoveToGoal = true;
									break;
							}
						}
						if (shouldMoveToGoal)
						{
							missions.emplace_front(BattleUnitMission::reachGoal(*this));
							missions.emplace_front(BattleUnitMission::turn(*this, goalPosition));
							missions.front()->start(state, *this);
						}
					}
				}
			}

			// Try finishing missions and starting new ones
			while (missions.size() > 0 && missions.front()->isFinished(state, *this))
			{
				LogWarning("Unit mission \"%s\" finished", missions.front()->getName().cStr());
				missions.pop_front();

				// We may have retreated as a result of finished mission
				if (retreated)
					return;

				if (!missions.empty())
				{
					missions.front()->start(state, *this);
					continue;
				}
				else
				{
					LogWarning("No next unit mission, going idle");
					setMovementState(AgentType::MovementState::None);
					break;
				}
			}

			// Try picking new destination if still have movement ticks remaining
			if (missions.size() > 0 && atGoal)
			{
				atGoal = !missions.front()->getNextDestination(state, *this, goalPosition);
			}

			// Try turning
			if (turning_animation_ticks_remaining > 0)
			{
				if (turning_animation_ticks_remaining > turnTicksRemaining)
				{
					turning_animation_ticks_remaining -= turnTicksRemaining;
					turnTicksRemaining = 0;
				}
				else
				{
					turnTicksRemaining -= turning_animation_ticks_remaining;
					turning_animation_ticks_remaining = 0;
					facing = goalFacing;
				}
			}
		}

	} // End of Movement and Body Animation

	// Firing

	{
		// FIXME: TODO

		// TBD later
	}

	// FIXME: Soldier "thinking" (auto-attacking, auto-turning)
}

void BattleUnit::destroy(GameState &)
{
	auto this_shared = shared_from_this();
	auto b = battle.lock();
	if (b)
		b->units.remove(this_shared);
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

	// Do not rise if out of TUs
	if (missions.size() > 0 && missions.front()->type == BattleUnitMission::MissionType::AcquireTU)
		return;

	// Check if we can rise into target state
	auto targetState = AgentType::BodyState::Standing;
	while (agent->getAnimationPack()->getFrameCountBody(getDisplayedItem(), current_body_state,
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
				targetState = AgentType::BodyState::Prone;
				continue;
		}
		break;
	}
	missions.clear();
	missions.emplace_front(BattleUnitMission::changeStance(*this, targetState));
	missions.front()->start(state, *this);
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
	           getDisplayedItem(), current_body_state, AgentType::BodyState::Downed,
	           current_hand_state, current_movement_state, facing) == 0)
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
			case AgentType::BodyState::Downed:
				break;
		}
		break;
	}
	missions.clear();
	missions.emplace_front(BattleUnitMission::fall(*this));
	missions.front()->start(state, *this); // Start falling immediately
	missions.emplace_front(BattleUnitMission::changeStance(*this, AgentType::BodyState::Downed));
	missions.front()->start(state, *this);
}

void BattleUnit::retreat(GameState &state)
{
	tileObject->removeFromMap();
	retreated = true;
	removeFromSquad();
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

void BattleUnit::beginBodyStateChange(AgentType::BodyState state, int ticks)
{
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

void BattleUnit::setHandState(AgentType::HandState state)
{
	current_hand_state = state;
	target_hand_state = state;
	hand_animation_ticks_remaining = 0;
}

void BattleUnit::setMovementState(AgentType::MovementState state)
{
	current_movement_state = state;
	if (state == AgentType::MovementState::None)
	{
		movement_ticks_passed = 0;
		movement_sounds_played = 0;
	}
}

int BattleUnit::getWalkSoundIndex()
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

bool BattleUnit::shouldPlaySoundNow()
{
	bool play = false;
	int sounds_to_play = getDistanceTravelled() / UNITS_TRAVELLED_PER_SOUND;
	if (sounds_to_play != movement_sounds_played)
	{
		int divisor = (current_movement_state == AgentType::MovementState::Running)
		                  ? UNITS_TRAVELLED_PER_SOUND_RUNNING_DIVISOR
		                  : 1;
		play = ((sounds_to_play + divisor - 1) % divisor) == 0;
		movement_sounds_played = sounds_to_play;
	}
	return play;
}

// FIXME: When unit dies, gets destroyed, retreats or changes ownership, remove it from squad
}

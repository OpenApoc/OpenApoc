#include "game/state/battle/battleunit.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunitanimationpack.h"
#include "game/state/gamestate.h"
#include "game/state/tileview/tileobject_shadow.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/state/tileview/collision.h"

namespace OpenApoc
{
	bool BattleUnit::isBodyStateAllowed(AgentType::BodyState bodyState)
	{
		if (bodyState == AgentType::BodyState::Flying)
		{
			// FIXME: See if unit is wearing flying armor
		}
		return agent->type->allowed_body_states.find(bodyState) != agent->type->allowed_body_states.end();
	}

	bool BattleUnit::assignToSquad(int squad)
	{
		auto b = battle.lock();
		if (!b)
		{
			LogError("Battle disappeared");
		}
		return b->forces[owner].insert(squad, shared_from_this());
	}

	void BattleUnit::moveToSquadPosition(int position)
	{
		auto b = battle.lock();
		if (!b)
		{
			LogError("Battle disappeared");
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
		}
		else
		{
			this->tileObject->setPosition(pos);
		}

		if (this->shadowObject)
		{
			this->shadowObject->setPosition(pos);
		}
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
		int SCALE = 1;
		return stunDamageInTicks / SCALE;
	}

	bool BattleUnit::isDead() const
	{
		return getHealth() == 0 || destroyed;
	}

	bool BattleUnit::isUnconscious() const
	{
		return !isDead() && getStunDamage() > getHealth();
	}


	// FIXME: Apply damage to the unit
	bool BattleUnit::applyDamage(GameState &state, int damage, float armour)
	{
		std::ignore = state;
		std::ignore = damage;
		std::ignore = armour;
		//if (this->shield <= damage)
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
		//else
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
			//auto vehicleDir = glm::round(this->facing);
			//auto projectileDir = glm::normalize(projectile->getVelocity());
			//auto dir = vehicleDir + projectileDir;
			//dir = glm::round(dir);

			//auto armourDirection = VehicleType::ArmourDirection::Right;
			//if (dir.x == 0 && dir.y == 0 && dir.z == 0)
			//{
			//	armourDirection = VehicleType::ArmourDirection::Front;
			//}
			//else if (dir * 0.5f == vehicleDir)
			//{
			//	armourDirection = VehicleType::ArmourDirection::Rear;
			//}
			//// FIXME: vehicle Z != 0
			//else if (dir.z < 0)
			//{
			//	armourDirection = VehicleType::ArmourDirection::Top;
			//}
			//else if (dir.z > 0)
			//{
			//	armourDirection = VehicleType::ArmourDirection::Bottom;
			//}
			//else if ((vehicleDir.x == 0 && dir.x != dir.y) || (vehicleDir.y == 0 && dir.x == dir.y))
			//{
			//	armourDirection = VehicleType::ArmourDirection::Left;
			//}

			//float armourValue = 0.0f;
			//auto armour = this->type->armour.find(armourDirection);
			//if (armour != this->type->armour.end())
			//{
			//	armourValue = armour->second;
			//}

			//if (applyDamage(state, projectile->damage, armourValue))
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
		std::ignore = state;
		std::ignore = ticks;
	}

	// FIXME: When unit dies, gets destroyed, retreats or changes ownership, remove it from squad
}

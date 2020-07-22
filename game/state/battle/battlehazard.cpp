#include "game/state/battle/battlehazard.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/rules/doodadtype.h"
#include "game/state/shared/aequipment.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_battlehazard.h"
#include "game/state/tilemap/tileobject_battleitem.h"
#include "game/state/tilemap/tileobject_battlemappart.h"
#include "game/state/tilemap/tileobject_battleunit.h"

#include <cmath>

namespace OpenApoc
{
BattleHazard::BattleHazard(GameState &state, StateRef<DamageType> damageType, bool delayVisibility)
    : damageType(damageType), hazardType(damageType->hazardType)
{
	frame = randBoundsExclusive(state.rng, 0, HAZARD_FRAME_COUNT);
	if (delayVisibility)
	{
		ticksUntilVisible =
		    std::max((unsigned)0, (hazardType->doodadType->lifetime - 4) * TICKS_MULTIPLIER);
	}
	frameChangeTicksAccumulated =
	    randBoundsInclusive(state.rng, (unsigned)0, TICKS_PER_HAZARD_UPDATE);
}

void BattleHazard::die(GameState &state, bool violently)
{
	auto this_shared = shared_from_this();
	tileObject->removeFromMap();
	tileObject.reset();

	if (!violently)
	{
		return;
	}
	// Place smoke where fire died off
	if (damageType->effectType == DamageType::EffectType::Fire)
	{
		StateRef<DamageType> dtSmoke = {&state, "DAMAGETYPE_SMOKE"};
		state.current_battle->placeHazard(state, ownerOrganisation, ownerUnit, dtSmoke, position,
		                                  dtSmoke->hazardType->getLifetime(state), power, 6);
	}
}
void BattleHazard::dieAndRemove(GameState &state, bool violently)
{
	die(state, violently);
	state.current_battle->hazards.erase(shared_from_this());
}

bool BattleHazard::expand(GameState &state, const TileMap &map, const Vec3<int> &to, unsigned ttl,
                          bool fireSmoke)
{
	// list of coordinates to check
	static const std::map<Vec3<int>, std::list<std::pair<Vec3<int>, std::set<TileObject::Type>>>>
	    searchPattern = {
	        // Vertical
	        {{0, 0, 1}, {{{0, 0, 1}, {TileObject::Type::Ground}}}},
	        {{0, 0, -1},
	         {
	             {{0, 0, 0}, {TileObject::Type::Ground}},
	         }},
	        // Horizontal direct
	        {{0, -1, 0},
	         {
	             {{0, 0, 0}, {TileObject::Type::RightWall}},
	         }},
	        {{0, 1, 0},
	         {
	             {{0, 1, 0}, {TileObject::Type::RightWall}},
	         }},
	        {{-1, 0, 0},
	         {
	             {{0, 0, 0}, {TileObject::Type::LeftWall}},
	         }},
	        {{1, 0, 0},
	         {
	             {{1, 0, 0}, {TileObject::Type::LeftWall}},
	         }},
	        // Horizontal top-left
	        {{-1, -1, 0},
	         {
	             {{-1, 0, 0}, {TileObject::Type::Feature}},
	             {{0, -1, 0}, {TileObject::Type::Feature}},
	             {{-1, 0, 0}, {TileObject::Type::RightWall}},
	             {{0, -1, 0}, {TileObject::Type::LeftWall}},
	             {{0, 0, 0}, {TileObject::Type::RightWall}},
	             {{0, 0, 0}, {TileObject::Type::LeftWall}},
	         }},
	        // Horizontal bottom-right
	        {{1, 1, 0},
	         {
	             {{0, 1, 0}, {TileObject::Type::Feature}},
	             {{1, 0, 0}, {TileObject::Type::Feature}},
	             {{0, 1, 0}, {TileObject::Type::RightWall}},
	             {{1, 0, 0}, {TileObject::Type::LeftWall}},
	             {{1, 1, 0}, {TileObject::Type::RightWall}},
	             {{1, 1, 0}, {TileObject::Type::LeftWall}},
	         }},
	        // Horizontal top-right
	        {{1, -1, 0},
	         {
	             {{1, 0, 0}, {TileObject::Type::Feature}},
	             {{0, -1, 0}, {TileObject::Type::Feature}},
	             {{0, 0, 0}, {TileObject::Type::RightWall}},
	             {{1, -1, 0}, {TileObject::Type::LeftWall}},
	             {{1, 0, 0}, {TileObject::Type::RightWall}},
	             {{1, 0, 0}, {TileObject::Type::LeftWall}},
	         }},
	        // Horizontal bottom-left
	        {{-1, 1, 0},
	         {
	             {{-1, 0, 0}, {TileObject::Type::Feature}},
	             {{0, 1, 0}, {TileObject::Type::Feature}},
	             {{-1, 1, 0}, {TileObject::Type::RightWall}},
	             {{0, 0, 0}, {TileObject::Type::LeftWall}},
	             {{0, 1, 0}, {TileObject::Type::RightWall}},
	             {{0, 1, 0}, {TileObject::Type::LeftWall}},
	         }},
	    };

	// Ensure coordinates are ok
	if (to.x < 0 || to.x >= map.size.x || to.y < 0 || to.y >= map.size.y || to.z < 0 ||
	    to.z >= map.size.z)
	{
		return false;
	}

	// Fire spreads smoke
	// Actual spread of fire is handled in the effect application
	auto spreadDamageType = damageType;
	if (fireSmoke)
	{
		spreadDamageType = {&state, "DAMAGETYPE_SMOKE"};
		ttl = spreadDamageType->hazardType->getLifetime(state);
	}

	// Ensure no hazard already there

	sp<BattleHazard> existingHazard;
	bool replaceWeaker = false;
	auto targetTile = map.getTile(to.x, to.y, to.z);
	for (auto &obj : targetTile->ownedObjects)
	{
		if (obj->getType() == TileObject::Type::Hazard)
		{
			existingHazard = std::static_pointer_cast<TileObjectBattleHazard>(obj)->getHazard();
			// Replace weaker hazards (if not smoke from fire or fire itself)
			// Replace non-fire hazards if fire
			if ((hazardType->fire && !fireSmoke &&
			     existingHazard->damageType != spreadDamageType) ||
			    (!hazardType->fire && existingHazard->damageType == spreadDamageType &&
			     existingHazard->lifetime - existingHazard->age < ttl))
			{
				replaceWeaker = true;
			}
			break;
		}
	}
	if (!replaceWeaker && existingHazard)
	{
		return false;
	}

	// Calculate target's resistance to hazard spread

	auto dir = to - (Vec3<int>)position;
	int block = 0;
	for (auto &pair : searchPattern.at(dir))
	{
		auto pos = pair.first + (Vec3<int>)position;
		auto tile = map.getTile(pos);
		for (auto &obj : tile->ownedObjects)
		{
			if (pair.second.find(obj->getType()) != pair.second.end())
			{
				auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(obj)->getOwner();
				block = std::max(block, mp->type->block[spreadDamageType->blockType]);
			}
		}
	}
	// FIXME: Made up, ensure this fits vanilla behavior
	if ((hazardType->fire && !fireSmoke && block == 255) || power <= block)
	{
		return false;
	}

	// If spreading fire
	if (hazardType->fire && !fireSmoke)
	{
		// Find out if tile contains something flammable that we can penetrate
		bool penetrationAchieved = false;
		for (auto &obj : targetTile->ownedObjects)
		{
			if (obj->getType() == TileObject::Type::Ground ||
			    obj->getType() == TileObject::Type::Feature)
			{
				auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(obj)->getOwner();
				if (mp->canBurn(age))
				{
					penetrationAchieved = true;
				}
				break;
			}
		}

		if (penetrationAchieved)
		{
			if (replaceWeaker)
			{
				existingHazard->dieAndRemove(state, false);
			}
			state.current_battle->placeHazard(
			    state, ownerOrganisation, ownerUnit, spreadDamageType, {to.x, to.y, to.z},
			    spreadDamageType->hazardType->getLifetime(state), 0, 1, false);
		}
	}
	// If spreading something else
	else
	{
		// If reached here try place hazard
		if (replaceWeaker)
		{
			if (fireSmoke)
			{
				LogError("Smoke from fire should never try to replace weaker hazards");
				return true;
			}
			existingHazard->lifetime = lifetime;
			existingHazard->age = age;
			existingHazard->ticksUntilVisible = 0;
		}
		else
		{

			auto hazard = state.current_battle->placeHazard(
			    state, ownerOrganisation, ownerUnit, spreadDamageType, {to.x, to.y, to.z},
			    fireSmoke ? ttl : lifetime, fireSmoke ? 1 : power, fireSmoke ? 6 : 1, false);
			if (hazard && !fireSmoke)
			{
				hazard->age = age;
			}
		}
	}

	return true;
}

void BattleHazard::grow(GameState &state)
{
	if (hazardType->fire)
	{
		auto &map = tileObject->map;

		// Try to light up adjacent stuff on fire

		for (int x = position.x - 1; x <= position.x + 1; x++)
		{
			for (int y = position.y - 1; y <= position.y + 1; y++)
			{
				expand(state, map, {x, y, position.z}, 0);
			}
		}
		for (int z = position.z - 1; z <= position.z + 1; z++)
		{
			expand(state, map, {position.x, position.y, z}, 0);
		}

		// Now spread smoke

		if (randBoundsExclusive(state.rng, 0, 100) >= HAZARD_SPREAD_CHANCE)
		{
			return;
		}

		for (int x = position.x - 1; x <= position.x + 1; x++)
		{
			for (int y = position.y - 1; y <= position.y + 1; y++)
			{
				if (expand(state, map, {x, y, position.z}, 0, true))
				{
					return;
				}
			}
		}
		for (int z = position.z - 1; z <= position.z + 1; z++)
		{
			if (expand(state, map, {position.x, position.y, z}, 0, true))
			{
				return;
			}
		}
	}
	else
	{
		if (power == 0)
		{
			return;
		}
		if (randBoundsExclusive(state.rng, 0, 100) >= HAZARD_SPREAD_CHANCE)
		{
			return;
		}
		auto &map = tileObject->map;
		int newTTL = lifetime - age;

		for (int x = position.x - 1; x <= position.x + 1; x++)
		{
			for (int y = position.y - 1; y <= position.y + 1; y++)
			{
				if (expand(state, map, {x, y, position.z}, newTTL))
				{
					return;
				}
			}
		}
		for (int z = position.z - 1; z <= position.z + 1; z++)
		{
			if (expand(state, map, {position.x, position.y, z}, newTTL))
			{
				return;
			}
		}
	}
}

void BattleHazard::applyEffect(GameState &state)
{
	auto tile = tileObject->getOwningTile();

	auto set = tile->ownedObjects;
	for (auto &obj : set)
	{
		if (tile->ownedObjects.find(obj) == tile->ownedObjects.end())
		{
			continue;
		}
		if (obj->getType() == TileObject::Type::Ground ||
		    obj->getType() == TileObject::Type::Feature ||
		    obj->getType() == TileObject::Type::LeftWall ||
		    obj->getType() == TileObject::Type::RightWall)
		{
			auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(obj)->getOwner();
			switch (damageType->effectType)
			{
				case DamageType::EffectType::Fire:
					if (mp->applyBurning(state, age))
					{
						// Map part burned and provided fuel for our fire, keep the fire raging
						if (power < 0 && age > 10)
						{
							power = -power;
						}
					}
					break;
				default:
					switch (damageType->blockType)
					{
						case DamageType::BlockType::Gas:
						case DamageType::BlockType::Psionic:
							break;
						default:
							mp->applyDamage(state, power, damageType);
							break;
					}
					break;
			}
		}
		else if (obj->getType() == TileObject::Type::Item)
		{
			if (damageType->effectType == DamageType::EffectType::Fire)
			{
				// It was observed that armor resists fire damage deal to it
				// It also appears that damage is applied gradually at a rate of around 1 damage per
				// second
				// In tests, marsec armor (20% modifier) was hurt by fire but X-Com armor (10%
				// modifier) was not
				// If we apply damage once per turn, we apply 4 at once. Since we round down, 4 *
				// 20% will be rounded to 0
				// while it should be 1. So we add 1 here
				auto i = std::static_pointer_cast<TileObjectBattleItem>(obj)->getItem();
				i->applyDamage(state, 2 * TICKS_PER_HAZARD_UPDATE / TICKS_PER_SECOND + 1,
				               damageType);
			}
		}
		else if (obj->getType() == TileObject::Type::Unit)
		{
			StateRef<BattleUnit> u = {
			    &state, std::static_pointer_cast<TileObjectBattleUnit>(obj)->getUnit()->id};
			// Determine direction of hit
			Vec3<float> velocity = -position;
			velocity -= Vec3<float>{0.5f, 0.5f, 0.5f};
			velocity += u->position;
			if (velocity.x == 0.0f && velocity.y == 0.0f)
			{
				velocity.z = 1.0f;
			}
			// Determine whether to hit head, legs or torso
			auto cposition = u->position;
			// Hit torso
			if (sqrtf(velocity.x * velocity.x + velocity.y * velocity.y) > std::abs(velocity.z))
			{
				cposition.z += (float)u->getCurrentHeight() / 2.0f / 40.0f;
			}
			// Hit head
			else if (velocity.z < 0)
			{
				cposition.z += (float)u->getCurrentHeight() / 40.0f;
			}
			else
			{
				// Legs are default already
			}
			// Apply
			u->applyDamage(state, power, damageType,
			               u->determineBodyPartHit(damageType, cposition, velocity),
			               DamageSource::Hazard);
		}
	}
}

bool BattleHazard::updateInner(GameState &state, unsigned int ticks)
{
	nextUpdateTicksAccumulated += ticks;
	while (nextUpdateTicksAccumulated >= TICKS_PER_HAZARD_UPDATE)
	{
		nextUpdateTicksAccumulated -= TICKS_PER_HAZARD_UPDATE;
		if (hazardType->fire)
		{
			// Explanation for how fire works is at the end of battlehazard.h
			if (power > 0)
			{
				age -= 6;
				if (age <= 10)
				{
					power = -power;
				}
			}
			else
			{
				age += 10;
			}
			power += power / std::abs(power);
			if (power % 2)
			{
				applyEffect(state);
				if (age < 130)
				{
					grow(state);
				}
			}
			if (age >= 130)
			{
				die(state, true);
				return true;
			}
		}
		else
		{
			age++;
			if (age % 2)
			{
				applyEffect(state);
				updateTileVisionBlock(state);
				if (age < lifetime)
				{
					grow(state);
				}
			}
			if (age >= lifetime)
			{
				die(state, true);
				return true;
			}
		}
	}
	return false;
}

bool BattleHazard::update(GameState &state, unsigned int ticks)
{
	bool realTime = state.current_battle->mode == Battle::Mode::RealTime;

	if (ticksUntilVisible > 0)
	{
		if (ticksUntilVisible > ticks)
		{
			ticksUntilVisible -= ticks;
		}
		else
		{
			ticksUntilVisible = 0;
		}
	}

	frameChangeTicksAccumulated += ticks;
	while (frameChangeTicksAccumulated >= TICKS_PER_HAZARD_UPDATE)
	{
		frameChangeTicksAccumulated -= TICKS_PER_HAZARD_UPDATE;
		frame++;
		frame %= hazardType->fire ? 2 : HAZARD_FRAME_COUNT;
	}

	if (realTime)
	{
		return updateInner(state, ticks);
	}
	return false;
}

bool BattleHazard::updateTB(GameState &state) { return updateInner(state, TICKS_PER_TURN); }

void BattleHazard::updateTileVisionBlock(GameState &state)
{
	int visionBlock =
	    damageType->effectType == DamageType::EffectType::Smoke ? (lifetime - age) / 3 : 0;
	if (tileObject->getOwningTile()->updateVisionBlockage(visionBlock))
	{
		state.current_battle->queueVisionRefresh(position);
	}
}

} // namespace OpenApoc

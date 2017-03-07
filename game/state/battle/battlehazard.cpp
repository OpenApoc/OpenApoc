#include "game/state/battle/battlehazard.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/rules/damage.h"
#include "game/state/rules/doodad_type.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_battlehazard.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_battleunit.h"

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
	ticksUntilNextEffect = TICKS_PER_HAZARD_EFFECT;
	ticksUntilNextFrameChange =
	    randBoundsInclusive(state.rng, (unsigned)0, TICKS_PER_HAZARD_EFFECT);
}

void BattleHazard::die(GameState &state, bool violently)
{
	auto this_shared = shared_from_this();
	state.current_battle->hazards.erase(this_shared);
	this->tileObject->removeFromMap();
	this->tileObject.reset();
	if (!violently)
	{
		return;
	}
	// Place smoke where fire died off
	if (damageType->effectType == DamageType::EffectType::Fire)
	{
		StateRef<DamageType> dtSmoke = {&state, "DAMAGETYPE_SMOKE"};
		state.current_battle->placeHazard(state, dtSmoke, position,
		                                  dtSmoke->hazardType->getLifetime(state), power, 6);
	}
}

bool BattleHazard::expand(GameState &state, const TileMap &map, const Vec3<int> &to, unsigned ttl)
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
	if (damageType->effectType == DamageType::EffectType::Fire)
	{
		spreadDamageType = {&state, "DAMAGETYPE_SMOKE"};
	}

	// Ensure no hazard already there

	sp<BattleHazard> existingHazard;
	bool replaceWeaker = false;
	auto targetTile = map.getTile(to.x, to.y, to.z);
	for (auto obj : targetTile->ownedObjects)
	{
		if (obj->getType() == TileObject::Type::Hazard)
		{
			existingHazard = std::static_pointer_cast<TileObjectBattleHazard>(obj)->getHazard();
			// Replace weaker hazards
			if (existingHazard->damageType == spreadDamageType &&
			    existingHazard->lifetime - existingHazard->age < ttl)
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
		for (auto obj : tile->ownedObjects)
		{
			if (pair.second.find(obj->getType()) != pair.second.end())
			{
				auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(obj)->getOwner();
				block = std::max(block, mp->type->block[spreadDamageType->blockType]);
			}
		}
	}
	// FIXME: Made up, ensure this fits vanilla behavior
	if (power <= block)
	{
		return false;
	}

	// If reached here try place hazard
	if (replaceWeaker)
	{
		existingHazard->lifetime = lifetime;
		existingHazard->age = age;
		existingHazard->ticksUntilVisible = 0;
	}
	else
	{
		auto hazard = state.current_battle->placeHazard(
		    state, spreadDamageType, {to.x, to.y, to.z}, lifetime, power,
		    damageType->effectType == DamageType::EffectType::Fire ? 6 : 1);
		if (hazard && damageType->effectType != DamageType::EffectType::Fire)
		{
			hazard->age = age;
			hazard->ticksUntilVisible = 0;
		}
	}
	return true;
}

void BattleHazard::grow(GameState &state)
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

void BattleHazard::applyEffect(GameState &state)
{
	auto tile = tileObject->getOwningTile();

	auto set = tile->ownedObjects;
	for (auto obj : set)
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
					LogWarning("Set map part on fire!");
					break;
				default:
					mp->applyDamage(state, power, damageType);
					break;
			}
		}
		else if (obj->getType() == TileObject::Type::Unit)
		{
			StateRef<BattleUnit> u = {
			    &state, std::static_pointer_cast<TileObjectBattleUnit>(obj)->getUnit()->id};
			switch (damageType->effectType)
			{
				case DamageType::EffectType::Fire:
					LogWarning("Set unit on fire!");
					break;
				default:
				{
					// Determine direction of hit
					Vec3<float> velocity = -position;
					velocity -= Vec3<float>{0.5f, 0.5f, 0.5f};
					velocity += u->position;
					if (velocity.x == 0.0f && velocity.y == 0.0f)
					{
						velocity.z = 1.0f;
					}
					// Determine wether to hit head, legs or torso
					auto cposition = u->position;
					// Hit torso
					if (sqrtf(velocity.x * velocity.x + velocity.y * velocity.y) >
					    std::abs(velocity.z))
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
						// Legs are defeault already
					}
					// Apply
					u->applyDamage(state, power, damageType,
					               u->determineBodyPartHit(damageType, cposition, velocity));
				}
				break;
			}
		}
	}
}

void BattleHazard::update(GameState &state, unsigned int ticks)
{
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

	ticksUntilNextFrameChange -= ticks;
	if (ticksUntilNextFrameChange <= 0)
	{
		ticksUntilNextFrameChange += TICKS_PER_HAZARD_EFFECT;
		frame++;
		frame %= HAZARD_FRAME_COUNT;
	}

	ticksUntilNextEffect -= ticks;
	if (ticksUntilNextEffect <= 0)
	{
		ticksUntilNextEffect += TICKS_PER_HAZARD_EFFECT;
		applyEffect(state);
		age++;
		updateTileVisionBlock(state);
		if (age >= lifetime)
		{
			die(state);
		}
		else
		{
			grow(state);
		}
	}
}

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

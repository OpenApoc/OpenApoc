#include "game/state/battle/battleexplosion.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlehazard.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/rules/doodadtype.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_battlehazard.h"
#include "game/state/tilemap/tileobject_battleitem.h"
#include "game/state/tilemap/tileobject_battlemappart.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "library/strings_format.h"
#include <cmath>

namespace OpenApoc
{
BattleExplosion::BattleExplosion(Vec3<int> position, StateRef<DamageType> damageType, int power,
                                 int depletionRate, bool damageInTheEnd,
                                 StateRef<Organisation> ownerOrg, StateRef<BattleUnit> ownerUnit)
    : position(position), power(power), ticksUntilExpansion(TICKS_MULTIPLIER * 2),
      locationsToExpand({{{position, {power, power}}}, {}, {}}), damageInTheEnd(damageInTheEnd),
      locationsVisited({position}), damageType(damageType), depletionRate(depletionRate),
      ownerUnit(ownerUnit), ownerOrganisation(ownerOrg)
{
}

void BattleExplosion::die(GameState &state)
{
	auto this_shared = shared_from_this();
	state.current_battle->explosions.erase(this_shared);
	// Deal damage
	if (damageInTheEnd)
	{
		auto &map = *state.current_battle->map;
		for (auto &pos : locationsToDamage)
		{
			damage(state, map, pos.first, pos.second);
		}
	}
	// Hack to make all hazards update at once
	auto &map = *state.current_battle->map;
	for (auto &pos : locationsVisited)
	{
		auto tile = map.getTile(pos.x, pos.y, pos.z);
		sp<BattleHazard> existingHazard;
		for (auto &obj : tile->ownedObjects)
		{
			if (obj->getType() == TileObject::Type::Hazard)
			{
				existingHazard = std::static_pointer_cast<TileObjectBattleHazard>(obj)->getHazard();
				break;
			}
		}
		if (existingHazard)
		{
			existingHazard->nextUpdateTicksAccumulated = 0;
		}
	}
}

void BattleExplosion::update(GameState &state, unsigned int ticks)
{
	ticksUntilExpansion -= ticks;
	while (ticksUntilExpansion <= 0)
	{
		ticksUntilExpansion += TICKS_MULTIPLIER * 2;
		grow(state);
		if (locationsToExpand[0].empty() && locationsToExpand[1].empty() &&
		    locationsToExpand[2].empty())
		{
			die(state);
			return;
		}
	}
}

void BattleExplosion::damage(GameState &state, const TileMap &map, Vec3<int> pos, int damage)
{
	auto tile = map.getTile(pos);
	// Explosions with no hazard spawn smoke with half ttl
	if (!damageType->hazardType)
	{
		StateRef<DamageType> dtSmoke = {&state, "DAMAGETYPE_SMOKE"};
		state.current_battle->placeHazard(state, ownerOrganisation, ownerUnit, dtSmoke, pos,
		                                  dtSmoke->hazardType->getLifetime(state), damage, 2,
		                                  false);
	}
	// Explosions with no custom explosion doodad spawn hazards when dealing damage
	else if (!damageType->explosionDoodad)
	{
		state.current_battle->placeHazard(state, ownerOrganisation, ownerUnit, damageType, pos,
		                                  damageType->hazardType->getLifetime(state), damage, 1,
		                                  false);
	}
	// Gas does no direct damage
	if (damageType->doesImpactDamage())
	{
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
						// Nothing, map parts are not damaged by fire at explosion time
						break;
					default:
						mp->applyDamage(state, damage, damageType);
						break;
				}
			}
			else if (obj->getType() == TileObject::Type::Unit)
			{
				StateRef<BattleUnit> u = {
				    &state, std::static_pointer_cast<TileObjectBattleUnit>(obj)->getUnit()->id};
				if (affectedUnits.find(u) != affectedUnits.end())
				{
					continue;
				}
				affectedUnits.insert(u);
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
				// Hit torso if coming from the side, not from above or below
				if (sqrtf(velocity.x * velocity.x + velocity.y * velocity.y) > std::abs(velocity.z))
				{
					cposition.z += (float)u->getCurrentHeight() / 2.0f / 40.0f;
				}
				// Hit head if coming from above
				else if (velocity.z < 0)
				{
					cposition.z += (float)u->getCurrentHeight() / 40.0f;
				}
				// Hit legs if coming from below
				else
				{
					// Legs are default already
				}
				// Apply
				u->applyDamage(state, damage, damageType,
				               u->determineBodyPartHit(damageType, cposition, velocity),
				               DamageSource::Impact, ownerUnit);
			}
			else if (obj->getType() == TileObject::Type::Item)
			{
				// Special effects do not damage items, fire damages items differently and not on
				// explosion impact
				if (damageType->effectType == DamageType::EffectType::None)
				{
					auto i = std::static_pointer_cast<TileObjectBattleItem>(obj)->getItem();
					i->applyDamage(state, damage, damageType);
				}
			}
		}
	}
}

void BattleExplosion::expand(GameState &state, const TileMap &map, const Vec3<int> &from,
                             const Vec3<int> &to, int nextPower)
{
	// list of coordinates to check
	static const std::map<Vec3<int>, std::list<std::pair<Vec3<int>, std::set<TileObject::Type>>>>
	    searchPattern = {
	        // Vertical
	        {{0, 0, 1}, {{{0, 0, 1}, {TileObject::Type::Ground, TileObject::Type::Feature}}}},
	        {{0, 0, -1},
	         {{{0, 0, 0}, {TileObject::Type::Ground}}, {{0, 0, -1}, {TileObject::Type::Feature}}}},
	        // Horizontal direct
	        {{0, -1, 0},
	         {{{0, 0, 0}, {TileObject::Type::RightWall}},
	          {{0, -1, 0}, {TileObject::Type::Feature}}}},
	        {{0, 1, 0},
	         {{{0, 1, 0}, {TileObject::Type::RightWall}},
	          {{0, 1, 0}, {TileObject::Type::Feature}}}},
	        {{-1, 0, 0},
	         {{{0, 0, 0}, {TileObject::Type::LeftWall}},
	          {{-1, 0, 0}, {TileObject::Type::Feature}}}},
	        {{1, 0, 0},
	         {{{1, 0, 0}, {TileObject::Type::LeftWall}}, {{1, 0, 0}, {TileObject::Type::Feature}}}},
	        // Horizontal top-left
	        {{-1, -1, 0},
	         {
	             {{-1, -1, 0}, {TileObject::Type::Feature}},
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
	             {{1, 1, 0}, {TileObject::Type::Feature}},
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
	             {{1, -1, 0}, {TileObject::Type::Feature}},
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
	             {{-1, 1, 0}, {TileObject::Type::Feature}},
	             {{-1, 0, 0}, {TileObject::Type::Feature}},
	             {{0, 1, 0}, {TileObject::Type::Feature}},
	             {{-1, 1, 0}, {TileObject::Type::RightWall}},
	             {{0, 0, 0}, {TileObject::Type::LeftWall}},
	             {{0, 1, 0}, {TileObject::Type::RightWall}},
	             {{0, 1, 0}, {TileObject::Type::LeftWall}},
	         }},
	    };

	if (to.x < 0 || to.x >= map.size.x || to.y < 0 || to.y >= map.size.y || to.z < 0 ||
	    to.z >= map.size.z || nextPower < 2 * depletionRate ||
	    locationsVisited.find(to) != locationsVisited.end())
	{
		return;
	}
	locationsVisited.insert(to);
	auto dir = to - from;
	int depletionThis = 0;
	int depletionNext = 0;

	// Deplete explosion according to map parts encountered
	for (auto &pair : searchPattern.at(dir))
	{
		auto pos = pair.first + from;
		auto tile = map.getTile(pos);
		for (auto &obj : tile->ownedObjects)
		{
			if (pair.second.find(obj->getType()) != pair.second.end())
			{
				auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(obj)->getOwner();

				int depletion = 2 * mp->type->block[damageType->blockType];

				depletionNext = std::max(depletionNext, depletion);
				// Feature in target tile does not block damage to the tile
				if (!(pos == to && obj->getType() == TileObject::Type::Feature))
				{
					depletionThis = std::max(depletionThis, depletion);
				}
			}
		}
	}
	// FIXME: Actually read this option
	int distance = (1 + (dir.x != 0 ? 1 : 0) + (dir.y != 0 ? 1 : 0) + (dir.z != 0 ? 2 : 0));
	nextPower -= depletionRate * distance / 2;

	// If we reach the tile, and our type has no range dissipation, just apply power
	int thisPower = nextPower - depletionThis;
	if (thisPower > 0 && !damageType->hasDamageDissipation())
	{
		thisPower = power;
	}
	nextPower -= depletionNext;

	// Add this tile to those which will be visited in the future
	if (thisPower > 0)
	{
		// Queue damage application
		locationsToExpand[distance].emplace(to, Vec2<int>{thisPower, nextPower});
		// Spawn doodad
		auto doodadType = damageType->explosionDoodad;
		if (!doodadType)
		{
			auto velocity = to - position;
			auto absx = std::abs(velocity.x);
			velocity.x /= std::max(1, absx);
			auto absy = std::abs(velocity.y);
			velocity.y /= std::max(1, absy);
			if (velocity.x != 0 && velocity.y != 0)
			{
				// Determine sprite direction
				// If within +-22.5 degrees of an axis, which has a tan of ~0.414,
				// we use four-directional sprite, else diagonal sprite
				if ((float)std::min(absx, absy) / std::max(absx, absy) < 0.414)
				{
					if (absx > absy)
						velocity.y = 0;
					else
						velocity.x = 0;
				}
			}
			doodadType = {&state,
			              format("DOODAD_BATTLE_EXPLOSION_%d%d", velocity.x + 1, velocity.y + 1)};
		}
		Vec3<float> doodadPos = to;
		doodadPos += Vec3<float>{0.5f, 0.5f, 0.5f};
		state.current_battle->placeDoodad(doodadType, doodadPos);
	}
}

void BattleExplosion::grow(GameState &state)
{
	auto &map = *state.current_battle->map;

	state.current_battle->notifyAction(position);

	for (int i = 0; i < 2; i++)
	{
		locationsToExpand.emplace_back();
		// Deal damage and expand in four straight directions
		for (auto &pos : locationsToExpand[0])
		{
			if (damageType->hazardType && damageType->explosionDoodad)
			{
				state.current_battle->placeHazard(
				    state, ownerOrganisation, ownerUnit, damageType, pos.first,
				    damageType->hazardType->getLifetime(state), pos.second.x);
			}
			if (damageInTheEnd)
			{
				locationsToDamage.emplace(pos.first, pos.second.x);
			}
			else
			{
				damage(state, map, pos.first, pos.second.x);
			}
			auto dir = pos.first - position;
			int minX = dir.x <= 0 ? -1 : 0;
			int maxX = dir.x >= 0 ? 1 : 0;
			int minY = dir.y <= 0 ? -1 : 0;
			int maxY = dir.y >= 0 ? 1 : 0;

			for (int x = minX; x <= maxX; x++)
			{
				expand(state, map, pos.first, {pos.first.x + x, pos.first.y, pos.first.z},
				       pos.second.y);
			}
			for (int y = minY; y <= maxY; y++)
			{
				expand(state, map, pos.first, {pos.first.x, pos.first.y + y, pos.first.z},
				       pos.second.y);
			}
		}
		// Expand in diagonal directions that were left, as well as vertically
		for (auto &pos : locationsToExpand[0])
		{
			auto dir = pos.first - position;
			int minX = dir.x <= 0 ? -1 : 0;
			int maxX = dir.x >= 0 ? 1 : 0;
			int minY = dir.y <= 0 ? -1 : 0;
			int maxY = dir.y >= 0 ? 1 : 0;

			for (int z = -1; z <= 1; z += 2)
			{
				expand(state, map, pos.first, {pos.first.x, pos.first.y, pos.first.z + z},
				       pos.second.y);
			}
			for (int x = minX; x <= maxX; x++)
			{
				for (int y = minY; y <= maxY; y++)
				{
					expand(state, map, pos.first, {pos.first.x + x, pos.first.y + y, pos.first.z},
					       pos.second.y);
				}
			}
		}
		// Current entry is processed now
		locationsToExpand.erase(locationsToExpand.begin());
	}
}

} // namespace OpenApoc

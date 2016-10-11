#include "game/state/battle/battleexplosion.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlehazard.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/rules/damage.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_battleitem.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "library/strings_format.h"
#include <cmath>

namespace OpenApoc
{
BattleExplosion::BattleExplosion(Vec3<int> position, StateRef<DamageType> damageType, int power,
                                 int depletionRate, bool damageInTheEnd,
                                 StateRef<BattleUnit> ownerUnit)
    : position(position), ticksUntilExpansion(TICKS_MULTIPLIER * 2),
      locationsToExpand({{position, {power, power}}}), locationsVisited({position}),
      damageType(damageType), depletionRate(depletionRate), damageInTheEnd(damageInTheEnd),
      ownerUnit(ownerUnit)
{
}

void BattleExplosion::die(GameState &state)
{
	if (damageInTheEnd)
	{
		auto &map = *state.current_battle->map;
		for (auto &pos : locationsToDamage)
		{
			damage(state, map, pos.first, pos.second);
		}
	}
	auto this_shared = shared_from_this();
	state.current_battle->explosions.erase(this_shared);
}

void BattleExplosion::update(GameState &state, unsigned int ticks)
{
	ticksUntilExpansion -= ticks;
	while (ticksUntilExpansion <= 0)
	{
		ticksUntilExpansion += TICKS_MULTIPLIER * 2;
		nextExpandLimited = !nextExpandLimited;
		grow(state);
		if (locationsToExpand.empty())
		{
			die(state);
			return;
		}
	}
}

void BattleExplosion::damage(GameState &state, const TileMap &map, Vec3<int> pos, int power)
{
	auto tile = map.getTile(pos);
	if (damageType->gas || damageType->flame)
	{
		// Spawn hazards
	}
	// Gas does no direct damage
	if (!damageType->gas)
	{
		for (auto obj : tile->ownedObjects)
		{
			if (obj->getType() == TileObject::Type::Ground ||
			    obj->getType() == TileObject::Type::Feature ||
			    obj->getType() == TileObject::Type::LeftWall ||
			    obj->getType() == TileObject::Type::RightWall)
			{
				auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(obj)->getOwner();
				if (damageType->flame)
				{
					LogWarning("Set map part on fire!");
				}
				else
				{
					mp->applyDamage(state, power, damageType);
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
				if (damageType->flame)
				{
					LogWarning("Set unit on fire!");
				}
				else
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
					// FIXME: Give experience
					u->applyDamage(state, power, damageType,
					               u->determineBodyPartHit(damageType, cposition, velocity));
				}
			}
			else if (obj->getType() == TileObject::Type::Item)
			{
				auto i = std::static_pointer_cast<TileObjectBattleItem>(obj)->getItem();
				i->applyDamage(state, power, damageType);
			}
		}
	}
}

void BattleExplosion::expand(GameState &state, const TileMap &map, const Vec3<int> &from,
                             const Vec3<int> &to, int power)
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
	    to.z >= map.size.z || power <= 0 || locationsVisited.find(to) != locationsVisited.end())
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
		for (auto obj : tile->ownedObjects)
		{
			if (pair.second.find(obj->getType()) != pair.second.end())
			{
				auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(obj)->getOwner();

				int depletion = 0;
				if (damageType->psionic)
					depletion = 2 * mp->type->block_psionic;
				else if (damageType->gas)
					depletion = 2 * mp->type->block_gas;
				else if (damageType->flame)
					depletion = 2 * mp->type->block_fire;
				else
					depletion = 2 * mp->type->block_physical;

				depletionNext = std::max(depletionNext, depletion);
				if (!(pos == to && obj->getType() == TileObject::Type::Feature) &&
				    !(dir.x == 0 && dir.y == 0 && obj->getType() == TileObject::Type::Ground))
				{
					depletionThis = std::max(depletionThis, depletion);
				}
			}
		}
	}
	int thisPower = power -
	                depletionRate *
	                    (1 + (dir.x != 0 ? 1 : 0) + (dir.y != 0 ? 1 : 0) + (dir.z != 0 ? 1 : 0)) /
	                    2;
	int nextPower = thisPower;
	nextPower -= depletionNext;
	thisPower -= depletionThis;
	if (thisPower > 0)
	{
		// Queue damage application
		locationsToExpand.emplace(to, Vec2<int>{thisPower, nextPower});
		// Spawn doodad
		auto doodadType = damageType->doodadType;
		if (damageType->explosive && !damageType->gas)
		{
			doodadType = {&state, format("DOODAD_BATTLE_EXPLOSION_%d%d", dir.x + 1, dir.y + 1)};
		}
		Vec3<float> doodadPos = to;
		doodadPos += Vec3<float>{0.5f, 0.5f, 0.5f};
		state.current_battle->placeDoodad(doodadType, doodadPos);
	}
}

void BattleExplosion::grow(GameState &state)
{
	std::set<std::pair<Vec3<int>, Vec2<int>>> locationsToExpandNow = locationsToExpand;
	locationsToExpand.clear();
	auto &map = *state.current_battle->map;

	// Deal damage and expand in four straight directions
	for (auto pos : locationsToExpandNow)
	{
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
	if (!nextExpandLimited)
	{
		for (auto pos : locationsToExpandNow)
		{
			auto dir = pos.first - position;
			int minX = dir.x <= 0 ? -1 : 0;
			int maxX = dir.x >= 0 ? 1 : 0;
			int minY = dir.y <= 0 ? -1 : 0;
			int maxY = dir.y >= 0 ? 1 : 0;

			for (int z = -1; z <= 1; z++)
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
	}
}

} // namespace OpenApoc

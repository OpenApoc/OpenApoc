#include "game/state/battle.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "game/state/battlemappart.h"
#include "game/state/battlemappart_type.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"
#include <functional>
#include <future>
#include <limits>
#include <unordered_map>

namespace OpenApoc
{
// An ordered list of the types drawn in each layer
// Within the same layer these are ordered by a calculated z based on the 'center' position
static std::vector<std::set<BattleTileObject::Type>> layerMap = {
    // Draw ground first, then put stuff on top of that
    {BattleTileObject::Type::Ground, BattleTileObject::Type::LeftWall,
     BattleTileObject::Type::RightWall, BattleTileObject::Type::Scenery},
    {},
};

Battle::~Battle()
{
	// For now, do nothing
}

void Battle::start()
{
	for (auto &pair : initial_grounds)
	{
		auto s = mksp<BattleMapPart>();

		s->type = pair.second;
		s->initialPosition = pair.first;
		s->currentPosition = s->initialPosition;

		map_parts.insert(s);
	}
	for (auto &pair : initial_left_walls)
	{
		auto s = mksp<BattleMapPart>();

		s->type = pair.second;
		s->initialPosition = pair.first;
		s->currentPosition = s->initialPosition;

		map_parts.insert(s);
	}
	for (auto &pair : initial_right_walls)
	{
		auto s = mksp<BattleMapPart>();

		s->type = pair.second;
		s->initialPosition = pair.first;
		s->currentPosition = s->initialPosition;

		map_parts.insert(s);
	}
	for (auto &pair : initial_scenery)
	{
		auto s = mksp<BattleMapPart>();

		s->type = pair.second;
		s->initialPosition = pair.first;
		s->currentPosition = s->initialPosition;

		map_parts.insert(s);
	}

	initMap();
}

void Battle::initMap()
{
	if (this->map)
	{
		LogError("Called on battle with existing map");
		return;
	}
	this->map.reset(new BattleTileMap(this->size, layerMap));
	for (auto &s : this->map_parts)
	{
		this->map->addObjectToMap(s);
	}
}

void Battle::update(GameState &state, unsigned int ticks)
{
	Trace::start("City::update::map_parts->update");
	for (auto &s : this->map_parts)
	{
		s->update(state, ticks);
	}
	Trace::end("City::update::ground->update");
}

} // namespace OpenApoc

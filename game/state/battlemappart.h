#pragma once
#include "framework/includes.h"
#include "game/state/stateobject.h"
#include "library/sp.h"

namespace OpenApoc
{
class Battle;
class Collision;
class TileObjectBattleMapPart;
class BattleMapPartType;
class BattleItem;

class BattleMapPart : public std::enable_shared_from_this<BattleMapPart>
{

  public:
	StateRef<BattleMapPartType> type;

	Vec3<float> getPosition() const { return currentPosition; }

	Vec3<int> initialPosition;
	Vec3<float> currentPosition;

	bool damaged = false;
	bool falling = false;
	bool destroyed = false;

	void handleCollision(GameState &state, Collision &c);

	void update(GameState &state, unsigned int ticks);
	void collapse(GameState &state);

	bool isAlive() const;

	sp<TileObjectBattleMapPart> tileObject;
	std::list<wp<BattleItem>> supportedItems;
	std::set<sp<BattleMapPart>> supportedParts;
	std::set<sp<BattleMapPart>> supportedBy;
	wp<Battle> battle;

	~BattleMapPart() = default;
};
}
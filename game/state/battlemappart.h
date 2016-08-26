#pragma once
#include "framework/includes.h"
#include "game/state/stateobject.h"
#include "library/sp.h"

namespace OpenApoc
{
class Battle;
class CollisionB;
class BattleTileObjectMapPart;
class BattleMapPartType;

class BattleMapPart : public std::enable_shared_from_this<BattleMapPart>
{

  public:
	StateRef<BattleMapPartType> type;

	Vec3<float> getPosition() const
	{
		// The "position" is the center, so offset by {0.5,0.5,0.5}
		Vec3<float> offsetPos = currentPosition;
		offsetPos += Vec3<float>{0.5, 0.5, 0.5};
		return offsetPos;
	}

	Vec3<int> initialPosition;
	Vec3<float> currentPosition;

	bool damaged;
	bool falling;
	bool destroyed;

	void handleCollision(GameState &state, CollisionB &c);

	void update(GameState &state, unsigned int ticks);
	void collapse(GameState &state);

	bool isAlive() const;

	sp<BattleTileObjectMapPart> tileObject;
	std::set<sp<BattleMapPart>> supports;
	std::set<sp<BattleMapPart>> supportedBy;
	StateRef<Battle> battle;

	BattleMapPart();
	~BattleMapPart() = default;
};
}
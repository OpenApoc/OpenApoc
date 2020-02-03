#pragma once

#include "game/state/battle/battle.h"
#include "game/state/gametime.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <vector>

#define MOTION_SCANNER_X 38
#define MOTION_SCANNER_Y 39

namespace OpenApoc
{

// How frequently are scanners updated, meaning, their dots "fade away"
// Makes sense to do 8 times a turn since that's 16 times per whole lifetime
// and we recognize 16 different colors
static const unsigned TICKS_PER_SCANNER_UPDATE = TICKS_PER_SECOND / 8;
// How long it takes for a dot to completely fade
static const unsigned TICKS_SCANNER_REMAIN_LIT = TICKS_PER_TURN / 2;

class BattleUnit;

class BattleScanner : public StateObject<BattleScanner>
{

  public:
	BattleScanner();

	// 2d map, amount of movement for each coordinate, stored in y * size.x + x order
	std::vector<int> movementTicks;
	// Updated every update, this indicates content has changed
	uint64_t version = 0;
	unsigned int updateTicksAccumulated = 0;
	Vec3<int> lastPosition;
	StateRef<BattleUnit> holder;

	void update(GameState &state, unsigned int ticks);
	void notifyMovement(Vec3<int> position);
};
} // namespace OpenApoc

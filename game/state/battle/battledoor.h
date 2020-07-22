#pragma once

#include "game/state/battle/battle.h"
#include "game/state/gametime.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <list>

namespace OpenApoc
{
static const unsigned TICKS_TO_STAY_OPEN = TICKS_PER_TURN;

class BattleMapPart;
class Battle;
class Sample;

class BattleDoor : public StateObject<BattleDoor>, public std::enable_shared_from_this<BattleDoor>
{
  public:
	UString id;

	// whether this door is still operational
	bool operational = false;
	bool right = false;
	// "Open" flag for doors
	bool open = false;
	void setDoorState(GameState &state, bool open);
	int animationFrameCount = 0;
	int openTicksRemaining = 0;
	// Amount of ticks until changing to open/closed state
	int animationTicksRemaining = 0;
	int getAnimationFrame();
	sp<Sample> doorSound;

	void update(GameState &state, unsigned int ticks);

	void playDoorSound();

	~BattleDoor() override = default;

	// Following members are not serialized, but rather are set in initBattle method

	std::list<wp<BattleMapPart>> mapParts;
	// Used to play sound at
	Vec3<float> position;
};
} // namespace OpenApoc

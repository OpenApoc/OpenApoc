#pragma once

#include "game/state/battle/battle.h"
#include "game/state/gametime.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"

#define HAZARD_FRAME_COUNT 3
// FIXME: This is a MADE UP VALUE!
// Study the algorithm of spreading the hazards
#define HAZARD_SPREAD_CHANCE 10 // out of 100

// -- Read about fire spread formula below the class declaration ---

namespace OpenApoc
{
static const unsigned TICKS_PER_HAZARD_UPDATE = TICKS_PER_TURN / 2;

class TileObjectBattleHazard;
class DamageType;
class HazardType;
class GameState;
class Organisation;
class BattleUnit;
class TileMap;

class BattleHazard : public std::enable_shared_from_this<BattleHazard>
{
  public:
	Vec3<float> getPosition() const { return this->position; }

	Vec3<float> position;
	StateRef<DamageType> damageType;
	StateRef<HazardType> hazardType;
	// Power (damage) for most hazards, growing/fading flag for fire
	int power = 0;
	// TTL for most hazards, Meaningless for fire
	unsigned lifetime = 0;
	// Time already lived for most hazards, stage for fire
	unsigned age = 0;
	unsigned int frame = 0;
	unsigned ticksUntilVisible = 0;
	unsigned frameChangeTicksAccumulated = 0;
	unsigned nextUpdateTicksAccumulated = 0;
	StateRef<Organisation> ownerOrganisation;
	StateRef<BattleUnit> ownerUnit;

	bool expand(GameState &state, const TileMap &map, const Vec3<int> &to, unsigned ttl,
	            bool fireSmoke = false);
	void grow(GameState &state);
	void applyEffect(GameState &state);
	void die(GameState &state, bool violently);
	void dieAndRemove(GameState &state, bool violently);
	void updateTileVisionBlock(GameState &state);

	// these return true if this hazard has died and needs to be deleted
	bool update(GameState &state, unsigned int ticks);
	bool updateInner(GameState &state, unsigned int ticks);
	bool updateTB(GameState &state);

	BattleHazard() = default;
	BattleHazard(GameState &state, StateRef<DamageType> damageType, bool delayVisibility = true);
	~BattleHazard() = default;

	// Following members are not serialized, but rather are set up in the initBattle method

	sp<TileObjectBattleHazard> tileObject;
};
} // namespace OpenApoc

/*
// Alexey Andronov (Istrebitel):

Okay, so I was trying to figure out how fire works, and I've studied videos and found out
that a very weird formula fits!

Fire in the game starts small, gradually enlarges, then rages for a bit, and then dies out.
We have 12 frames for fire. Let's number them 0 to 11 where 0 is full force fire.

Let "Stage" be the value that controls the fire's current state, and what frame we use.

When fire is applied (Incendiary missile or grenade), Stage = 10 - random[0-2] * 0,6
(here [] are inclusive brackets, as in widely accepted math notation)

Each 2 seconds Stage is decreased by 0,6 until it reaches 1.
After it reached one, each 2 seconds Stage is increased by 1 until i reaches 11.
After it reached 11, next time 2 seconds pass fire is extinguished completely.

What this gives us is a progression that looks like this:
    10 - 9,4 - 8,8 - 8,2 - 7,6 - 7,0 - ... 1,6 - 1 - 2 - 3 - ... - 10 - 11 - extinguished
   (^ start here ^)

Now, if we then round this value to nearest 0,5 we get progression that looks like this:
    10 - 9,5 - 9 - 8 - 7,5 - 7 - 6,5 ....

Now, if we add 0,5 and round up, or subtract 0,5 and round down, we get the possible frames
that the fire can show at every stage! One extra rule: frame 11 is reserved for dying flames

We get the following progression:

    10-9 - 10-9 - 10-8 - 9-7 - 8-7 - 8-6 - 7-6 - ...

Which is exactly how it appears to work in the game!

-- Fire spread --

Now, fire can spread to an adjacent flammable object (only feature or ground).
When it does, it starts burning from 10, as usual.
When it reaches past Stage 6 (value of 5,8) that's when object's "time to burn" (#9) timer starts
When this timer ends, object is destroyed.
Condition for spreading is the same - fire that reached past Stage 6 can spread.
At least for fire resist 25.
I don't know how it works for other resists so I will cheat and fake it

Assume 255 = immune
Otherwise "power" of current flame is 2 ^ (10 - Stage) * 3/2
Therefore:
- Stage 9 can penetrate 3
- Stage 8 can penetrate 6
- Stage 7 can penetrate 12
- Stage 6 can penetrate 24
- Stage 5 can penetrate 48
- Stage 4 can penetrate 96
- Stage 3 can penetrate 192
- Stage 2 can penetrate up to 254

Fire also never spreads to another fire

Smoke cannot extinguish fire that is burning a scenery, but can extinguish fire that is burning a
ground.
Walls are apparently immune to fire.

*/

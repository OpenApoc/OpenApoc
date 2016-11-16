#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>

namespace OpenApoc
{

class GameState;
class AEquipment;
class BattleUnit;

class AIAction
{
  public:
	enum class Type
	{
		Idle,
		AttackWeapon,
		AttackGrenade,
		AttackPsiPanic,
		AttackPsiStun,
		AttackPsiMC,
		Move,
		Retreat,
	};
	Type type = Type::Idle;
	// Who to attack
	sp<BattleUnit> target;
	// Where to move / where to throw
	Vec3<int> targetLocation;
	// What to fire / what to throw / what to use for PSI
	sp<AEquipment> item;
	// Other units participating
	std::list<StateRef<BattleUnit>> units;
	// Time to wait form until trying to think again
	unsigned ticks = 0;
	// Priority (used when choosing action)
	float priority = 0.0f;
	UString getName();
};

class AIState
{
  public:
	// Action type unit is executing
	AIAction::Type type = AIAction::Type::Idle;
	// Ticks delay until unit will think again (and possibly change it's course of action)
	unsigned ticksUntilThinkAgain = 0;
	// Wether unit should think again when ticks ran out, or wether unit should think again when
	// it's not busy
	bool waitForCompletion = false;
	// Wether action is offensive. Unit with non-offensive action will re-think immediately when
	// spotting enemies
	bool offensive = false;
};

enum class AIType
{
	None,     // Crysalises
	Civilian, //
	Loner,    // Poppers, Large units
	Group,    // Majority of the units
	Panic,    //
	Berserk,  //
};

class AI
{
  public:
	// Calculate AI's next action
	// If unit has group AI, and sees no enemies, the group will move together
	static AIAction think(GameState &state, BattleUnit &u);

	// Do the AI routine - organise inventory, stance etc.
	static void routine(GameState &state, BattleUnit &u);

	// Calculate AI's next action in case no enemies are seen
	static AIAction thinkGreen(GameState &state, std::list<StateRef<BattleUnit>> &units);
	// Calculate AI's next action in case enemies are seen
	static AIAction thinkRed(GameState &state, BattleUnit &u);

	static AIAction getWeaponAction(GameState &state, BattleUnit &u, sp<AEquipment> e,
	                                sp<BattleUnit> target);
	static AIAction getPsiAction(GameState &state, BattleUnit &u, sp<AEquipment> e,
	                             sp<BattleUnit> target, AIAction::Type type);
	static AIAction getGrenadeAction(GameState &state, BattleUnit &u, sp<AEquipment> e,
	                                 sp<BattleUnit> target);
	static AIAction getMoveAction(GameState &state, std::list<StateRef<BattleUnit>> &units);
	static AIAction getRetreatAction(GameState &state, std::list<StateRef<BattleUnit>> &units);
};
}
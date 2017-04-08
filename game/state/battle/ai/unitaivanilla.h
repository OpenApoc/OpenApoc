#pragma once
#include "game/state/battle/ai/unitai.h"

namespace OpenApoc
{

enum class PsiStatus;

// AI that handles vanilla alien and civilian behavior
class UnitAIVanilla : public UnitAI
{
  public:
	UnitAIVanilla();

	uint64_t ticksLastThink = 0;
	// Value of 0 means we will not re-think based on timer
	uint64_t ticksUntilReThink = 0;

	// What AI decided to do at last think()
	AIDecision lastDecision;

	// Was enemy ever visible since last think()
	bool enemySpotted = false;
	// Previous value, to identify when enemy was first spotted
	bool enemySpottedPrevious = false;
	// Relative position of a person who attacked us since last think()
	Vec3<int> attackerPosition = {0, 0, 0};
	// Relative position of last seen enemy's last seen position since last think()
	Vec3<int> lastSeenEnemyPosition = {0, 0, 0};

	void reset(GameState &state, BattleUnit &u) override;

	// Calculate AI's next decision, then do the routine
	// If unit has group AI, and patrol decision is made, the group will move together
	std::tuple<AIDecision, bool> think(GameState &state, BattleUnit &u, bool interrupt) override;

	void notifyUnderFire(Vec3<int> position) override;
	void notifyHit(Vec3<int> position) override;
	void notifyEnemySpotted(Vec3<int> position) override;

  private:
	AIDecision thinkInternal(GameState &state, BattleUnit &u);
	// Do the AI routine - organise inventory, move speed, stance etc.
	void routine(GameState &state, BattleUnit &u) override;

	// Calculate AI's next decision in case no enemy is engaged
	// Return values are decision, priority, ticks until re-think
	std::tuple<AIDecision, float, unsigned> thinkGreen(GameState &state, BattleUnit &u);
	// Calculate AI's next decision in case enemy is engaged (attacking)
	// Return values are decision, priority, ticks until re-think
	std::tuple<AIDecision, float, unsigned> thinkRed(GameState &state, BattleUnit &u);

	std::tuple<AIDecision, float, unsigned> getAttackDecision(GameState &state, BattleUnit &u);

	std::tuple<AIDecision, float, unsigned> getWeaponDecision(GameState &state, BattleUnit &u,
	                                                          sp<AEquipment> e,
	                                                          StateRef<BattleUnit> target);
	std::tuple<AIDecision, float, unsigned> getPsiDecision(GameState &state, BattleUnit &u,
	                                                       sp<AEquipment> e,
	                                                       StateRef<BattleUnit> target,
	                                                       PsiStatus status);
	std::tuple<AIDecision, float, unsigned> getGrenadeDecision(GameState &state, BattleUnit &u,
	                                                           sp<AEquipment> e,
	                                                           StateRef<BattleUnit> target);
	std::tuple<AIDecision, float, unsigned> getBrainsuckerDecision(GameState &state, BattleUnit &u);
	std::tuple<AIDecision, float, unsigned> getSuicideDecision(GameState &state, BattleUnit &u);
};
}
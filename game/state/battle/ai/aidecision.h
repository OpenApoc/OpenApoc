#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"

namespace OpenApoc
{

class AEquipment;
class BattleUnit;
enum class MovementMode;
enum class KneelingMode;
enum class WeaponStatus;

class AIAction
{
  public:
	enum class Type
	{
		AttackWeaponTile,
		AttackWeaponUnit,
		AttackGrenade,
		AttackPsiPanic,
		AttackPsiStun,
		AttackPsiMC,
		AttackBrainsucker,
		AttackSuicide,
	};
	AIAction();

	// Parameters that are stored for future reference

	// Action type (an attack usually)
	Type type = Type::AttackWeaponTile;
	// Who to attack
	StateRef<BattleUnit> targetUnit;
	// What location to attack
	Vec3<int> targetLocation = {0, 0, 0};
	// What to fire / what to throw / what to use for PSI
	// For simply attacking with all weapons in hands, this can be null
	sp<AEquipment> item;
	// If attacking with weapons, which hands to use
	// (init in constructor since it's undefined here)
	WeaponStatus weaponStatus;
	// Psi energy value, used to track whether unit psi attacked
	int psiEnergySnapshot = 0;
	// Prevent out of turn re-thinking
	bool preventOutOfTurnReThink = false;
	bool executed = false;

	// Methods

	UString getName();
	// Returns whether action is being undertaken by unit right now
	// Meaning the unit is doing it but it's not done
	// Also flags "executed" when it sees it undertaken for the first time
	bool inProgress(BattleUnit &u);
	// Returns whether action was in progress and is no longer
	bool isFinished(BattleUnit &u);

  private:
	bool inProgressInternal(BattleUnit &u);
};

class AIMovement
{
  public:
	enum class Type
	{
		// Stop moving
		Stop,
		// Turn to target location
		Turn,
		// Change stance
		ChangeStance,
		// Move to target location (patrol)
		Patrol,
		// Move to target location (advancing on enemy)
		Advance,
		// Move to target location (pursuing an enemy that left LOS)
		Pursue,
		// Move to target location (getting in range of fire/throw)
		GetInRange,
		// Move to target location (taking cover)
		TakeCover,
		// Move to target location (retreating)
		Retreat
	};
	AIMovement();
	AIMovement(const AIMovement &obj) = default;

	// Parameters that are stored for future reference

	// Movement type
	Type type = Type::Stop;
	// Where to move / face
	Vec3<int> targetLocation = {0, 0, 0};
	// Preferred movement speed (not used for Stop or Turn)
	// (init in constructor since it's undefined here)
	MovementMode movementMode;
	// Preferred kneeling state (not used for Stop or Turn)
	// (init in constructor since it's undefined here)
	KneelingMode kneelingMode;
	// Whether movement is only useful to achieve the action
	// Means, if action is finished, movement is finished too
	bool subordinate = false;
	bool executed = false;

	// Methods

	UString getName();
	// Returns whether action is being undertaken by unit right now
	// Meaning the unit is doing it but it's not done
	// Also flags "executed" when it sees it undertaken for the first time
	bool inProgress(BattleUnit &u);
	// Returns whether movement was in progress and is no longer
	bool isFinished(BattleUnit &u);

  private:
	bool inProgressInternal(BattleUnit &u);
};

class AIDecision
{
  public:
	AIDecision() = default;
	AIDecision(sp<AIAction> action, sp<AIMovement> movement);
	bool isEmpty();

	// Parameters that are stored for future reference

	UString ai; // AI that made the decision

	// Action to be taken (nullptr = carry on with previous action)
	sp<AIAction> action;

	// Movement to be done (nullptr = carry on with previous movement)
	sp<AIMovement> movement;

	// Methods

	UString getName();
};
} // namespace OpenApoc
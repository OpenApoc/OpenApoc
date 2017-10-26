#pragma once
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/state/battle/battleunit.h"
#include "game/state/city/vehicle.h"
#include "game/state/gametime.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>

// Based on the fact that retribution (tr = 10) turns 90 degrees (PI/2) per second
#define PROJECTILE_TURN_PER_TICK ((float)(M_PI / 2.0f) / 10.0f / TICKS_PER_SECOND)

// Alexey Andronov (Istrebitel):
// For some reason, advancing projectiles using their speed produces improper results
// With a multiplier of four, it seems to match.
// Tests with cityscape shown this to work similar to vanilla
#define PROJECTILE_VELOCITY_MULTIPLIER 4.0f

namespace OpenApoc
{

class GameState;
class TileObjectProjectile;
class TileMap;
class Collision;
class DoodadType;
class DamageType;
class Sample;
class Vehicle;
class BattleUnit;
class TileObject;
class VoxelMap;
class Image;

class Projectile : public std::enable_shared_from_this<Projectile>
{
  public:
	enum class Type
	{
		Beam,
		Missile,
	};

	// Beams have a width & tail length
	// FIXME: Make this a non-constant colour?
	// FIXME: Width is currently just used for drawing - TODO What is "collision" size of beams?
	Projectile(Type type, StateRef<Vehicle> firer, StateRef<Vehicle> target,
	           Vec3<float> targetPosition, Vec3<float> position, Vec3<float> velocity, int turnRate,
	           float lifetime, int damage, unsigned int delay, int depletionRate,
	           unsigned int tail_length, std::list<sp<Image>> projectile_sprites,
	           sp<Sample> impactSfx, StateRef<DoodadType> doodadType,
	           sp<VoxelMap> voxelMap = nullptr, int stunTicks = 0,
	           std::list<StateRef<VEquipmentType>> splitIntoTypes = {}, bool manualFire = false);
	Projectile(Type type, StateRef<BattleUnit> firer, StateRef<BattleUnit> target,
	           Vec3<float> targetPosition, Vec3<float> position, Vec3<float> velocity, int turnRate,
	           float lifetime, int damage, unsigned int delay, int depletionRate,
	           unsigned int tail_length, std::list<sp<Image>> projectile_sprites,
	           sp<Sample> impactSfx, StateRef<DoodadType> doodadType,
	           StateRef<DamageType> damageType, sp<VoxelMap> voxelMap = nullptr, int stunTicks = 0,
	           std::list<StateRef<AEquipmentType>> splitIntoTypes = {}, bool manualFire = false);
	Projectile();
	virtual void update(GameState &state, unsigned int ticks);
	void die(GameState &state, bool displayDoodad = true, bool playSound = true,
	         bool expired = false);

	Vec3<float> getPosition() const { return this->position; }

	Collision checkProjectileCollision(TileMap &map);

	virtual ~Projectile();

	sp<TileObjectProjectile> tileObject;

	Type type;
	Vec3<float> position;
	Vec3<float> velocity;
	int turnRate = 0;
	float age = 0.0f;
	float lifetime = 0.0f;
	int damage = 0;
	unsigned int delay_ticks_remaining = 0;
	int depletionRate = 0;
	StateRef<Vehicle> firerVehicle;
	StateRef<BattleUnit> firerUnit;
	Vec3<float> firerPosition;
	StateRef<Vehicle> trackedVehicle;
	StateRef<BattleUnit> trackedUnit;
	Vec3<float> targetPosition;
	Vec3<float> previousPosition;
	std::list<Vec3<float>> spritePositions;
	unsigned int tail_length = 0;
	std::list<sp<Image>> projectile_sprites;
	float sprite_distance = 0.0f;
	sp<VoxelMap> voxelMapLof;
	sp<VoxelMap> voxelMapLos;
	bool manualFire = false;

	sp<Sample> impactSfx;
	StateRef<DoodadType> doodadType;
	StateRef<DamageType> damageType;

	unsigned int ownerInvulnerableTicks = 0;

	Vec3<float> velocityScale;

	int stunTicks = 0;
	std::list<StateRef<VEquipmentType>> splitIntoTypesCity;
	std::list<StateRef<AEquipmentType>> splitIntoTypesBattle;

	friend class TileObjectProjectile;

	// Following members are not serialized, but rather are set in initState / initBattle method
	sp<TileObject> trackedObject;
};
}; // namespace OpenApoc

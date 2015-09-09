#pragma once

#include "library/vec.h"
#include "library/strings.h"

namespace OpenApoc
{

class WeaponDef
{
  private:
	WeaponDef() {}
	friend class RulesLoader;

  public:
	enum class Type
	{
		Unknown,
		Vehicle,
	};
	enum class ProjectileType
	{
		Unknown,
		Beam,
	};
	UString name;
	UString firingSFX;
	Type type;
	bool pointDefence;
	float arc;
	float range;
	float accuracy;
	float firingDelay;

	ProjectileType projectileType;
	float projectileDamage;
	float projectileTailLength;
	float projectileSpeed;
	float projectileTurnRate;
	UString hitSprite;

	float beamWidth;
	Colour beamColour;

	int ammoCapacity;
	int ammoPerShot;
	UString ammoType; // empty string for recharging
};

}; // namespace OpenApoc

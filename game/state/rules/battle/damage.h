#pragma once

#include "game/state/stateobject.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{
class Image;
class Sample;
class DoodadType;
class GameState;

class HazardType : public StateObject<HazardType>
{
  public:
	StateRef<DoodadType> doodadType;
	sp<Sample> sound;

	// Lifetime of a hazard
	// In TB, hazard damages and spreads once every turn end
	// In RT, it does so every 4 seconds

	unsigned minLifetime = 0;
	unsigned maxLifetime = 0;

	// Whether frames follow the logic of fire
	bool fire = false;

	// Get first frame used for age and offset
	sp<Image> getFrame(unsigned age, int offset);

	// Get a random lifetime for the hazard of this type
	int getLifetime(GameState &state);
};

class DamageModifier : public StateObject<DamageModifier>
{
  public:
	// nothing?
};

enum class DamageSource
{
	Impact,
	Hazard,
	Debuff
};

class DamageType : public StateObject<DamageType>
{
  public:
	enum class BlockType
	{
		Physical,
		Psionic,
		Gas,
		Fire
	};
	enum class EffectType
	{
		None,
		Stun,
		Smoke,
		Fire,
		Enzyme,
		Brainsucker
	};

	UString name;

	bool ignore_shield = false;
	// When causing unit death makes it non-violent
	bool non_violent = false;
	sp<Image> icon_sprite;
	std::map<StateRef<DamageModifier>, int> modifiers;

	// Whether this damage type produces explosion on hit
	bool explosive = false;
	// Doodad used by explosion, if nullptr will use common explosion doodad set
	StateRef<DoodadType> explosionDoodad;
	BlockType blockType = BlockType::Physical;
	EffectType effectType = EffectType::None;
	StateRef<HazardType> hazardType;

	// True if explosive damage should reduce with distance (gas deals full damage everywhere)
	bool hasDamageDissipation() const { return blockType != BlockType::Gas; }
	// True if this damage type deals damage on initial impact
	bool doesImpactDamage() const { return blockType != BlockType::Gas; }
	// True if this always impacts unit's head
	bool alwaysImpactsHead() const { return blockType == BlockType::Gas; }
	// True if this ignores armor value
	bool ignoresArmorValue() const { return effectType == EffectType::Smoke; }
	// True if this deals damage to armor
	bool dealsArmorDamage() const
	{
		return effectType != EffectType::Stun && effectType != EffectType::Smoke &&
		       effectType != EffectType::Fire;
	}
	// True if this damage type deals fatal wounds
	bool dealsFatalWounds() const
	{
		return effectType != EffectType::Stun && effectType != EffectType::Smoke;
	}
	// True if this deals stun damage instead of health damage
	bool dealsStunDamage() const
	{
		return effectType == EffectType::Stun || effectType == EffectType::Smoke;
	}

	// Sounds when this explodes
	std::list<sp<Sample>> explosionSounds;

	int dealDamage(int damage, StateRef<DamageModifier> modifier) const;
};
} // namespace OpenApoc

#pragma once
#include "game/state/stateobject.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{
class Image;
class Sample;
class DoodadType;
class DamageModifier : public StateObject<DamageModifier>
{
  public:
	UString id;
	UString name;
};

class DamageType : public StateObject<DamageType>
{
  public:
	UString id;
	UString name;
	bool ignore_shield = false;
	sp<Image> icon_sprite;
	std::map<StateRef<DamageModifier>, int> modifiers;

	// Wether this damage type produces explosion on hit
	bool explosive = false;
	// True = explosion will spawn gas and animate as such
	bool gas = false;
	// True = deals no health damage, but sets on fire
	// also, if explosion then will spawn file patches
	bool flame = false;
	// True = deals no health daamge, but stun damage instead, which cannot exceeed 2x power
	bool stun = false;
	// True = deals no health damage, but stun damage instead, which is fixed value of 2
	// also, blocks vision
	bool smoke = false;

	// stun lasts 1 to 2
	// smoke lasts 6 to 10
	// ag not tested yet, assume 1 to 2
	// clouds animate 1 neighbour sprite once in 2 seconds
	// when exploding clouds animate once from full to zero and then start lingering
	// explosion doodad is DOODAD_30_EXPLODING_PAYLOAD
	// In TB smoke and fire deals damage on turn ends and on first spawn

	// Doodad type spawned by this
	StateRef<DoodadType> doodadType;

	int dealDamage(int damage, StateRef<DamageModifier> modifier) const;
};
}

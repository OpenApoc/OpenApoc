#pragma once
#include "game/state/stateobject.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{
class Image;
class Sample;
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
};
}

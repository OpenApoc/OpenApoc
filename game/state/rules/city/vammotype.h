#pragma once

#include "game/state/stateobject.h"
#include "library/strings.h"

namespace OpenApoc
{

class Rules;
class Image;
class Sample;
class Organisation;

class VAmmoType : public StateObject<VAmmoType>
{
  public:
	VAmmoType() = default;

	~VAmmoType() override = default;

	UString id;
	UString name;

	unsigned ammo_id = 0;

	int weight = 0;
	int store_space = 0;
	StateRef<Organisation> manufacturer;
};

} // namespace OpenApoc

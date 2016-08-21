#pragma once
#include "game/state/organisation.h"
#include "game/state/stateobject.h"
#include "library/strings.h"
#include "library/vec.h"
#include <map>
#include <set>

namespace OpenApoc
{

class Rules;
class Image;
class Sample;
class VAmmoType : public StateObject<VAmmoType>
{
  public:
	VAmmoType();

	~VAmmoType() override = default;

	UString id;
	UString name;

	unsigned ammo_id;

	int weight;
	int store_space;
	StateRef<Organisation> manufacturer;
};

} // namespace OpenApoc

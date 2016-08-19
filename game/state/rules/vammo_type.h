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

	virtual ~VAmmoType() = default;

	UString id;
	UString name;

	// FIXME: Vanilla has no sprites for vehicle ammunition, should we introduce it?
	sp<Image> equipscreen_sprite;

	int weight;
	int store_space;
	StateRef<Organisation> manufacturer;
};

} // namespace OpenApoc

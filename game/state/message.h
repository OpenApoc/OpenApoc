#pragma once

#include "game/state/gametime.h"
#include "library/strings.h"
#include "library/vec.h"

namespace OpenApoc
{

class GameState;

class EventMessage
{
  public:
	static const Vec3<int> NO_LOCATION;

	GameTime time;
	UString text;
	UString location;

	Vec3<int> getMapLocation(GameState &state) const;
};

}; // namespace OpenApoc

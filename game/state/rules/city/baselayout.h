#pragma once

#include "game/state/stateobject.h"
#include "library/rect.h"
#include "library/vec.h"
#include <set>

namespace OpenApoc
{

class BaseLayout : public StateObject<BaseLayout>
{
  public:
	std::set<Rect<int>> baseCorridors;
	Vec2<int> baseLift = {0, 0};
};

}; // namespace OpenApoc

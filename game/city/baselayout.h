#pragma once
#include "library/sp.h"
#include "library/vec.h"
#include "library/rect.h"
#include "game/stateobject.h"

#include <set>

namespace OpenApoc
{

class BaseLayout : public StateObject<BaseLayout>
{
  public:
	std::set<Rect<int>> baseCorridors;
	Vec2<int> baseLift;
};

}; // namespace OpenApoc

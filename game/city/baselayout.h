#pragma once
#include "game/stateobject.h"
#include "library/rect.h"
#include "library/sp.h"
#include "library/vec.h"

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

#pragma once

#include "library/strings.h"
#include "library/rect.h"
#include "library/vec.h"
#include <vector>

namespace OpenApoc
{
class UString;
class Base;

class BuildingDef
{
  private:
	BuildingDef() {}
	UString name;
	Rect<int> bounds;
	UString ownerName;
	std::vector<Rect<int>> baseCorridors;
	Vec2<int> baseLift;
	friend class RulesLoader;

  public:
	const UString &getName() const { return this->name; }
	const Rect<int> &getBounds() const { return this->bounds; }
	const UString &getOwnerName() const { return this->ownerName; }
	const std::vector<Rect<int>> &getBaseCorridors() const { return this->baseCorridors; }
	const Vec2<int> &getBaseLift() const { return this->baseLift; }
};
};

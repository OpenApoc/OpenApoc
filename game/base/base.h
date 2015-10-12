#pragma once

#include "library/strings.h"
#include "library/vec.h"
#include "game/base/facility.h"

namespace OpenApoc
{

class Building;
class FacilityDef;

class Base
{
  private:
	std::vector<Facility> facilities;
  public:
	static const int SIZE = 8;
	enum class BuildError { None, Occupied, OutOfBounds, NoMoney };

	const Building &building;
	UString name;

	Base(const Building &building);

	const Facility *getFacility(Vec2<int> pos) const;
	const std::vector<Facility> &getFacilities() const;
	BuildError canBuildFacility(const FacilityDef& def, Vec2<int> pos) const;
	void buildFacility(const FacilityDef &def, Vec2<int> pos);
	BuildError canDestroyFacility(Vec2<int> pos) const;
	void destroyFacility(Vec2<int> pos);
};

}; // namespace OpenApoc

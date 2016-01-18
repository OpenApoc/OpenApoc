#pragma once
#include "library/strings.h"

namespace OpenApoc
{
class RulesLoader;
class FacilityDef
{
  private:
	FacilityDef()
	    : fixed(false), buildCost(0), buildTime(0), weeklyCost(0), capacityType(Capacity::Nothing),
	      capacityAmount(0), size(1)
	{
	}
	friend class RulesLoader;

  public:
	enum class Capacity
	{
		Nothing,
		Quarters,
		Stores,
		Medical,
		Training,
		Psi,
		Repair,
		Chemistry,
		Physics,
		Workshop,
		Aliens,
	};

	UString id;
	UString name;
	bool fixed;
	int buildCost;
	int buildTime;
	int weeklyCost;
	Capacity capacityType;
	int capacityAmount;
	int size;
	UString sprite;
};
};

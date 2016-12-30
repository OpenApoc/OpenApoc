#pragma once

#include "game/state/research.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{
class Image;
class UfopaediaEntry;
class FacilityType : public StateObject
{
	STATE_OBJECT(FacilityType)
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
		Aliens
	};
	FacilityType();
	UString name;
	bool fixed;
	int buildCost;
	int buildTime;
	int weeklyCost;
	Capacity capacityType;
	int capacityAmount;
	int size;
	sp<Image> sprite;
	ResearchDependency dependency;
	StateRef<UfopaediaEntry> ufopaedia_entry;
	bool isVisible() const;
};
};

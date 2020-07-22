#pragma once

#include "game/state/city/research.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{
class Image;
class UfopaediaEntry;
class BattleMapSector;
class FacilityType : public StateObject<FacilityType>
{
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

	// Facility cannot be built over (concrete, lift)
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
	int sector = 0;

	bool isVisible() const;
};
}; // namespace OpenApoc

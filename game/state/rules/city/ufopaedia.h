#pragma once

#include "game/state/city/research.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{

class ResearchTopic;
class LazyImage;

class UfopaediaEntry : public StateObject<UfopaediaEntry>
{
  public:
	enum class Data
	{
		Nothing,
		Organisation,
		Vehicle,
		VehicleEquipment,
		Equipment,
		Facility,
		Building
	};
	UfopaediaEntry();
	UString title;
	UString description;
	sp<LazyImage> background;
	// The ID of the 'dynamic' data shown with this entry (income/balance for organisations, stats
	// for weapons etc.)
	UString data_id;
	Data data_type;
	ResearchDependency dependency;
	bool isVisible() const;
};

class UfopaediaCategory : public StateObject<UfopaediaCategory>
{
  public:
	UString title;
	UString description;
	sp<LazyImage> background;
	StateRefMap<UfopaediaEntry> entries;
};

} // namespace OpenApoc

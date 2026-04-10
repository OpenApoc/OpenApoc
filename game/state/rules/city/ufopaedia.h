#pragma once

#include "game/state/city/research.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include <list>

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
	// Ordered list of entry references. Entries themselves are owned by
	// GameState::ufopaedia_entries; this list just tracks category membership
	// and display order.
	std::list<StateRef<UfopaediaEntry>> entries;
};

} // namespace OpenApoc

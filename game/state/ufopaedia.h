#pragma once

#include "framework/image.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{

class ResearchTopic;

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
	static const std::map<Data, UString> DataMap;
	UfopaediaEntry();
	UString title;
	UString description;
	sp<Image> background;
	// The ID of the 'dynamic' data shown with this entry (income/balance for organisations, stats
	// for weapons etc.)
	UString data_id;
	Data data_type;
	StateRef<ResearchTopic> required_research;
	bool isVisible() const;
};

class UfopaediaCategory
{
  public:
	UString title;
	UString description;
	sp<Image> background;
	std::map<UString, sp<UfopaediaEntry>> entries;
};

} // namespace OpenApoc

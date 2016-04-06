#pragma once

#include "framework/image.h"
#include "game/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{

class ResearchTopic;

class UfopaediaEntry : public StateObject<UfopaediaEntry>
{
  public:
	UString title;
	UString description;
	sp<Image> background;
	// The ID of the 'dynamic' data shown with this entry (income/balance for organisations, stats
	// for weapons etc.)
	UString data_id;
	StateRef<ResearchTopic> required_research;
	const bool isVisible() const;
};

class UfopaediaCategory
{
  public:
	UString title;
	UString description;
	sp<Image> background;
	std::map<UString, sp<UfopaediaEntry>> entries;
	// FIXME: Add an enum or something for the 'type' of data to show the dynamic extra stuff at the
	// side? (w/ UfopaediaEntry::data_id to select the specific object)
};

} // namespace OpenApoc

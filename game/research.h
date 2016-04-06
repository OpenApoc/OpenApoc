#pragma once

#include "game/stateobject.h"
#include <map>

namespace OpenApoc
{

class UfopaediaEntry;

class ResearchTopic : public StateObject<ResearchTopic>
{
  public:
	ResearchTopic();
	enum class Type
	{
		BioChem,
		Physics,
	};
	static const std::map<Type, UString> TypeMap;
	enum class LabSize
	{
		Small,
		Large,
	};
	static const std::map<LabSize, UString> LabSizeMap;
	UString name;
	UString description;
	StateRef<UfopaediaEntry> ufopaedia_entry;
	unsigned man_hours;
	unsigned man_hours_progress;
	Type type;
	LabSize required_lab_size;

	unsigned score;

	const bool isComplete() const;
};

} // namespace OpenApoc

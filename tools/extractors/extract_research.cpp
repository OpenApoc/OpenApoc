#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"

#include "framework/framework.h"
#include "game/city/baselayout.h"

namespace OpenApoc
{

void InitialGameStateExtractor::extractResearch(GameState &state, Difficulty difficulty)
{
	auto &data = this->ufo2p;
	for (int i = 0; i < data.research_data->count(); i++)
	{
		auto rdata = data.research_data->get(i);

		auto r = mksp<ResearchTopic>();

		r->name = data.research_names->get(i);
		auto id = ResearchTopic::getPrefix() + canon_string(r->name);
		r->description = data.research_descriptions->get(i);
		r->ufopaedia_entry = "";
		r->man_hours = rdata.skillHours;
		r->man_hours_progress = 0;
		switch (rdata.researchGroup)
		{
			case 0:
				r->type = ResearchTopic::Type::BioChem;
				break;
			case 1:
				r->type = ResearchTopic::Type::Physics;
				break;
			default:
				LogError("Unexpected researchGroup 0x%02x for research item %s",
				         (unsigned)rdata.researchGroup, id.c_str());
		}
		switch (rdata.labSize)
		{
			case 0:
				r->required_lab_size = ResearchTopic::LabSize::Small;
				break;
			case 1:
				r->required_lab_size = ResearchTopic::LabSize::Large;
				break;
			default:
				LogError("Unexpected labSize 0x%02x for research item %s", (unsigned)rdata.labSize,
				         id.c_str());
		}
		r->score = rdata.score;

		if (state.research.find(id) != state.research.end())
		{
			LogError("Multiple research topics with ID \"%s\"", id.c_str());
		}
		state.research[id] = r;
// FIXME: The ufopaedia entries here don't seem to directly map to the IDs we're currently using?
// May also be a many:1 ratio (e.g. the "alien gas" research topic unlocks multiple ufopaedia
// entries) making this more complex
#if 0

		auto ufopaediaEntryID = "PAEDIAENTRY_" + canon_string(r->name);
		auto ufopaediaCatID =
		    "PAEDIACATEGORY_" + canon_string(data.ufopaedia_group->get(rdata.ufopaediaGroup));
		auto paediaCat = state.ufopaedia[ufopaediaCatID];
		if (!paediaCat)
		{
			state.ufopaedia[ufopaediaCatID] = mksp<UfopaediaCategory>();
			paediaCat = state.ufopaedia[ufopaediaCatID];
		}
		auto paediaEntry = paediaCat->entries[ufopaediaEntryID];
		if (!paediaEntry)
		{
			paediaCat->entries[ufopaediaEntryID] = mksp<UfopaediaEntry>();
			paediaEntry = paediaCat->entries[ufopaediaEntryID];
		}
		if (paediaEntry->required_research)
		{
			LogError("Multiple required research for UFOPaedia topic \"%s\" - \"%s\" and \"%s\"",
			         ufopaediaEntryID.c_str(), r->name.c_str(),
			         paediaEntry->required_research->name.c_str());
		}
		paediaEntry->required_research = {&state, id};
#endif
	}
}

} // namespace OpenApoc

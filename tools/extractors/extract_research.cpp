#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/baselayout.h"
#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"

namespace OpenApoc
{

void InitialGameStateExtractor::extractResearch(GameState &state) const
{
	auto &data = this->ufo2p;
	for (unsigned i = 0; i < data.research_data->count(); i++)
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
				         (unsigned)rdata.researchGroup, id);
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
				         id);
		}
		// FIXME: this assumed all listed techs are required, which is not true for some topics
		// (It's possible that an unknown member in ResearchData marks this, or it's done
		// in-code)
		// This should be fixed up in the patch.

		ResearchDependency dependency;
		dependency.type = ResearchDependency::Type::All;

		for (int pre = 0; pre < 3; pre++)
		{

			if (rdata.prereqTech[pre] != 0xffff)
			{
				auto prereqId = ResearchTopic::getPrefix() +
				                canon_string(data.research_names->get(rdata.prereqTech[pre]));
				dependency.topics.emplace(StateRef<ResearchTopic>{&state, prereqId});
			}
		}

		r->dependencies.research.push_back(dependency);

		/*ItemDependency itemdep;
		itemdep.agentItemsRequired[{&state, "AEQUIPMENTTYPE_PSICLONE"}] = 1;
		r->dependencies.items.push_back(itemdep);*/

		r->score = rdata.score;

		if (state.research.topics.find(id) != state.research.topics.end())
		{
			LogError("Multiple research topics with ID \"%s\"", id);
		}
		state.research.topics[id] = r;
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
			         ufopaediaEntryID, r->name,
			         paediaEntry->required_research->name);
		}
		paediaEntry->required_research = {&state, id};
#endif
	}
}

} // namespace OpenApoc

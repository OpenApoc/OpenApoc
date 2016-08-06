#include "framework/framework.h"
#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"

namespace OpenApoc
{

void InitialGameStateExtractor::extractOrganisations(GameState &state, Difficulty)
{
	auto &data = this->ufo2p;
	LogInfo("Number of org strings: %zu", data.organisation_names->readStrings.size());

	for (int i = 0; i < data.organisation_names->count(); i++)
	{
		UString id = data.get_org_id(i);

		auto name = data.organisation_names->get(i);
		// FIXME: proper finances
		int balance = 5000;
		int income = 1000;

		if (id == "ORG_X-COM")
		{
			balance = 140000;
			income = 92000;
		}
		state.organisations[id] = std::make_shared<Organisation>(name, balance, income);
	}
	state.player = {&state, "ORG_X-COM"};
}

} // namespace OpenApoc

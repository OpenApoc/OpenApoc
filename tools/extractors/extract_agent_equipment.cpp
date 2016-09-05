#include "framework/framework.h"
#include "game/state/rules/aequipment_type.h"
#include "tools/extractors/common/tacp.h"
#include "tools/extractors/extractors.h"

namespace OpenApoc
{

void InitialGameStateExtractor::extractAgentEquipment(GameState &state, Difficulty)
{
	auto &data = this->tacp;
	auto &data_u = this->ufo2p;

	int equipment_count = 0;

	for (unsigned i = 0; i < data.agent_equipment->count(); i++)
	{
		auto e = mksp<AEquipmentType>();
		auto edata = data.agent_equipment->get(i);

		// Since we are reading strings from UFO2P.EXE, we'll be missing the last row
		// used for a bogus "Elerium Pod" duplicate
		// Fix this here for now, in future maybe remove alltogether as i think it's unused
		if (i == data.agent_equipment->count() - 1)
			e->name = data_u.agent_equipment_names->get(i - 2);
		else
			e->name = data_u.agent_equipment_names->get(i);
		UString id = UString::format("%s%s", AEquipmentType::getPrefix(), canon_string(e->name));

		e->id = id;

		e->equipscreen_sprite = fw().data->loadImage(UString::format(
		    "PCK:xcom3/ufodata/pequip.pck:xcom3/ufodata/pequip.tab:%d:xcom3/tacdata/tactical.pal",
		    (int)edata.sprite_idx));
		e->equipscreen_size = {edata.size_x, edata.size_y};

		e->manufacturer = {&state, data_u.getOrgId(edata.manufacturer)};

		state.agent_equipment[id] = e;
	}

	// Remember for the future, these names are not present in the exe, have to put them in manually
	// General 14 = Structure Probe
	// General 15 = Vortex Analyzer
}

} // namespace OpenApoc

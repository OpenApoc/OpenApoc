#pragma once

#include "game/state/rules/aequipment_type.h"
#include "tools/extractors/common/aequipment.h"
#include "tools/extractors/common/datachunk.h"
#include "tools/extractors/common/strtab.h"
#include <algorithm>
#include <memory>
#include <string>

namespace OpenApoc
{

class TACP
{
  public:
	TACP(std::string fileName = "XCOM3/TACEXE/TACP.EXE");

	std::unique_ptr<StrTab> damage_modifier_names;

	std::unique_ptr<DataChunk<AgentEquipmentData>> agent_equipment;
};

// TACP &getTACPData();

} // namespace OpenApoc

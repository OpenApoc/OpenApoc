#include "framework/data.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/city/economyinfo.h"
#include "game/state/gamestate.h"
#include "library/strings_format.h"
#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"

namespace OpenApoc
{

namespace
{
static std::vector<UString> vehicleAmmoNames = {

    {"VEQUIPMENTAMMOTYPE_FUSION_POWERFUEL"},
    {"VEQUIPMENTAMMOTYPE_ELERIUM_115"},
    {"VEQUIPMENTAMMOTYPE_ZORIUM"},
    {"VEQUIPMENTAMMOTYPE_MULTI_CANNON_ROUND"},
    {"VEQUIPMENTAMMOTYPE_JANITOR_MISSILE"},
    {"VEQUIPMENTAMMOTYPE_JUSTICE_MISSILE"},
    {"VEQUIPMENTAMMOTYPE_PROPHET_MISSILE"},
    {"VEQUIPMENTAMMOTYPE_RETRIBUTION_MISSILE"},
    {"VEQUIPMENTAMMOTYPE_DISRUPTOR_BOMB"},
    {"VEQUIPMENTAMMOTYPE_STASIS_BOMB"},
    {"VEQUIPMENTAMMOTYPE_DISRUPTOR_MULTI-BOMB"},
    {"VEQUIPMENTAMMOTYPE_REPEATER_40MM_CANNON_ROUND"},
    {"VEQUIPMENTAMMOTYPE_AIRGUARD_52MM_CANNON_ROUND"},
    {"VEQUIPMENTAMMOTYPE_GROUND_LAUNCHED_MISSILE"},
    {"VEQUIPMENTAMMOTYPE_AIR_DEFENSE_MISSILE"},
};
}

void InitialGameStateExtractor::extractEconomy(GameState &state) const
{
	auto &data = this->ufo2p;
	LogInfo("Number of economy 1 data chunks: {}", (unsigned)data.economy_data1->count());
	LogInfo("Number of economy 2 data chunks: {}", (unsigned)data.economy_data2->count());
	LogInfo("Number of economy 3 data chunks: {}", (unsigned)data.economy_data3->count());

	for (unsigned idx = 0; idx < data.economy_data1->count(); idx++)
	{
		int i = idx;
		auto e = data.economy_data1->get(i);

		auto economyInfo = EconomyInfo();
		economyInfo.weekAvailable = e.week;
		economyInfo.basePrice = e.basePrice;
		economyInfo.currentPrice = e.curPrice;
		economyInfo.currentStock = e.curStock;
		economyInfo.lastStock = e.lastStock;
		economyInfo.maxStock = e.maxStock;
		economyInfo.minStock = e.minStock;

		UString id = "";
		if (i < 34)
		{
			// Skip overspawn
			if (i == 33)
			{
				continue;
			}
			id = data.getVehicleId(i);
		}
		else
		{
			i -= 34;
			if (i < 49)
			{
				id = data.getVequipmentId(i);
			}
			else
			{
				LogError("Unexpected data in economy data pack 1!");
			}
		}
		state.economy[id] = economyInfo;
	}
	for (unsigned idx = 0; idx < data.economy_data2->count(); idx++)
	{
		int i = idx;
		auto e = data.economy_data2->get(i);

		auto economyInfo = EconomyInfo();
		economyInfo.weekAvailable = e.week;
		economyInfo.basePrice = e.basePrice;
		economyInfo.currentPrice = e.curPrice;
		economyInfo.currentStock = e.curStock;
		economyInfo.lastStock = e.lastStock;
		economyInfo.maxStock = e.maxStock;
		economyInfo.minStock = e.minStock;

		UString id = "";
		if (i < 15)
		{
			id = vehicleAmmoNames[i];
		}
		else
		{
			LogError("Unexpected data in economy data pack 2!");
		}
		state.economy[id] = economyInfo;
	}
	for (unsigned idx = 0; idx < data.economy_data3->count(); idx++)
	{
		int i = idx;
		auto e = data.economy_data3->get(i);

		auto economyInfo = EconomyInfo();
		economyInfo.weekAvailable = e.week;
		economyInfo.basePrice = e.basePrice;
		economyInfo.currentPrice = e.curPrice;
		economyInfo.currentStock = e.curStock;
		economyInfo.lastStock = e.lastStock;
		economyInfo.maxStock = e.maxStock;
		economyInfo.minStock = e.minStock;

		UString id = "";
		if (i < 87)
		{
			id = data.agent_equipment_names->get(i);
			id = fmt::format("{}{}", AEquipmentType::getPrefix(), canon_string(id));
		}
		else
		{
			LogError("Unexpected data in economy data pack 3!");
		}
		state.economy[id] = economyInfo;
	}
}

} // namespace OpenApoc

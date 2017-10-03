#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/state/ufopaedia.h"
#include "library/strings_format.h"
#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"

#define ORG_MEGAPOL 3
#define ORG_TRANSTELLAR 9
#define ORG_CIVILIAN 27

namespace OpenApoc
{

void InitialGameStateExtractor::extractOrganisations(GameState &state) const
{
	auto &data = this->ufo2p;
	LogInfo("Number of org strings: %zu", data.organisation_names->readStrings.size());
	LogInfo("Number of orgs: %u", (unsigned)data.organisation_data->count());

	// Organisations

	for (unsigned i = 0; i < data.organisation_data->count(); i++)
	{
		auto o = mksp<Organisation>();

		auto odata = data.organisation_data->get(i);
		auto rdata = data.organisation_starting_relationships_data->get(i);

		UString id = data.getOrgId(i);

		o->name = data.organisation_names->get(i);
		o->id = id;

		auto ped = format("%s%s", UfopaediaEntry::getPrefix(),
		                  canon_string(data.organisation_names->get(i)));
		o->ufopaedia_entry = {&state, ped};

		o->balance = odata.starting_funds;
		o->income = odata.starting_funding;
		o->tech_level = odata.starting_tech_level + 1;
		o->average_guards = odata.average_guards;

		// "Civilian" organisation has no loot entry
		if (i == ORG_CIVILIAN)
		{
			for (int k = 0; k < 3; k++)
			{
				Organisation::LootPriority priority;
				switch (k)
				{
					case 0:
						priority = Organisation::LootPriority::A;
						break;
					case 1:
						priority = Organisation::LootPriority::B;
						break;
					case 2:
						priority = Organisation::LootPriority::C;
						break;
				}
				for (int j = 0; j < 5; j++)
				{
					o->loot[priority].push_back(nullptr);
				}
			}
		}
		else
		{
			auto ldata = data.organisation_raid_loot_data->get(i);

			for (int k = 0; k < 3; k++)
			{
				Organisation::LootPriority priority;
				switch (k)
				{
					case 0:
						priority = Organisation::LootPriority::A;
						break;
					case 1:
						priority = Organisation::LootPriority::B;
						break;
					case 2:
						priority = Organisation::LootPriority::C;
						break;
				}
				for (int j = 0; j < 5; j++)
				{
					if (ldata.loot_idx[k][j] == 0)
					{
						o->loot[priority].push_back(nullptr);
					}
					else
					{
						o->loot[priority].emplace_back(
						    &state, format("%s%s", AEquipmentType::getPrefix(),
						                   canon_string(data.agent_equipment_names->get(
						                       ldata.loot_idx[k][j]))));
					}
				}
			}
		}

		// infiltration
		auto idata = data.infiltration_speed_org->get(i);
		o->infiltrationSpeed = idata.speed;

		// relationship
		for (int j = 0; j < 28; j++)
		{
			StateRef<Organisation> o2 = {&state, data.getOrgId(j)};

			o->current_relations[o2] = (float)rdata.relationships[j];
		}

		if (i != ORG_CIVILIAN)
		{
			auto vdata = data.vehicle_park->get(i);

			// Give all orgs some service vehicles
			o->vehiclePark[{&state, "VEHICLETYPE_CONSTRUCTION_VEHICLE"}] = 3;
			o->vehiclePark[{&state, "VEHICLETYPE_RESCUE_TRANSPORT"}] = 3;
			o->vehiclePark[{&state, "VEHICLETYPE_CIVILIAN_CAR"}] = 3;
			o->vehiclePark[{&state, "VEHICLETYPE_BLAZER_TURBO_BIKE"}] = 3;
			o->agentPark = 3;

			// Vehicle park
			switch (vdata.vehiclePark)
			{
				// No combat vehice park
				case 0:
					break;
				// Nutrivend, SuperDynamics, GeneralMetro
				case 2:
					o->vehiclePark[{&state, "VEHICLETYPE_PHOENIX_HOVERCAR"}] = 2;
					o->vehiclePark[{&state, "VEHICLETYPE_HOVERBIKE"}] = 1;
					break;
				// Transtellar
				case 3:
					if (i != ORG_TRANSTELLAR)
					{
						LogError("Modded game? Only Transtellar should have vehicle park of 3?");
					}
					o->providesTransportationServices = true;
					o->vehiclePark[{&state, "VEHICLETYPE_AIRTAXI"}] = 10;
					o->vehiclePark[{&state, "VEHICLETYPE_AIRTRANS"}] = 10;
					o->vehiclePark[{&state, "VEHICLETYPE_AUTOTAXI"}] = 10;
					o->vehiclePark[{&state, "VEHICLETYPE_AUTOTRANS"}] = 10;
					o->vehiclePark[{&state, "VEHICLETYPE_SPACE_LINER"}] = 6;
					// Combat vehicles
					o->vehiclePark[{&state, "VEHICLETYPE_PHOENIX_HOVERCAR"}] = 2;
					o->vehiclePark[{&state, "VEHICLETYPE_HOVERBIKE"}] = 1;
					o->vehiclePark[{&state, "VEHICLETYPE_VALKYRIE_INTERCEPTOR"}] = 1;
					break;
				// Miscellaneous (many orgs have this value)
				default:
					LogError("Modded game? Found unexpected vehiclePark value of %d",
					         (int)vdata.vehiclePark);
				case 4:
					o->vehiclePark[{&state, "VEHICLETYPE_PHOENIX_HOVERCAR"}] = 2;
					o->vehiclePark[{&state, "VEHICLETYPE_HOVERBIKE"}] = 1;
					o->vehiclePark[{&state, "VEHICLETYPE_VALKYRIE_INTERCEPTOR"}] = 1;
					o->vehiclePark[{&state, "VEHICLETYPE_HAWK_AIR_WARRIOR"}] = 1;
					break;
				// Technocrats Extopians Solmine Lifetree
				case 5:
					o->vehiclePark[{&state, "VEHICLETYPE_PHOENIX_HOVERCAR"}] = 3;
					o->vehiclePark[{&state, "VEHICLETYPE_HOVERBIKE"}] = 2;
					o->vehiclePark[{&state, "VEHICLETYPE_VALKYRIE_INTERCEPTOR"}] = 1;
					o->vehiclePark[{&state, "VEHICLETYPE_HAWK_AIR_WARRIOR"}] = 1;
					break;
				// Cyberweb
				case 6:
					o->vehiclePark[{&state, "VEHICLETYPE_PHOENIX_HOVERCAR"}] = 3;
					o->vehiclePark[{&state, "VEHICLETYPE_HOVERBIKE"}] = 2;
					o->vehiclePark[{&state, "VEHICLETYPE_VALKYRIE_INTERCEPTOR"}] = 2;
					o->vehiclePark[{&state, "VEHICLETYPE_HAWK_AIR_WARRIOR"}] = 1;
					break;
				// MSec
				case 10:
					o->vehiclePark[{&state, "VEHICLETYPE_PHOENIX_HOVERCAR"}] = 4;
					o->vehiclePark[{&state, "VEHICLETYPE_HOVERBIKE"}] = 2;
					o->vehiclePark[{&state, "VEHICLETYPE_VALKYRIE_INTERCEPTOR"}] = 3;
					o->vehiclePark[{&state, "VEHICLETYPE_HAWK_AIR_WARRIOR"}] = 3;
					break;
				// SELF / Mutant Alliance
				case 20:
					o->vehiclePark[{&state, "VEHICLETYPE_PHOENIX_HOVERCAR"}] = 8;
					o->vehiclePark[{&state, "VEHICLETYPE_HOVERBIKE"}] = 4;
					o->vehiclePark[{&state, "VEHICLETYPE_VALKYRIE_INTERCEPTOR"}] = 5;
					o->vehiclePark[{&state, "VEHICLETYPE_HAWK_AIR_WARRIOR"}] = 5;
					break;
				// Sirius
				case 30:
					o->vehiclePark[{&state, "VEHICLETYPE_PHOENIX_HOVERCAR"}] = 11;
					o->vehiclePark[{&state, "VEHICLETYPE_HOVERBIKE"}] = 5;
					o->vehiclePark[{&state, "VEHICLETYPE_VALKYRIE_INTERCEPTOR"}] = 8;
					o->vehiclePark[{&state, "VEHICLETYPE_HAWK_AIR_WARRIOR"}] = 7;
					break;
				// Megapol
				case 55:
					if (i != ORG_MEGAPOL)
					{
						LogError("Modded game? Only Megapol should have vehicle park of 55?");
					}
					o->vehiclePark[{&state, "VEHICLETYPE_POLICE_HOVERCAR"}] = 18;
					o->vehiclePark[{&state, "VEHICLETYPE_POLICE_CAR"}] = 15;
					o->vehiclePark[{&state, "VEHICLETYPE_GRIFFON_AFV"}] = 9;
					o->vehiclePark[{&state, "VEHICLETYPE_VALKYRIE_INTERCEPTOR"}] = 9;
					o->vehiclePark[{&state, "VEHICLETYPE_HAWK_AIR_WARRIOR"}] = 9;
					break;
				// Gangs
				case 60:
					o->vehiclePark[{&state, "VEHICLETYPE_PHOENIX_HOVERCAR"}] = 24;
					o->vehiclePark[{&state, "VEHICLETYPE_HOVERBIKE"}] = 12;
					o->vehiclePark[{&state, "VEHICLETYPE_VALKYRIE_INTERCEPTOR"}] = 15;
					o->vehiclePark[{&state, "VEHICLETYPE_HAWK_AIR_WARRIOR"}] = 15;
					break;
			}

			// Missions
			switch (vdata.vehiclePark)
			{
				// Government
				case 0:
					o->missionQueue.emplace_back((uint64_t)TICKS_PER_MINUTE,
					                             Organisation::MissionPattern{});
					break;
				// Transtellar
				case 3:
					break;
				// Most orgs
				case 2:
				case 4:
				case 5:
				case 6:
				case 10:
				case 30:
					break;
				// SELF / Mutant Alliance
				case 20:
					break;
				// Crime
				case 60:
					break;
				// Police
				case 55:
					break;
			}
		}
		state.organisations[id] = o;
	}
	state.player = {&state, "ORG_X-COM"};
	state.aliens = {&state, "ORG_ALIEN"};
	state.civilian = {&state, "ORG_CIVILIAN"};
}

} // namespace OpenApoc

#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/ufopaedia.h"
#include "library/strings_format.h"
#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"

#define ORG_XCOM 0
#define ORG_ALIENS 1
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
		o->rebuildingRate = odata.rebuilding_rate;

		// "Civilian" organisation has no loot entry and icon
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
			o->icon = fw().data->loadImage(format("PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/"
			                                      "vs_icon.tab:%d:xcom3/ufodata/pal_01.dat",
			                                      91 + i));

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

		// Done in common xml patch
		/*if (i == ORG_CIVILIAN)
		{
		    o->hirableAgentTypes[{&state, "AGENTTYPE_X-COM_QUANTUM_PHYSICIST"}] = {0,2};
		    o->hirableAgentTypes[{&state, "AGENTTYPE_X-COM_BIOCHEMIST"}] = { 0,2 };
		    o->hirableAgentTypes[{&state, "AGENTTYPE_X-COM_MECHANIC"}] = { 0,1 };
		}*/

		// Since we have 33% of agents going every day
		// And about 15 max agent pool spotted in vanilla
		// We should have average of 5 agents coming in every day
		// That would be average of 3 soldiers 1 android 1 hybrid and 5 each non-combat
		// That would be 4x 0-1 soldiers from some orgs and 0-2 from gravball, 0-2 from mutant/self,
		// and 10x 0-1 from orgs for bio,phys and engi (or more if there's less suitable orgs)

		// Park

		if (i != ORG_CIVILIAN && i != ORG_XCOM && i != ORG_ALIENS)
		{
			auto vdata = data.vehicle_park->get(i);

			// Give all orgs some service vehicles
			o->vehiclePark[{&state, "VEHICLETYPE_CONSTRUCTION_VEHICLE"}] = 3;
			o->vehiclePark[{&state, "VEHICLETYPE_RESCUE_TRANSPORT"}] = 3;
			o->vehiclePark[{&state, "VEHICLETYPE_CIVILIAN_CAR"}] = 3;
			o->vehiclePark[{&state, "VEHICLETYPE_BLAZER_TURBO_BIKE"}] = 3;

			// Vehicle park
			switch (vdata.vehiclePark)
			{
				// No combat vehicle park
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
			// Minute shortcut
			uint64_t m = (uint64_t)TICKS_PER_MINUTE;
			// Second shortcut
			uint64_t s = (uint64_t)TICKS_PER_SECOND;
			// Relation sets
			std::set<Organisation::Relation> Allied = {Organisation::Relation::Allied};
			std::set<Organisation::Relation> NeutralPlus = {Organisation::Relation::Allied,
			                                                Organisation::Relation::Friendly,
			                                                Organisation::Relation::Neutral};
			std::set<Organisation::Relation> UnfriendlyMinus = {Organisation::Relation::Unfriendly,
			                                                    Organisation::Relation::Hostile};

			auto &missions = o->missions[{&state, "CITYMAP_HUMAN"}];
			// Agents
			/*missions.emplace_back(m, 7 * m, 13 * m, 1, 1, std::set<StateRef<VehicleType>>{{}},
			                         Organisation::MissionPattern::Target::Other);*/
			switch (vdata.vehiclePark)
			{
				// Government
				case 0:
					missions.emplace_back(
					    0, 3 * m, 7 * m, 1, 1,
					    std::set<StateRef<VehicleType>>{{&state, "VEHICLETYPE_RESCUE_TRANSPORT"}},
					    Organisation::MissionPattern::Target::OwnedOrOther);
					missions.emplace_back(0, 3 * m, 7 * m, 1, 1,
					                      std::set<StateRef<VehicleType>>{
					                          {&state, "VEHICLETYPE_CONSTRUCTION_VEHICLE"}},
					                      Organisation::MissionPattern::Target::OwnedOrOther);
					missions.emplace_back(
					    0, 2 * m, 4 * m, 1, 1,
					    std::set<StateRef<VehicleType>>{{&state, "VEHICLETYPE_CIVILIAN_CAR"},
					                                    {&state, "VEHICLETYPE_BLAZER_TURBO_BIKE"}},
					    Organisation::MissionPattern::Target::OwnedOrOther);
					break;
				// Transtellar
				case 3:
					missions.emplace_back(
					    50 * s, 2 * m, 6 * m, 1, 1,
					    std::set<StateRef<VehicleType>>{{&state, "VEHICLETYPE_SPACE_LINER"}},
					    Organisation::MissionPattern::Target::DepartToSpace);
					missions.emplace_back(
					    22 * s, 2 * m, 6 * m, 1, 1,
					    std::set<StateRef<VehicleType>>{{&state, "VEHICLETYPE_SPACE_LINER"}},
					    Organisation::MissionPattern::Target::ArriveFromSpace);
					missions.emplace_back(
					    0, 5 * m, 11 * m, 1, 1,
					    std::set<StateRef<VehicleType>>{{&state, "VEHICLETYPE_AUTOTRANS"}},
					    Organisation::MissionPattern::Target::Other, NeutralPlus);
					missions.emplace_back(
					    0, 5 * m, 11 * m, 1, 3,
					    std::set<StateRef<VehicleType>>{{&state, "VEHICLETYPE_AIRRANS"}},
					    Organisation::MissionPattern::Target::Other, NeutralPlus);
					break;
				// Most orgs
				case 2:
				case 4:
				case 5:
				case 6:
				case 10:
				case 20:
					missions.emplace_back(
					    0, 15 * m, 25 * m, 1, 1,
					    std::set<StateRef<VehicleType>>{{&state, "VEHICLETYPE_CIVILIAN_CAR"},
					                                    {&state, "VEHICLETYPE_BLAZER_TURBO_BIKE"}},
					    Organisation::MissionPattern::Target::OwnedOrOther, Allied);
					break;
				// Sirius
				case 30:
					missions.emplace_back(
					    0, 7 * m, 13 * m, 1, 1,
					    std::set<StateRef<VehicleType>>{{&state, "VEHICLETYPE_CIVILIAN_CAR"},
					                                    {&state, "VEHICLETYPE_BLAZER_TURBO_BIKE"}},
					    Organisation::MissionPattern::Target::OwnedOrOther);
					break;
				// Crime
				case 60:
					missions.emplace_back(
					    5 * m, 11 * m, 19 * m, 1, 1,
					    std::set<StateRef<VehicleType>>{{&state, "VEHICLETYPE_CIVILIAN_CAR"},
					                                    {&state, "VEHICLETYPE_BLAZER_TURBO_BIKE"}},
					    Organisation::MissionPattern::Target::OwnedOrOther);
					break;
				// Police
				case 55:
					missions.emplace_back(
					    0, 20 * s, 40 * s, 1, 1,
					    std::set<StateRef<VehicleType>>{{&state, "VEHICLETYPE_POLICE_CAR"}},
					    Organisation::MissionPattern::Target::Owned);
					missions.emplace_back(
					    5 * m, 13 * m, 17 * m, 3, 5,
					    std::set<StateRef<VehicleType>>{{&state, "VEHICLETYPE_POLICE_CAR"}},
					    Organisation::MissionPattern::Target::Owned);
					missions.emplace_back(
					    3 * m, 30 * m, 90 * m, 3, 5,
					    std::set<StateRef<VehicleType>>{{&state, "VEHICLETYPE_POLICE_CAR"}},
					    Organisation::MissionPattern::Target::Other, UnfriendlyMinus);
					break;
			}
		}
		state.organisations[id] = o;
	}
	state.player = {&state, "ORG_X-COM"};
	state.aliens = {&state, "ORG_ALIEN"};
	state.civilian = {&state, "ORG_CIVILIAN"};
	state.government = {&state, "ORG_GOVERNMENT"};
}

} // namespace OpenApoc

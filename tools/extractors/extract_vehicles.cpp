#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemap.h"
#include "game/state/rules/city/ufopaedia.h"
#include "library/strings_format.h"
#include "library/voxel.h"
#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"
#include <cstring>
#include <list>
#include <map>
#include <set>

namespace OpenApoc
{
namespace
{
// XXX HACK - UFOs have different number of animation frames, I don't know any
//  link between this and stored data, hence a lookup table
std::map<UString, int> UFOAnimationFrames = {
    {"VEHICLETYPE_ALIEN_PROBE", 6},        {"VEHICLETYPE_ALIEN_SCOUT", 6},
    {"VEHICLETYPE_ALIEN_TRANSPORTER", 12}, {"VEHICLETYPE_ALIEN_FAST_ATTACK_SHIP", 12},
    {"VEHICLETYPE_ALIEN_DESTROYER", 12},   {"VEHICLETYPE_ALIEN_ASSAULT_SHIP", 12},
    {"VEHICLETYPE_ALIEN_BOMBER", 12},      {"VEHICLETYPE_ALIEN_ESCORT", 12},
    {"VEHICLETYPE_ALIEN_BATTLESHIP", 3},   {"VEHICLETYPE_ALIEN_MOTHERSHIP", 6},
};

// Magic lookup table for the small/large equipscreen icons
// no idea if this is stored in the .exe/data files somewhere...
std::map<UString, int> EquipscreenSprite = {{"VEHICLETYPE_ANNIHILATOR", 0},
                                            {"VEHICLETYPE_WOLFHOUND_APC", 1},
                                            {"VEHICLETYPE_BLAZER_TURBO_BIKE", 2},
                                            {"VEHICLETYPE_BIOTRANS", 3},
                                            {"VEHICLETYPE_VALKYRIE_INTERCEPTOR", 4},
                                            {"VEHICLETYPE_PHOENIX_HOVERCAR", 5},
                                            {"VEHICLETYPE_DIMENSION_PROBE", 6},
                                            {"VEHICLETYPE_RETALIATOR", 7},
                                            {"VEHICLETYPE_STORMDOG", 8},
                                            {"VEHICLETYPE_EXPLORER", 9},
                                            {"VEHICLETYPE_HOVERBIKE", 10},
                                            {"VEHICLETYPE_HAWK_AIR_WARRIOR", 11},
                                            {"VEHICLETYPE_GRIFFON_AFV", 12}};

std::set<UString> AgentFreight = {"VEHICLETYPE_AIRTAXI" /*, "VEHICLETYPE_AUTOTAXI"*/};
std::set<UString> CargoFreight = {"VEHICLETYPE_AIRTRANS" /*, "VEHICLETYPE_AUTOTRANS"*/};
std::set<UString> BioFreight = {"VEHICLETYPE_AIRTRANS"};
std::set<UString> Rescue = {"VEHICLETYPE_ANNIHILATOR",      "VEHICLETYPE_VALKYRIE_INTERCEPTOR",
                            "VEHICLETYPE_RETALIATOR",       "VEHICLETYPE_HAWK_AIR_WARRIOR",
                            "VEHICLETYPE_RESCUE_TRANSPORT", "VEHICLETYPE_CONSTRUCTION_VEHICLE"};
} // namespace
static void extract_equipment_layout(GameState &state, sp<VehicleType> vehicle, const UFO2P &data,
                                     VehicleEquipmentLayout layout,
                                     const uint8_t initial_equipment[45])
{
	if (layout.slot_count > 45)
	{
		LogError("Invalid equipment slot count %d", (int)layout.slot_count);
	}
	for (int i = 0; i < layout.slot_count; i++)
	{
		auto &slot = layout.slots[i];
		vehicle->equipment_layout_slots.emplace_back();
		auto &outSlot = vehicle->equipment_layout_slots.back();
		switch (slot.type)
		{
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_TYPE_ENGINE:
				outSlot.type = EquipmentSlotType::VehicleEngine;
				break;
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_TYPE_WEAPON:
				outSlot.type = EquipmentSlotType::VehicleWeapon;
				break;
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_TYPE_GENERAL:
				outSlot.type = EquipmentSlotType::VehicleGeneral;
				break;
			default:
				LogError("Invalid equipment slot type %d", (int)slot.type);
		}
		switch (slot.alignment_x)
		{
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_LEFT:
				outSlot.align_x = AlignmentX::Left;
				break;
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_CENTRE:
				outSlot.align_x = AlignmentX::Centre;
				break;
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_RIGHT:
				outSlot.align_x = AlignmentX::Right;
				break;
			default:
				LogError("Invalid equipment align_x type %d", (int)slot.alignment_x);
		}
		switch (slot.alignment_y)
		{
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_TOP:
				outSlot.align_y = AlignmentY::Top;
				break;
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_CENTRE:
				outSlot.align_y = AlignmentY::Centre;
				break;
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_BOTTOM:
				outSlot.align_y = AlignmentY::Bottom;
				break;
			default:
				LogError("Invalid equipment align_y type %d", (int)slot.alignment_x);
		}

		outSlot.bounds = {slot.position_x, slot.position_y, slot.position_x + slot.size_x,
		                  slot.position_y + slot.size_y};
		// 0 means empty, 0xff is used to say it's filled by another module (that is larger than
		// 1x1) but without it's origin in this slot
		if (initial_equipment[i] != 0 && initial_equipment[i] != 0xff)
		{
			vehicle->initial_equipment_list.emplace_back(
			    outSlot.bounds.p0,
			    StateRef<VEquipmentType>{&state, data.getVequipmentId(initial_equipment[i])});
		}
	}
}

void InitialGameStateExtractor::extractVehicles(GameState &state) const
{
	auto &data = this->ufo2p;
	LogInfo("Number of vehicle strings: %zu", data.vehicle_names->readStrings.size());

	for (unsigned i = 0; i < data.vehicle_data->count(); i++)
	{
		auto v = data.vehicle_data->get(i);

		UString id = data.getVehicleId(i);

		state.vehicle_types[id] = std::make_shared<VehicleType>();
		auto &vehicle = state.vehicle_types[id];
		auto name = data.vehicle_names->get(i);
		vehicle->name = name;
		vehicle->manufacturer = {&state, data.getOrgId(v.manufacturer)};
		// We draw objects based on centre, therefore we must adjust here
		vehicle->image_offset = {v.image_position_1, v.image_position_2 * 3.0f / 4.0f};

		auto ped =
		    format("%s%s", UfopaediaEntry::getPrefix(), canon_string(data.vehicle_names->get(i)));
		vehicle->ufopaedia_entry = {&state, ped};

		if (i < 10)
		{
			vehicle->researchUnlock.emplace_back(&state,
			                                     "RESEARCH_UNLOCK_ALIEN_CRAFT_CONTROL_SYSTEMS");
			vehicle->researchUnlock.emplace_back(&state,
			                                     "RESEARCH_UNLOCK_ALIEN_CRAFT_ENERGY_SOURCE");
			vehicle->researchUnlock.emplace_back(&state, "RESEARCH_UNLOCK_ALIEN_CRAFT_PROPULSION");
			vehicle->researchUnlock.emplace_back(&state,
			                                     format("RESEARCH_UNLOCK_UFO_TYPE_%d", i + 1));
		}

		if (v.movement_type == 0)
		{
			vehicle->type = VehicleType::Type::Road;
			int image_offset = 0;
			std::vector<VehicleType::Direction> directions = {
			    VehicleType::Direction::N,  VehicleType::Direction::NE, VehicleType::Direction::E,
			    VehicleType::Direction::SE, VehicleType::Direction::S,  VehicleType::Direction::SW,
			    VehicleType::Direction::W,  VehicleType::Direction::NW};
			auto bank = VehicleType::Banking::Flat;
			for (auto &dir : directions)
			{
				auto str = format("PCK:xcom3/ufodata/vehicle.pck:xcom3/ufodata/vehicle.tab:%d",
				                  (int)(v.graphic_frame + image_offset++));
				;
				vehicle->directional_sprites[bank][dir] = fw().data->loadImage(str);
			}
			// Ground vehicles don't have diagonal ascend/descend sprites (As roads don't seem to do
			// that)
			directions = {VehicleType::Direction::N, VehicleType::Direction::E,
			              VehicleType::Direction::S, VehicleType::Direction::W};
			std::vector<VehicleType::Banking> banking = {VehicleType::Banking::Ascending,
			                                             VehicleType::Banking::Descending};
			for (auto &bank : banking)
			{
				for (auto &dir : directions)
				{
					auto str = format("PCK:xcom3/ufodata/vehicle.pck:xcom3/ufodata/vehicle.tab:%d",
					                  (int)(v.graphic_frame + image_offset++));
					vehicle->directional_sprites[bank][dir] = fw().data->loadImage(str);
				}
			}
			vehicle->mapIconType = VehicleType::MapIconType::SmallCircle;
		}
		else if (v.movement_type == 1)
		{
			// X appears to be a 'size' - so the middle is div 2
			// FIXME: This Y doesn't seem to map to anything?
			// Might be due to us trying to use a 'center of mass'-style position while apoc vanilla
			// uses
			// 'bottom of tile' for vehicles?
			vehicle->shadow_offset = {v.shadow_position_1 / 2, 0};
			if (v.animation_type == 0)
			{
				vehicle->type = VehicleType::Type::UFO;
				if (v.size_x == 1 && v.size_y == 1)
				{
					vehicle->mapIconType = VehicleType::MapIconType::Arrow;
				}
				else if (v.size_x == 2 && v.size_y == 2)
				{
					// FIXME: Support 'large' strategy sprites?
					vehicle->mapIconType = VehicleType::MapIconType::LargeCircle;
#if 0
					for (int x = 0; x <= 1; x++)
					{
						for (int y = 0; y <= 1; y++)
						{
							std::stringstream ss;
							ss << "PCKSTRAT:xcom3/ufodata/stratmap.pck:xcom3/ufodata/stratmap.tab:"
							   << 573 + x + (2 * y);
							auto sprite = fw().data->loadImage(ss.str());
							vehicle->directional_stratecy_sprites[Vec3<float>{1,0,0}][x][y] = sprite;
						}
					}
#endif
				}
				else
				{
					LogError("Unknown vehicle size {%d,%d}", v.size_x, v.size_y);
				}
				int animFrames = UFOAnimationFrames[id];

				for (int i = 0; i < animFrames; i++)
				{
					auto str = format("PCK:xcom3/ufodata/saucer.pck:xcom3/ufodata/saucer.tab:%d",
					                  (int)(v.graphic_frame + i));
					vehicle->animation_sprites.push_back(fw().data->loadImage(str));
				}

				auto str = format("PCK:xcom3/ufodata/saucer.pck:xcom3/ufodata/saucer.tab:%d",
				                  (int)(v.graphic_frame + animFrames));

				vehicle->crashed_sprite = fw().data->loadImage(str);

				str = format("PCKSHADOW:xcom3/ufodata/shadow.pck:xcom3/ufodata/shadow.tab:%d",
				             (int)(v.shadow_graphic));
				vehicle->directional_shadow_sprites[VehicleType::Direction::N] =
				    fw().data->loadImage(str);

				// UFOs starting with trans (id = 2) up to mship (id = 9) have maps from 51 to 58
				// Therefore 49 + id gives map index for the ufo
				if (i > 1)
				{
					vehicle->battle_map = {&state, format("%s%s", BattleMap::getPrefix(),
					                                      this->battleMapPaths[48 + i])};
				}
				// fill crews
				UFO2P::fillCrew(state, ufo2p.crew_ufo_downed->get(i), vehicle->crew_downed);
				UFO2P::fillCrew(state, ufo2p.crew_ufo_deposit->get(i), vehicle->crew_deposit);
			}
			else
			{
				vehicle->type = VehicleType::Type::Flying;
				std::vector<VehicleType::Direction> directions = {
				    VehicleType::Direction::N, VehicleType::Direction::NE,
				    VehicleType::Direction::E, VehicleType::Direction::SE,
				    VehicleType::Direction::S, VehicleType::Direction::SW,
				    VehicleType::Direction::W, VehicleType::Direction::NW};
				// When banking there are more sprites (16 instead of 8) and seem to start in a
				// slightly different direction?
				std::vector<VehicleType::Direction> bankingDirectionsR = {
				    VehicleType::Direction::NWW, VehicleType::Direction::NW,
				    VehicleType::Direction::NNW, VehicleType::Direction::N,
				    VehicleType::Direction::NNE, VehicleType::Direction::NE,
				    VehicleType::Direction::NEE, VehicleType::Direction::E,
				    VehicleType::Direction::SEE, VehicleType::Direction::SE,
				    VehicleType::Direction::SSE, VehicleType::Direction::S,
				    VehicleType::Direction::SSW, VehicleType::Direction::SW,
				    VehicleType::Direction::SWW, VehicleType::Direction::W,
				};
				std::vector<VehicleType::Direction> bankingDirectionsL = {
				    VehicleType::Direction::SWW, VehicleType::Direction::SW,
				    VehicleType::Direction::SSW, VehicleType::Direction::S,
				    VehicleType::Direction::SSE, VehicleType::Direction::SE,
				    VehicleType::Direction::SEE, VehicleType::Direction::E,
				    VehicleType::Direction::NEE, VehicleType::Direction::NE,
				    VehicleType::Direction::NNE, VehicleType::Direction::N,
				    VehicleType::Direction::NNW, VehicleType::Direction::NW,
				    VehicleType::Direction::NWW, VehicleType::Direction::W,
				};
				std::vector<VehicleType::Banking> bankings = {
				    VehicleType::Banking::Flat,      VehicleType::Banking::Descending,
				    VehicleType::Banking::Ascending, VehicleType::Banking::Right,
				    VehicleType::Banking::Left,
				};

				vehicle->mapIconType = VehicleType::MapIconType::Arrow;

				int image_offset = 0;
				for (auto &bank : bankings)
				{
					auto directionsForThisBanking = directions;
					if (bank == VehicleType::Banking::Left)
						directionsForThisBanking = bankingDirectionsL;
					if (bank == VehicleType::Banking::Right)
						directionsForThisBanking = bankingDirectionsR;
					for (auto &dir : directionsForThisBanking)
					{
						auto str =
						    format("PCK:xcom3/ufodata/saucer.pck:xcom3/ufodata/saucer.tab:%d",
						           (int)(v.graphic_frame + image_offset++));
						vehicle->directional_sprites[bank][dir] = fw().data->loadImage(str);
					}
					// XXX HACK - The space liner doesn't have banking/ascending/descending images
					if (id == std::string("VEHICLETYPE_SPACE_LINER"))
						break;
				}

				// Shadows don't have banking
				image_offset = 0;
				for (auto &dir : directions)
				{
					auto str =
					    format("PCKSHADOW:xcom3/ufodata/shadow.pck:xcom3/ufodata/shadow.tab:%d",
					           (int)(v.shadow_graphic + image_offset++));
					vehicle->directional_shadow_sprites[dir] = fw().data->loadImage(str);
				}
			}
		}
		else
		{
			LogError("Unknown type for vehicle %s", id);
		}

		vehicle->acceleration = v.acceleration;
		vehicle->top_speed = v.top_speed;
		vehicle->health = v.constitution;
		vehicle->crash_health = v.crash_constitution;
		vehicle->weight = v.weight;
		vehicle->canEnterDimensionGate = (v.dimension_travel != 0);

		vehicle->armour[VehicleType::ArmourDirection::Rear] = v.armour_rear;
		vehicle->armour[VehicleType::ArmourDirection::Top] = v.armour_top;
		vehicle->armour[VehicleType::ArmourDirection::Left] = v.armour_left;
		vehicle->armour[VehicleType::ArmourDirection::Right] = v.armour_right;
		vehicle->armour[VehicleType::ArmourDirection::Bottom] = v.armour_bottom;
		vehicle->armour[VehicleType::ArmourDirection::Front] = v.armour_front;

		vehicle->passengers = v.passenger_capacity;
		vehicle->aggressiveness = v.manufacturer == 0 ? 1000 : v.aggressiveness;
		vehicle->score = v.score;

		vehicle->provideFreightAgent = AgentFreight.find(id) != AgentFreight.end();
		vehicle->provideFreightCargo = CargoFreight.find(id) != CargoFreight.end();
		vehicle->provideFreightBio = BioFreight.find(id) != BioFreight.end();
		vehicle->canRescueCrashed = Rescue.find(id) != Rescue.end();

		// The equipment_screen_name is up to 8 chars, any may have NULLs. Memcpy() that to a char[]
		// and add a trailing null (in case all 8 are used) and it's just like a c-string!
		// Then we can use the const char* std::string constructor to convert to something we can
		// append to
		char equipment_screen_filename[9];
		memcpy((void *)&equipment_screen_filename[0], (void *)&v.equipment_screen_name[0], 8);
		equipment_screen_filename[8] = '\0';
		std::string equipment_screen_image =
		    "xcom3/ufodata/" + to_lower(UString(equipment_screen_filename)) + ".pcx";
		// If it's all NULLs skip (as it might be an alien ship or something and therefore no
		// equipment screen)
		if (equipment_screen_filename[0] != '\0')
		{
			// Even some specified equipment screens don't exist - presumably for vehicles you can't
			// ever get (e.g. the 'airtaxi')
			auto img = fw().data->loadImage(equipment_screen_image);
			if (!img)
			{
				LogInfo("Skipping missing equipment screen image \"%s\"",
				        equipment_screen_image.c_str());
			}
			else
			{
				vehicle->equipment_screen = img;
			}
		}

		// The icons seem to be in index order starting at 4 (with 3 weird 'troop' icons and a
		// megaspawn first?)
		auto str = format(
		    "PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:%d:xcom3/ufodata/pal_01.dat",
		    i + 4);
		vehicle->icon = fw().data->loadImage(str);

		auto it = EquipscreenSprite.find(id);
		if (it != EquipscreenSprite.end())
		{
			auto str =
			    format("PCK:xcom3/ufodata/bigveh.pck:xcom3/ufodata/bigveh.tab:%d", (int)it->second);
			vehicle->equip_icon_big = fw().data->loadImage(str);

			str = format("PCK:xcom3/ufodata/smalveh.pck:xcom3/ufodata/smalveh.tab:%d:xcom3/ufodata/"
			             "researc2.pcx",
			             (int)it->second);
			vehicle->equip_icon_small = fw().data->loadImage(str);
		}

		extract_equipment_layout(state, vehicle, data,
		                         data.vehicle_equipment_layouts->get(v.equipment_layout),
		                         v.loaded_equipment_slots);

		// Alexey Andronov (Istrebitel)
		//
		// Loftemps... Okay, loftemps for vehicles, this gets weird.
		// Here's what I was able to figure out:
		//
		// Vehicles use loftemps 119, 120, 121, 122, 150 and 151.
		// I will present them in ASCII here for easier understanding.		Legend:
		//																	* border,
		//	119 and 120 :					121 :							- empty
		//	Look something like this.				     					X filled
		//  120 is a bit thinner.
		//
		//  ******************				******************
		//  *------XXXX------*				*XXXX--------XXXX*
		//  *------XXXX------*				*XXXXXXXXXXXXXXXX*
		//  *------XXXX------*				*-XXXXXXXXXXXXXX-*
		//  *-----XXXXXX-----*				*-XXXXXXXXXXXXXX-*
		//	*------XXXX------*				*-XXXXXXXXXXXXXX-*
		//	*------XXXX------*				*-XXXXXXXXXXXXXX-*
		//	*------XXXX------*				*XXXXXXXXXXXXXXXX*
		//	*-----XXXXXX-----*				*XXXX--------XXXX*
		//	******************				******************
		//
		//  122  :							150:
		//
		//  ******************				(64x64)
		//  *----------------*				(contains a full circle)
		//  *----------------*
		//  *----------------*				151:
		//  *-------------XXX*
		//	*-----------XXXXX*				(32x64, only top 32x32 filled)
		//	*---------XXXXXXX*				(contains small dot in the middle)
		//	*--------XXXXXXXX*
		//	*--------XXXXXXXX*
		//	******************
		//
		//  As we can see, we must do some adjustments to use them properly
		//  Especially since we only use static size loftemps (32x32x16)
		//
		//  1) 119 and 120 have 166 and 117 respectively for horizontal alignment
		//	   Unfortunately, there is no diagonal version
		//	2) 121 can be used without any adjustment as it's omnidirectional
		//	3) 122 is clearly a mistake. It's a part of 2x2 image.
		//	   If we assume that game does not actually use that 2x2 image, but
		//	   uses 122 where told to, we should replace 122 with a proper map of
		//	   of the similar size (that will make it act close to vanilla)
		//	   111 fits the most, so we will use it
		//	4) 150 can be substituted with 2x2 consisting of 93, 94, 96, 95
		//	   which form the same circle but are 32x32 in size each
		//	5) 151 is obviously a mistake, as Hoverbike is the only vehicle that uses it
		//	   and Hoverbike is 1x1x1 vehicle. We should just discard the bottom part
		//
		//	----------------------------------------------------------------
		//
		//  And now, a very IMPORTANT update!
		//
		//  After studying further, I found out that all of the above
		//  was on purpose! Misaligned voxelmaps are assigned to small vehicles on purpose.
		//  This is what makes them "dodge" shots. Apoc programmers decided to cheat and
		//  just give vehicles misaligned voxelmaps to make them "dodge" well.
		//  Therefore, to recreate vanilla behavior, we must do so as well.
		//
		//  - 122: should have 123, 124, 125 for right, bottom, left facings
		//  - 151: should discard bottom part, but attach a blank voxel map at the bottom
		//  (or left if facing right, etc.)
		//
		//	When we do this, we will encounter another problem: selecting units!
		//  Since some vehicles have misaligned lof maps, we will have problem selecting them
		//  Therefore, we must provide vehicles with second set of voxelmaps, for selecting
		//
		// - 119: use 109 (as 120)
		// - 120: use 109 (as 119)
		// - 121: use 110
		// - 122: use 111
		// - 150: can use the same voxelmap, as it fits properly
		// - 151: 126,127,128,129 are closest fitting loftemps, each is a box in the opposite side,
		//		  sides going north east south west (so for north box is located at southern edge)
		//		  therefore, north and south would use 128, 126 east and west would use 127, 129
		//        However, now that we can load loftemps from png files, we made proper files for
		//        this

		vehicle->height = v.loftemps_height;

		auto size = Vec3<int>{v.size_x, v.size_y, v.size_z};

		// read indexes
		int horizontalVoxelMapIndex = -1;
		int verticalVoxelMapIdx = v.loftemps_index;
		int losVoxelMapIndex = -1;
		switch (v.loftemps_index)
		{
			// bidirectional
			case 121:
				horizontalVoxelMapIndex = v.loftemps_index;
				losVoxelMapIndex = 110;
				break;
			// use 116 for horizontal direction
			case 119:
				horizontalVoxelMapIndex = 116;
				losVoxelMapIndex = 109;
				break;
			// use 117 for horizontal direction
			case 120:
				horizontalVoxelMapIndex = 117;
				losVoxelMapIndex = 109;
				break;
			// special case, treated separately below
			case 122: // four directions, use 122, 123, 124, 125
				losVoxelMapIndex = 111;
				break;
			// special cases, treated separately below
			case 150: // use 93 94 first row, 96 95 second row		| los same
			case 151: // use custom made ones
				losVoxelMapIndex = 111;
				break;
			default:
				LogError("Unsupported vehicle loftemps index %d!", (int)v.loftemps_index);
		}

		// read voxelmaps
		static const float FACING_NORTH = 0.0f;
		static const float FACING_EAST = M_PI / 2.0f;
		static const float FACING_SOUTH = M_PI;
		static const float FACING_WEST = M_PI * 1.5f;

		// Free space in voxelmap
		int freeSpace = v.size_z * 16 - v.loftemps_height;
		int start = (freeSpace + 1) / 2;
		int end = v.size_z * 16 - freeSpace / 2;
		if (end > 16)
		{
			LogInfo("Vehicle %s has height %d", vehicle->name, end);
			end = end % 16;
		}
		if (freeSpace > 32)
		{
			LogError(
			    "Modded game? too much free space in voxelmap, logic below won't work properly");
		}
		if (vehicle->type == VehicleType::Type::Road)
		{
			// Ground vehicles instead start at center
			start = v.size_z * 16 / 2;
			end = start + v.loftemps_height;
			if (end > v.size_z * 16)
			{
				LogError("Modded game? Too high ground vehicle");
			}
		}

		switch (v.loftemps_index)
		{
			// omnidirectional
			case 121:
				vehicle->size[FACING_NORTH] = size;
				vehicle->voxelMaps[FACING_NORTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_NORTH].x * vehicle->size[FACING_NORTH].y *
				    vehicle->size[FACING_NORTH].z);
				vehicle->voxelMapsLOS[FACING_NORTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_NORTH].x * vehicle->size[FACING_NORTH].y *
				    vehicle->size[FACING_NORTH].z);
				for (int x = 0; x < v.size_x; x++)
				{
					for (int y = 0; y < v.size_y; y++)
					{
						for (int z = 0; z < v.size_z; z++)
						{
							// One facing only
							vehicle->voxelMaps[FACING_NORTH]
							                  [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							vehicle->voxelMapsLOS[FACING_NORTH]
							                     [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							int locStart = z == 0 ? start : 0;
							int locEnd = z == v.size_z - 1 ? end : 16;
							for (int i = locStart; i < locEnd; i++)
							{
								vehicle
								    ->voxelMaps[FACING_NORTH]
								               [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      verticalVoxelMapIdx)));
								vehicle
								    ->voxelMapsLOS[FACING_NORTH]
								                  [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      losVoxelMapIndex)));
							}
						}
					}
				}
				break;
			// bidirectional
			case 119:
			case 120:
				vehicle->size[FACING_NORTH] = size;
				vehicle->voxelMaps[FACING_NORTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_NORTH].x * vehicle->size[FACING_NORTH].y *
				    vehicle->size[FACING_NORTH].z);
				vehicle->voxelMapsLOS[FACING_NORTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_NORTH].x * vehicle->size[FACING_NORTH].y *
				    vehicle->size[FACING_NORTH].z);
				vehicle->size[FACING_EAST] = size;
				vehicle->voxelMaps[FACING_EAST] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_EAST].x * vehicle->size[FACING_EAST].y *
				    vehicle->size[FACING_EAST].z);
				vehicle->voxelMapsLOS[FACING_EAST] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_EAST].x * vehicle->size[FACING_EAST].y *
				    vehicle->size[FACING_EAST].z);
				vehicle->size[FACING_SOUTH] = size;
				vehicle->voxelMaps[FACING_SOUTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_SOUTH].x * vehicle->size[FACING_SOUTH].y *
				    vehicle->size[FACING_SOUTH].z);
				vehicle->voxelMapsLOS[FACING_SOUTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_SOUTH].x * vehicle->size[FACING_SOUTH].y *
				    vehicle->size[FACING_SOUTH].z);
				vehicle->size[FACING_WEST] = size;
				vehicle->voxelMaps[FACING_WEST] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_WEST].x * vehicle->size[FACING_WEST].y *
				    vehicle->size[FACING_WEST].z);
				vehicle->voxelMapsLOS[FACING_WEST] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_WEST].x * vehicle->size[FACING_WEST].y *
				    vehicle->size[FACING_WEST].z);
				for (int x = 0; x < v.size_x; x++)
				{
					for (int y = 0; y < v.size_y; y++)
					{
						for (int z = 0; z < v.size_z; z++)
						{
							// Four facings
							// Facing north
							vehicle->voxelMaps[FACING_NORTH]
							                  [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							vehicle->voxelMapsLOS[FACING_NORTH]
							                     [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							// Facing east
							vehicle->voxelMaps[FACING_EAST]
							                  [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							vehicle->voxelMapsLOS[FACING_EAST]
							                     [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							// Facing south
							vehicle->voxelMaps[FACING_SOUTH]
							                  [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							vehicle->voxelMapsLOS[FACING_SOUTH]
							                     [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							// Facing west
							vehicle->voxelMaps[FACING_WEST]
							                  [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							vehicle->voxelMapsLOS[FACING_WEST]
							                     [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							int locStart = z == 0 ? start : 0;
							int locEnd = z == v.size_z - 1 ? end : 16;
							for (int i = locStart; i < locEnd; i++)
							{
								// Facing north
								vehicle
								    ->voxelMaps[FACING_NORTH]
								               [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      verticalVoxelMapIdx)));
								vehicle
								    ->voxelMapsLOS[FACING_NORTH]
								                  [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      losVoxelMapIndex)));
								// Facing east
								vehicle
								    ->voxelMaps[FACING_EAST]
								               [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      horizontalVoxelMapIndex)));
								vehicle
								    ->voxelMapsLOS[FACING_EAST]
								                  [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      losVoxelMapIndex)));
								// Facing south
								vehicle
								    ->voxelMaps[FACING_SOUTH]
								               [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      verticalVoxelMapIdx)));
								vehicle
								    ->voxelMapsLOS[FACING_SOUTH]
								                  [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      losVoxelMapIndex)));
								// Facing west
								vehicle
								    ->voxelMaps[FACING_WEST]
								               [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      horizontalVoxelMapIndex)));
								vehicle
								    ->voxelMapsLOS[FACING_WEST]
								                  [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      losVoxelMapIndex)));
							}
						}
					}
				}
				break;
			// four directions, use 122, 123, 124, 125
			case 122:
				vehicle->size[FACING_NORTH] = size;
				vehicle->voxelMaps[FACING_NORTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_NORTH].x * vehicle->size[FACING_NORTH].y *
				    vehicle->size[FACING_NORTH].z);
				vehicle->voxelMapsLOS[FACING_NORTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_NORTH].x * vehicle->size[FACING_NORTH].y *
				    vehicle->size[FACING_NORTH].z);
				vehicle->size[FACING_EAST] = size;
				vehicle->voxelMaps[FACING_EAST] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_EAST].x * vehicle->size[FACING_EAST].y *
				    vehicle->size[FACING_EAST].z);
				vehicle->voxelMapsLOS[FACING_EAST] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_EAST].x * vehicle->size[FACING_EAST].y *
				    vehicle->size[FACING_EAST].z);
				vehicle->size[FACING_SOUTH] = size;
				vehicle->voxelMaps[FACING_SOUTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_SOUTH].x * vehicle->size[FACING_SOUTH].y *
				    vehicle->size[FACING_SOUTH].z);
				vehicle->voxelMapsLOS[FACING_SOUTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_SOUTH].x * vehicle->size[FACING_SOUTH].y *
				    vehicle->size[FACING_SOUTH].z);
				vehicle->size[FACING_WEST] = size;
				vehicle->voxelMaps[FACING_WEST] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_WEST].x * vehicle->size[FACING_WEST].y *
				    vehicle->size[FACING_WEST].z);
				vehicle->voxelMapsLOS[FACING_WEST] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_WEST].x * vehicle->size[FACING_WEST].y *
				    vehicle->size[FACING_WEST].z);
				for (int x = 0; x < v.size_x; x++)
				{
					for (int y = 0; y < v.size_y; y++)
					{
						for (int z = 0; z < v.size_z; z++)
						{
							// Four facings
							// Facing north
							vehicle->voxelMaps[FACING_NORTH]
							                  [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							vehicle->voxelMapsLOS[FACING_NORTH]
							                     [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							// Facing east
							vehicle->voxelMaps[FACING_EAST]
							                  [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							vehicle->voxelMapsLOS[FACING_EAST]
							                     [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							// Facing south
							vehicle->voxelMaps[FACING_SOUTH]
							                  [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							vehicle->voxelMapsLOS[FACING_SOUTH]
							                     [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							// Facing west
							vehicle->voxelMaps[FACING_WEST]
							                  [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							vehicle->voxelMapsLOS[FACING_WEST]
							                     [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							int locStart = z == 0 ? start : 0;
							int locEnd = z == v.size_z - 1 ? end : 16;
							for (int i = locStart; i < locEnd; i++)
							{
								// Facing north
								vehicle
								    ->voxelMaps[FACING_NORTH]
								               [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      122)));
								vehicle
								    ->voxelMapsLOS[FACING_NORTH]
								                  [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      losVoxelMapIndex)));
								// Facing east
								vehicle
								    ->voxelMaps[FACING_EAST]
								               [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      123)));
								vehicle
								    ->voxelMapsLOS[FACING_EAST]
								                  [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      losVoxelMapIndex)));
								// Facing south
								vehicle
								    ->voxelMaps[FACING_SOUTH]
								               [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      124)));
								vehicle
								    ->voxelMapsLOS[FACING_SOUTH]
								                  [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      losVoxelMapIndex)));
								// Facing west
								vehicle
								    ->voxelMaps[FACING_WEST]
								               [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      125)));
								vehicle
								    ->voxelMapsLOS[FACING_WEST]
								                  [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      losVoxelMapIndex)));
							}
						}
					}
				}
				break;
			// use 93 94 first row, 96 95 second row
			case 150:
			{
				if (v.size_x != 2 || v.size_y != 2)
				{
					LogError("Vehicle Type using loftemps 150 has invalid x and y size: expected "
					         "2x2, got %dx%d",
					         v.size_x, v.size_y);
				}
				// One facing, four maps
				vehicle->size[FACING_NORTH] = size;
				vehicle->voxelMaps[FACING_NORTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_NORTH].x * vehicle->size[FACING_NORTH].y *
				    vehicle->size[FACING_NORTH].z);
				vehicle->voxelMapsLOS[FACING_NORTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_NORTH].x * vehicle->size[FACING_NORTH].y *
				    vehicle->size[FACING_NORTH].z);
				std::map<Vec2<int>, int> loftempsMap = {
				    {{0, 0}, 93}, {{1, 0}, 94}, {{0, 1}, 96}, {{1, 1}, 95}};
				for (int z = 0; z < v.size_z; z++)
				{
					for (auto &pair : loftempsMap)
					{
						vehicle->voxelMaps[FACING_NORTH][z * v.size_y * v.size_x +
						                                 pair.first.y * v.size_x + pair.first.x] =
						    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
						vehicle
						    ->voxelMapsLOS[FACING_NORTH][z * v.size_y * v.size_x +
						                                 pair.first.y * v.size_x + pair.first.x] =
						    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
						int locStart = z == 0 ? start : 0;
						int locEnd = z == v.size_z - 1 ? end : 16;
						for (int i = locStart; i < locEnd; i++)
						{
							vehicle
							    ->voxelMaps[FACING_NORTH][z * v.size_y * v.size_x +
							                              pair.first.y * v.size_x + pair.first.x]
							    ->setSlice(i, fw().data->loadVoxelSlice(format(
							                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
							                      "ufodata/loftemps.tab:%d",
							                      pair.second)));
							vehicle
							    ->voxelMapsLOS[FACING_NORTH][z * v.size_y * v.size_x +
							                                 pair.first.y * v.size_x + pair.first.x]
							    ->setSlice(i, fw().data->loadVoxelSlice(format(
							                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
							                      "ufodata/loftemps.tab:%d",
							                      pair.second)));
						}
					}
				}
				break;
			}
			// Custom voxelmap from png file
			case 151:
				vehicle->size[FACING_NORTH] = size;
				vehicle->voxelMaps[FACING_NORTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_NORTH].x * vehicle->size[FACING_NORTH].y *
				    vehicle->size[FACING_NORTH].z);
				vehicle->voxelMapsLOS[FACING_NORTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_NORTH].x * vehicle->size[FACING_NORTH].y *
				    vehicle->size[FACING_NORTH].z);
				vehicle->size[FACING_EAST] = size;
				vehicle->voxelMaps[FACING_EAST] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_EAST].x * vehicle->size[FACING_EAST].y *
				    vehicle->size[FACING_EAST].z);
				vehicle->voxelMapsLOS[FACING_EAST] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_EAST].x * vehicle->size[FACING_EAST].y *
				    vehicle->size[FACING_EAST].z);
				vehicle->size[FACING_SOUTH] = size;
				vehicle->voxelMaps[FACING_SOUTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_SOUTH].x * vehicle->size[FACING_SOUTH].y *
				    vehicle->size[FACING_SOUTH].z);
				vehicle->voxelMapsLOS[FACING_SOUTH] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_SOUTH].x * vehicle->size[FACING_SOUTH].y *
				    vehicle->size[FACING_SOUTH].z);
				vehicle->size[FACING_WEST] = size;
				vehicle->voxelMaps[FACING_WEST] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_WEST].x * vehicle->size[FACING_WEST].y *
				    vehicle->size[FACING_WEST].z);
				vehicle->voxelMapsLOS[FACING_WEST] = std::vector<sp<VoxelMap>>(
				    vehicle->size[FACING_WEST].x * vehicle->size[FACING_WEST].y *
				    vehicle->size[FACING_WEST].z);
				for (int x = 0; x < v.size_x; x++)
				{
					for (int y = 0; y < v.size_y; y++)
					{
						for (int z = 0; z < v.size_z; z++)
						{
							// Four facings
							// Facing north
							vehicle->voxelMaps[FACING_NORTH]
							                  [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							vehicle->voxelMapsLOS[FACING_NORTH]
							                     [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							// Facing east
							vehicle->voxelMaps[FACING_EAST]
							                  [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							vehicle->voxelMapsLOS[FACING_EAST]
							                     [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							// Facing south
							vehicle->voxelMaps[FACING_SOUTH]
							                  [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							vehicle->voxelMapsLOS[FACING_SOUTH]
							                     [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							// Facing west
							vehicle->voxelMaps[FACING_WEST]
							                  [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							vehicle->voxelMapsLOS[FACING_WEST]
							                     [z * v.size_y * v.size_x + y * v.size_x + x] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							int locStart = z == 0 ? start : 0;
							int locEnd = z == v.size_z - 1 ? end : 16;
							for (int i = locStart; i < locEnd; i++)
							{
								// Facing north
								vehicle
								    ->voxelMaps[FACING_NORTH]
								               [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i,
								               fw().data->loadVoxelSlice("city/loftemps-151n.png"));
								vehicle
								    ->voxelMapsLOS[FACING_NORTH]
								                  [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      losVoxelMapIndex)));
								// Facing east
								vehicle
								    ->voxelMaps[FACING_EAST]
								               [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i,
								               fw().data->loadVoxelSlice("city/loftemps-151e.png"));
								vehicle
								    ->voxelMapsLOS[FACING_EAST]
								                  [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      losVoxelMapIndex)));
								// Facing south
								vehicle
								    ->voxelMaps[FACING_SOUTH]
								               [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i,
								               fw().data->loadVoxelSlice("city/loftemps-151s.png"));
								vehicle
								    ->voxelMapsLOS[FACING_SOUTH]
								                  [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      losVoxelMapIndex)));
								// Facing west
								vehicle
								    ->voxelMaps[FACING_WEST]
								               [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i,
								               fw().data->loadVoxelSlice("city/loftemps-151w.png"));
								vehicle
								    ->voxelMapsLOS[FACING_WEST]
								                  [z * v.size_y * v.size_x + y * v.size_x + x]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      losVoxelMapIndex)));
							}
						}
					}
				}
				break;
		}
	}
}

} // namespace OpenApoc

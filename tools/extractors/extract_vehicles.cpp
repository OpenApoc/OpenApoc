#include "common/ufo2p.h"
#include "extractors.h"

#include "framework/framework.h"
#include "game/tileview/voxel.h"

#include <list>
#include <map>
#include <set>

namespace OpenApoc
{

// XXX HACK - UFOs have different number of animation frames, I don't know any
//  link between this and stored data, hence a lookup table
std::map<UString, int> UFOAnimationFrames = {
    {"VEHICLE_ALIEN_PROBE", 6},        {"VEHICLE_ALIEN_SCOUT", 6},
    {"VEHICLE_ALIEN_TRANSPORTER", 12}, {"VEHICLE_ALIEN_FAST_ATTACK_SHIP", 12},
    {"VEHICLE_ALIEN_DESTROYER", 12},   {"VEHICLE_ALIEN_ASSAULT_SHIP", 12},
    {"VEHICLE_ALIEN_BOMBER", 12},      {"VEHICLE_ALIEN_ESCORT", 12},
    {"VEHICLE_ALIEN_BATTLESHIP", 3},   {"VEHICLE_ALIEN_MOTHERSHIP", 6},
};

// Magic lookup table for the small/large equipscreen icons
// no idea if this is stored in the .exe/data files somewhere...
std::map<UString, int> EquipscreenSprite = {{"VEHICLE_ANNIHILATOR", 0},
                                            {"VEHICLE_WOLFHOUND_APC", 1},
                                            {"VEHICLE_BLAZER_TURBO_BIKE", 2},
                                            {"VEHICLE_BIOTRANS", 3},
                                            {"VEHICLE_VALKYRIE_INTERCEPTOR", 4},
                                            {"VEHICLE_PHOENIX_HOVERCAR", 5},
                                            {"VEHICLE_DIMENSION_PROBE", 6},
                                            {"VEHICLE_RETALIATOR", 7},
                                            {"VEHICLE_STORMDOG", 8},
                                            {"VEHICLE_EXPLORER", 9},
                                            {"VEHICLE_HOVERBIKE", 10},
                                            {"VEHICLE_HAWK_AIR_WARRIOR", 11},
                                            {"VEHICLE_GRIFFON_AFV", 12}};

static void extract_equipment_layout(GameState &state, sp<VehicleType> vehicle, UFO2P &data,
                                     vehicle_equipment_layout_t layout,
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
				outSlot.type = VEquipmentType::Type::Engine;
				break;
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_TYPE_WEAPON:
				outSlot.type = VEquipmentType::Type::Weapon;
				break;
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_TYPE_GENERAL:
				outSlot.type = VEquipmentType::Type::General;
				break;
			default:
				LogError("Invalid equipment slot type %d", (int)slot.type);
		}
		switch (slot.alignment_x)
		{
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_LEFT:
				outSlot.align_x = VehicleType::AlignmentX::Left;
				break;
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_CENTRE:
				outSlot.align_x = VehicleType::AlignmentX::Centre;
				break;
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_RIGHT:
				outSlot.align_x = VehicleType::AlignmentX::Right;
				break;
			default:
				LogError("Invalid equipment align_x type %d", (int)slot.alignment_x);
		}
		switch (slot.alignment_y)
		{
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_TOP:
				outSlot.align_y = VehicleType::AlignmentY::Top;
				break;
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_CENTRE:
				outSlot.align_y = VehicleType::AlignmentY::Centre;
				break;
			case VEHICLE_EQUIPMENT_LAYOUT_SLOT_ALIGN_BOTTOM:
				outSlot.align_y = VehicleType::AlignmentY::Bottom;
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
			    StateRef<VEquipmentType>{&state, data.get_vequipment_id(initial_equipment[i])});
		}
	}
}

void InitialGameStateExtractor::extractVehicles(GameState &state, Difficulty difficulty)
{
	auto &data = this->ufo2p;
	LogInfo("Number of vehicle strings: %zu", data.vehicle_names->readStrings.size());

	for (int i = 0; i < data.vehicle_data->count(); i++)
	{
		auto v = data.vehicle_data->get(i);

		UString id = data.get_vehicle_id(i);

		state.vehicle_types[id] = std::make_shared<VehicleType>();
		auto &vehicle = state.vehicle_types[id];
		auto name = data.vehicle_names->get(i);
		vehicle->name = name;
		vehicle->manufacturer = {&state, data.get_org_id(v.manufacturer)};
		vehicle->size = {v.size_x, v.size_y, v.size_z};
		vehicle->image_offset = {v.image_position_1, v.image_position_2};

		if (v.movement_type == 0)
		{
			vehicle->type = VehicleType::Type::Ground;
			int image_offset = 0;
			std::vector<VehicleType::Direction> directions = {
			    VehicleType::Direction::N,  VehicleType::Direction::NE, VehicleType::Direction::E,
			    VehicleType::Direction::SE, VehicleType::Direction::S,  VehicleType::Direction::SW,
			    VehicleType::Direction::W,  VehicleType::Direction::NW};
			auto bank = VehicleType::Banking::Flat;
			for (auto &dir : directions)
			{
				auto str =
				    UString::format("PCK:xcom3/UFODATA/VEHICLE.PCK:xcom3/UFODATA/VEHICLE.TAB:%d",
				                    (int)(v.graphic_frame + image_offset++));
				;
				vehicle->directional_sprites[bank][VehicleType::directionToVector(dir)] =
				    fw().data->load_image(str);
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
					auto str = UString::format(
					    "PCK:xcom3/UFODATA/VEHICLE.PCK:xcom3/UFODATA/VEHICLE.TAB:%d",
					    (int)(v.graphic_frame + image_offset++));
					vehicle->directional_sprites[bank][VehicleType::directionToVector(dir)] =
					    fw().data->load_image(str);
				}
			}
			// Give ground vehicles non-directional stratmap stuff for now
			// FIXME: How to select hostile/friendly/neutral stratmap sprites?
			auto str = UString::format(
			    "PCKSTRAT:xcom3/UFODATA/STRATMAP.PCK:xcom3/UFODATA/STRATMAP.TAB:%d", 572);
			vehicle->directional_strategy_sprites[Vec3<float>{1, 0, 0}] =
			    fw().data->load_image(str);
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
					auto str = UString::format(
					    "PCKSTRAT:xcom3/UFODATA/STRATMAP.PCK:xcom3/UFODATA/STRATMAP.TAB:%d", 572);
					vehicle->directional_strategy_sprites[Vec3<float>{1, 0, 0}] =
					    fw().data->load_image(str);
				}
				else if (v.size_x == 2 && v.size_y == 2)
				{
					// FIXME: Support 'large' strategy sprites?
					auto str = UString::format(
					    "PCKSTRAT:xcom3/UFODATA/STRATMAP.PCK:xcom3/UFODATA/STRATMAP.TAB:%d", 572);
					vehicle->directional_strategy_sprites[Vec3<float>{1, 0, 0}] =
					    fw().data->load_image(str);
#if 0
					for (int x = 0; x <= 1; x++)
					{
						for (int y = 0; y <= 1; y++)
						{
							std::stringstream ss;
							ss << "PCKSTRAT:xcom3/UFODATA/STRATMAP.PCK:xcom3/UFODATA/STRATMAP.TAB:"
							   << 573 + x + (2 * y);
							auto sprite = fw().data->load_image(ss.str());
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
					auto str =
					    UString::format("PCK:xcom3/UFODATA/SAUCER.PCK:xcom3/UFODATA/SAUCER.TAB:%d",
					                    (int)(v.graphic_frame + i));
					vehicle->animation_sprites.push_back(fw().data->load_image(str));
				}

				auto str =
				    UString::format("PCK:xcom3/UFODATA/SAUCER.PCK:xcom3/UFODATA/SAUCER.TAB:%d",
				                    (int)(v.graphic_frame + animFrames));

				vehicle->crashed_sprite = fw().data->load_image(str);

				str = UString::format(
				    "PCKSHADOW:xcom3/UFODATA/SHADOW.PCK:xcom3/UFODATA/SHADOW.TAB:%d",
				    (int)(v.shadow_graphic));
				vehicle->directional_shadow_sprites[{1, 0, 0}] = fw().data->load_image(str);
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
				std::vector<VehicleType::Direction> bankingDirections = {
				    VehicleType::Direction::N,  VehicleType::Direction::NNE,
				    VehicleType::Direction::NE, VehicleType::Direction::NEE,
				    VehicleType::Direction::E,  VehicleType::Direction::SEE,
				    VehicleType::Direction::SE, VehicleType::Direction::SSE,
				    VehicleType::Direction::S,  VehicleType::Direction::SSW,
				    VehicleType::Direction::SW, VehicleType::Direction::SWW,
				    VehicleType::Direction::W,  VehicleType::Direction::NWW,
				    VehicleType::Direction::NW, VehicleType::Direction::NNW};
				std::vector<VehicleType::Banking> bankings = {
				    VehicleType::Banking::Flat,      VehicleType::Banking::Descending,
				    VehicleType::Banking::Ascending, VehicleType::Banking::Left,
				    VehicleType::Banking::Right,
				};
				int image_offset = 577; // stratmap.pck:577 is the first directional sprite
				for (auto &dir : directions)
				{
					auto str = UString::format(
					    "PCKSTRAT:xcom3/UFODATA/STRATMAP.PCK:xcom3/UFODATA/STRATMAP.TAB:%d",
					    image_offset++);
					vehicle->directional_strategy_sprites[VehicleType::directionToVector(dir)] =
					    fw().data->load_image(str);
				}

				image_offset = 0;
				for (auto &bank : bankings)
				{
					auto directionsForThisBanking = directions;
					if (bank == VehicleType::Banking::Left || bank == VehicleType::Banking::Right)
						directionsForThisBanking = bankingDirections;
					for (auto &dir : directionsForThisBanking)
					{
						auto str = UString::format(
						    "PCK:xcom3/UFODATA/SAUCER.PCK:xcom3/UFODATA/SAUCER.TAB:%d",
						    (int)(v.graphic_frame + image_offset++));
						vehicle->directional_sprites[bank][VehicleType::directionToVector(dir)] =
						    fw().data->load_image(str);
					}
					// XXX HACK - The space liner doesn't have banking/ascending/descendimg images
					if (id == std::string("VEHICLETYPE_SPACE_LINER"))
						break;
				}

				// Shadows don't have banking
				image_offset = 0;
				for (auto &dir : directions)
				{
					auto str = UString::format(
					    "PCKSHADOW:xcom3/UFODATA/SHADOW.PCK:xcom3/UFODATA/SHADOW.TAB:%d",
					    (int)(v.shadow_graphic + image_offset++));
					vehicle->directional_shadow_sprites[VehicleType::directionToVector(dir)] =
					    fw().data->load_image(str);
				}
			}
		}
		else
		{
			LogError("Unknown type for vehicle %s", id.c_str());
		}

		vehicle->acceleration = v.acceleration;
		vehicle->top_speed = v.top_speed;
		vehicle->health = v.constitution;
		vehicle->crash_health = v.crash_constitution;
		vehicle->weight = v.weight;

		vehicle->armour[VehicleType::ArmourDirection::Rear] = v.armour_rear;
		vehicle->armour[VehicleType::ArmourDirection::Top] = v.armour_top;
		vehicle->armour[VehicleType::ArmourDirection::Left] = v.armour_left;
		vehicle->armour[VehicleType::ArmourDirection::Right] = v.armour_right;
		vehicle->armour[VehicleType::ArmourDirection::Bottom] = v.armour_bottom;
		vehicle->armour[VehicleType::ArmourDirection::Front] = v.armour_front;

		vehicle->passengers = v.passenger_capacity;
		vehicle->aggressiveness = v.aggressiveness;
		vehicle->score = v.score;

		// The equipment_screen_name is up to 8 chars, any may have NULLs. Memcpy() that to a char[]
		// and add a trailing null (in case all 8 are used) and it's just like a c-string!
		// Then we can use the const char* std::string constructor to convert to something we can
		// append to
		char equipment_screen_filename[9];
		memcpy((void *)&equipment_screen_filename[0], (void *)&v.equipment_screen_name[0], 8);
		equipment_screen_filename[8] = '\0';
		std::string equipment_screen_image =
		    "xcom3/ufodata/" + std::string(equipment_screen_filename) + ".pcx";
		// If it's all NULLs skip (as it might be an alien ship or something and therefore no
		// equipment screen)
		if (equipment_screen_filename[0] != '\0')
		{
			// Even some specified equipment screens don't exist - presumably for vehicles you can't
			// ever get (e.g. the 'airtaxi')
			auto img = fw().data->load_image(equipment_screen_image);
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
		auto str = UString::format(
		    "PCK:xcom3/ufodata/vs_icon.pck:xcom3/ufodata/vs_icon.tab:%d:xcom3/ufodata/pal_01.dat",
		    i + 4);
		vehicle->icon = fw().data->load_image(str);

		auto it = EquipscreenSprite.find(id);
		if (it != EquipscreenSprite.end())
		{
			auto str = UString::format("PCK:xcom3/ufodata/bigveh.pck:xcom3/ufodata/bigveh.tab:%d",
			                           (int)it->second);
			vehicle->equip_icon_big = fw().data->load_image(str);

			str = UString::format("PCK:xcom3/ufodata/smalveh.pck:xcom3/ufodata/smalveh.tab:%d",
			                      (int)it->second);
			vehicle->equip_icon_small = fw().data->load_image(str);
		}

		extract_equipment_layout(state, vehicle, data,
		                         data.vehicle_equipment_layouts->get(v.equipment_layout),
		                         v.loaded_equipment_slots);

		vehicle->voxelMap =
		    std::make_shared<VoxelMap>(Vec3<int>{v.size_x * 32, v.size_y * 32, v.size_z * 16});

		for (int i = 0; i < v.loftemps_height; i++)
		{
			auto str =
			    UString::format("LOFTEMPS:xcom3/UFODATA/LOFTEMPS.DAT:xcom3/UFODATA/LOFTEMPS.TAB:%d",
			                    (int)v.loftemps_index);
			vehicle->voxelMap->setSlice(i, fw().data->load_voxel_slice(str));
		}
	}
}

} // namespace OpenApoc

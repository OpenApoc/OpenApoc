#include "framework/framework.h"
#include "library/strings_format.h"
#include "library/voxel.h"
#include "tools/extractors/common/ufo2p.h"
#include "tools/extractors/extractors.h"
#include <list>
#include <map>
#include <set>

namespace OpenApoc
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

static void extract_equipment_layout(GameState &state, sp<VehicleType> vehicle, UFO2P &data,
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
			    StateRef<VEquipmentType>{&state, data.getVequipmentId(initial_equipment[i])});
		}
	}
}

void InitialGameStateExtractor::extractVehicles(GameState &state, Difficulty)
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
				auto str = format("PCK:xcom3/ufodata/vehicle.pck:xcom3/ufodata/vehicle.tab:%d",
				                  (int)(v.graphic_frame + image_offset++));
				;
				vehicle->directional_sprites[bank][VehicleType::directionToVector(dir)] =
				    fw().data->loadImage(str);
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
					vehicle->directional_sprites[bank][VehicleType::directionToVector(dir)] =
					    fw().data->loadImage(str);
				}
			}
			// Give ground vehicles non-directional stratmap stuff for now
			// FIXME: How to select hostile/friendly/neutral stratmap sprites?
			auto str =
			    format("PCKSTRAT:xcom3/ufodata/stratmaP.pck:xcom3/ufodata/stratmaP.tab:%d", 572);
			vehicle->directional_strategy_sprites[Vec3<float>{1, 0, 0}] = fw().data->loadImage(str);
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
					auto str = format(
					    "PCKSTRAT:xcom3/ufodata/stratmap.pck:xcom3/ufodata/stratmap.tab:%d", 572);
					vehicle->directional_strategy_sprites[Vec3<float>{1, 0, 0}] =
					    fw().data->loadImage(str);
				}
				else if (v.size_x == 2 && v.size_y == 2)
				{
					// FIXME: Support 'large' strategy sprites?
					auto str = format(
					    "PCKSTRAT:xcom3/ufodata/stratmap.pck:xcom3/ufodata/stratmap.tab:%d", 572);
					vehicle->directional_strategy_sprites[Vec3<float>{1, 0, 0}] =
					    fw().data->loadImage(str);
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
				vehicle->directional_shadow_sprites[{1, 0, 0}] = fw().data->loadImage(str);

				// UFOs starting with trans (id = 2) up to mship (id = 9) have maps from 51 to 58
				// Therefore 49 + id gives map index for the ufo
				if (i > 1)
					vehicle->battle_map = {&state, format("%s%s", BattleMap::getPrefix(),
					                                      this->battleMapPaths[48 + i])};
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
					auto str =
					    format("PCKSTRAT:xcom3/ufodata/stratmap.pck:xcom3/ufodata/stratmap.tab:%d",
					           image_offset++);
					vehicle->directional_strategy_sprites[VehicleType::directionToVector(dir)] =
					    fw().data->loadImage(str);
				}

				image_offset = 0;
				for (auto &bank : bankings)
				{
					auto directionsForThisBanking = directions;
					if (bank == VehicleType::Banking::Left || bank == VehicleType::Banking::Right)
						directionsForThisBanking = bankingDirections;
					for (auto &dir : directionsForThisBanking)
					{
						auto str =
						    format("PCK:xcom3/ufodata/saucer.pck:xcom3/ufodata/saucer.tab:%d",
						           (int)(v.graphic_frame + image_offset++));
						vehicle->directional_sprites[bank][VehicleType::directionToVector(dir)] =
						    fw().data->loadImage(str);
					}
					// XXX HACK - The space liner doesn't have banking/ascending/descendimg images
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
					vehicle->directional_shadow_sprites[VehicleType::directionToVector(dir)] =
					    fw().data->loadImage(str);
				}
			}
		}
		else
		{
			LogError("Unknown type for vehicle %s", id.cStr());
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
		    "xcom3/ufodata/" + UString(equipment_screen_filename).toLower().str() + ".pcx";
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

			str = format("PCK:xcom3/ufodata/smalveh.pck:xcom3/ufodata/smalveh.tab:%d",
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
		//  1) 119 and 120 have 166 and 117 respectively for horisontal alignment
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
		//	   and Hoverbike is 1x1x1 vehicle. We hsould just discard the bottom part

		// read indexes
		int horizontalVoxelMapIndex = -1;
		int verticalVoxelMapIdx = v.loftemps_index;
		switch (v.loftemps_index)
		{
			// bidirectional
			case 121:
			case 151:
				horizontalVoxelMapIndex = v.loftemps_index;
				break;
			// use 116 for horizontal direction
			case 119:
				horizontalVoxelMapIndex = 116;
				break;
			// use 117 for horizontal direction
			case 120:
				horizontalVoxelMapIndex = 117;
				break;
			//  bidirectional, but use 111 for both directions
			case 122:
				horizontalVoxelMapIndex = 111;
				verticalVoxelMapIdx = 111;
				break;
			// use 93 94 first row, 96 95 second row
			case 150:
				// special case, treated separately below
				break;
			default:
				LogError("Unsupported vehicle loftemps index %d!", (int)v.loftemps_index);
		}

		// read voxelmaps
		vehicle->voxelMaps =
		    std::vector<std::map<Vec3<float>, sp<VoxelMap>>>(v.size_x * v.size_y * v.size_z);
		switch (v.loftemps_index)
		{
			// omnidirectional
			case 121:
			case 122:
			case 151:
				for (int x = 0; x < v.size_x; x++)
				{
					for (int y = 0; y < v.size_y; y++)
					{
						for (int z = 0; z < v.size_z; z++)
						{
							// One facing
							vehicle->voxelMaps[z * v.size_y * v.size_x + y * v.size_x + x]
							                  [{0.0f, -1.0f, 0.0f}] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							int limit = std::max(16, v.loftemps_height - 16 * z);
							for (int i = 0; i < limit; i++)
							{
								vehicle
								    ->voxelMaps[z * v.size_y * v.size_x + y * v.size_x + x]
								               [{0.0f, -1.0f, 0.0f}]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      verticalVoxelMapIdx)));
							}
						}
					}
				}
				break;
			// bidirectional
			case 119:
			case 120:
				for (int x = 0; x < v.size_x; x++)
				{
					for (int y = 0; y < v.size_y; y++)
					{
						for (int z = 0; z < v.size_z; z++)
						{
							// Four facings
							// Facing north
							vehicle->voxelMaps[z * v.size_y * v.size_x + y * v.size_x + x]
							                  [{0.0f, -1.0f, 0.0f}] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							// Facing south
							vehicle->voxelMaps[z * v.size_y * v.size_x + y * v.size_x + x]
							                  [{0.0f, 1.0f, 0.0f}] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							// Facing east
							vehicle->voxelMaps[z * v.size_y * v.size_x + y * v.size_x + x]
							                  [{1.0f, 0.0f, 0.0f}] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							// Facing west
							vehicle->voxelMaps[z * v.size_y * v.size_x + y * v.size_x + x]
							                  [{-1.0f, 0.0f, 0.0f}] =
							    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
							int limit = std::max(16, v.loftemps_height - 16 * z);
							for (int i = 0; i < limit; i++)
							{
								// Facing north
								vehicle
								    ->voxelMaps[z * v.size_y * v.size_x + y * v.size_x + x]
								               [{0.0f, -1.0f, 0.0f}]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      verticalVoxelMapIdx)));
								// Facing south
								vehicle
								    ->voxelMaps[z * v.size_y * v.size_x + y * v.size_x + x]
								               [{0.0f, 1.0f, 0.0f}]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      verticalVoxelMapIdx)));
								// Facing east
								vehicle
								    ->voxelMaps[z * v.size_y * v.size_x + y * v.size_x + x]
								               [{1.0f, 0.0f, 0.0f}]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      horizontalVoxelMapIndex)));
								// Facing west
								vehicle
								    ->voxelMaps[z * v.size_y * v.size_x + y * v.size_x + x]
								               [{-1.0f, 0.0f, 0.0f}]
								    ->setSlice(i, fw().data->loadVoxelSlice(format(
								                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
								                      "ufodata/loftemps.tab:%d",
								                      horizontalVoxelMapIndex)));
							}
						}
					}
				}
				break;
			// use 93 94 first row, 96 95 second row
			case 150:
				if (v.size_x != 2 || v.size_y != 2)
				{
					LogError("Vehicle Type using loftemps 150 has invalid x and y size: expected "
					         "2x2, got %dx%d",
					         v.size_x, v.size_y);
				}
				std::map<Vec2<int>, int> loftempsMap = {
				    {{0, 0}, 93}, {{1, 0}, 94}, {{0, 1}, 96}, {{1, 1}, 95}};
				for (int z = 0; z < v.size_z; z++)
				{
					for (auto &pair : loftempsMap)
					{
						vehicle->voxelMaps[z * v.size_y * v.size_x + pair.first.y * v.size_x +
						                   pair.first.x][{0.0f, -1.0f, 0.0f}] =
						    std::make_shared<VoxelMap>(Vec3<int>{32, 32, 16});
						int limit = std::max(16, v.loftemps_height - 16 * z);
						for (int i = 0; i < limit; i++)
						{
							vehicle
							    ->voxelMaps[z * v.size_y * v.size_x + pair.first.y * v.size_x +
							                pair.first.x][{0.0f, -1.0f, 0.0f}]
							    ->setSlice(i, fw().data->loadVoxelSlice(format(
							                      "LOFTEMPS:xcom3/ufodata/loftemps.dat:xcom3/"
							                      "ufodata/loftemps.tab:%d",
							                      pair.second)));
						}
					}
				}
				break;
		}
	}
}

} // namespace OpenApoc

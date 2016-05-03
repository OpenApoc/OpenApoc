#include "game/state/gamestate.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/serialize.h"
#include "game/state/base/facility.h"
#include "game/state/city/baselayout.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/doodad.h"
#include "game/state/city/projectile.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/rules/vequipment.h"
#include "library/voxel.h"

namespace OpenApoc
{

namespace
{

/* Avoid out-of-order declarations breaking */
template <typename T> void serializeIn(const GameState *state, sp<SerializationNode> node, T &val);
template <typename T> void serializeOut(sp<SerializationNode> node, const T &val);

template <typename T>
void serializeIn(const GameState *state, sp<SerializationNode> node, T &val,
                 const std::map<T, UString> &valueMap);
template <typename T>
void serializeOut(sp<SerializationNode> node, const T &val, const std::map<T, UString> &valueMap);

void serializeIn(const GameState *state, sp<SerializationNode> node, UString &str)
{
	if (!node)
		return;
	str = node->getValue();
}

void serializeIn(const GameState *state, sp<SerializationNode> node, unsigned int &val)
{
	if (!node)
		return;
	val = node->getValueUInt();
}

void serializeIn(const GameState *state, sp<SerializationNode> node, unsigned char &val)
{
	if (!node)
		return;
	val = node->getValueUChar();
}

void serializeIn(const GameState *state, sp<SerializationNode> node, float &val)
{
	if (!node)
		return;
	val = node->getValueFloat();
}

void serializeIn(const GameState *state, sp<SerializationNode> node, int &val)
{
	if (!node)
		return;
	val = node->getValueInt();
}

void serializeIn(const GameState *state, sp<SerializationNode> node, bool &val)
{
	if (!node)
		return;
	val = node->getValueBool();
}

template <typename T>
void serializeIn(const GameState *state, sp<SerializationNode> node, StateRef<T> &ref)
{
	if (!node)
		return;
	ref = StateRef<T>{state, node->getValue()};
}

template <typename T>
void serializeIn(const GameState *state, sp<SerializationNode> node, up<T> &ptr)
{
	if (!node)
		return;
	auto upNode = node->getNodeOpt("up");
	if (upNode)
	{
		if (!ptr)
		{
			ptr.reset(new T);
		}
		serializeIn(state, upNode, *ptr);
	}
}

template <typename T>
void serializeIn(const GameState *state, sp<SerializationNode> node, sp<T> &ptr)
{
	if (!node)
		return;
	auto spNode = node->getNodeOpt("sp");
	if (spNode)
	{
		if (!ptr)
		{
			ptr = std::make_shared<T>();
		}
		serializeIn(state, spNode, *ptr);
	}
}

template <>
void serializeIn<LazyImage>(const GameState *state, sp<SerializationNode> node, sp<LazyImage> &ptr)
{
	if (!node)
		return;
	ptr = std::static_pointer_cast<LazyImage>(fw().data->load_image(node->getValue(), true));
}

template <>
void serializeIn<Image>(const GameState *state, sp<SerializationNode> node, sp<Image> &ptr)
{
	if (!node)
		return;
	ptr = fw().data->load_image(node->getValue());
}

template <typename T>
void serializeIn(const GameState *state, sp<SerializationNode> node, Vec3<T> &val)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("x"), val.x);
	serializeIn(state, node->getNode("y"), val.y);
	serializeIn(state, node->getNode("z"), val.z);
}

template <typename T>
void serializeIn(const GameState *state, sp<SerializationNode> node, Vec2<T> &val)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("x"), val.x);
	serializeIn(state, node->getNode("y"), val.y);
}

template <typename T>
void serializeIn(const GameState *state, sp<SerializationNode> node, Rect<T> &val)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("p0"), val.p0);
	serializeIn(state, node->getNode("p1"), val.p1);
}

template <typename T>
void serializeIn(const GameState *state, sp<SerializationNode> node, T &val,
                 const std::map<T, UString> &valueMap)
{
	if (!node)
		return;
	auto str = node->getValue();
	for (auto &pair : valueMap)
	{
		if (pair.second == str)
		{
			val = pair.first;
			return;
		}
	}
	throw SerializationException(
	    UString::format("Invalid enum value for %s: \"%s\"", typeid(T).name(), str.c_str()), node);
}

template <typename T>
void serializeIn(const GameState *state, sp<SerializationNode> node, std::list<T> &list);

template <typename Key, typename Value>
void serializeIn(const GameState *state, sp<SerializationNode> node, std::map<Key, Value> &map)
{
	if (!node)
		return;
	auto entry = node->getNodeOpt("entry");
	while (entry)
	{
		Key key;
		serializeIn(state, entry->getNodeReq("key"), key);
		auto &value = map[key];
		serializeIn(state, entry->getNodeReq("value"), value);

		entry = entry->getNextSiblingOpt("entry");
	}
}

template <typename Value>
void serializeInSectionMap(const GameState *state, sp<SerializationNode> node,
                           std::map<UString, Value> &map)
{
	if (!node)
		return;
	auto entry = node->getNodeOpt("entry");
	while (entry)
	{
		UString key;
		serializeIn(state, entry->getNodeReq("key"), key);
		auto &value = map[key];
		serializeIn(state, entry->getSectionReq(key), value);

		entry = entry->getNextSiblingOpt("entry");
	}
}

template <typename Key, typename Value>
void serializeIn(const GameState *state, sp<SerializationNode> node, std::map<Key, Value> &map,
                 const std::map<Key, UString> &keyMap)
{
	if (!node)
		return;
	auto entry = node->getNodeOpt("entry");
	while (entry)
	{
		Key key;
		serializeIn(state, entry->getNodeReq("key"), key, keyMap);
		auto &value = map[key];
		serializeIn(state, entry->getNodeReq("value"), value);

		entry = entry->getNextSiblingOpt("entry");
	}
}

template <typename A, typename B>
void serializeIn(const GameState *state, sp<SerializationNode> node, std::pair<A, B> &pair)
{
	if (!node)
		return;
	serializeIn(state, node->getNodeReq("first"), pair.first);
	serializeIn(state, node->getNodeReq("second"), pair.second);
}

template <typename T>
void serializeIn(const GameState *state, sp<SerializationNode> node, std::list<T> &list)
{
	if (!node)
		return;
	auto entry = node->getNodeOpt("entry");
	while (entry)
	{
		list.emplace_back();
		serializeIn(state, entry, list.back());
		entry = entry->getNextSiblingOpt("entry");
	}
}

template <typename T>
void serializeIn(const GameState *state, sp<SerializationNode> node, std::vector<T> &vector)
{
	if (!node)
		return;
	auto entry = node->getNodeOpt("entry");
	while (entry)
	{
		vector.emplace_back();
		auto &val = vector.back();
		serializeIn(state, entry, val);
		entry = entry->getNextSiblingOpt("entry");
	}
}
// std::vector<bool> is special
template <>
void serializeIn(const GameState *state, sp<SerializationNode> node, std::vector<bool> &vector)
{
	if (!node)
		return;
	vector = node->getValueBoolVector();
}

template <>
void serializeIn(const GameState *state, sp<SerializationNode> node,
                 VehicleType::EquipmentLayoutSlot &slot)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("type"), slot.type, VEquipmentType::TypeMap);
	serializeIn(state, node->getNode("align_x"), slot.align_x, VehicleType::AlignmentXMap);
	serializeIn(state, node->getNode("align_y"), slot.align_y, VehicleType::AlignmentYMap);
	serializeIn(state, node->getNode("bounds"), slot.bounds);
}

template <>
void serializeIn<VoxelSlice>(const GameState *state, sp<SerializationNode> node,
                             sp<VoxelSlice> &ptr)
{
	if (!node)
		return;
	ptr = fw().data->load_voxel_slice(node->getValue());
}

template <>
void serializeIn<Sample>(const GameState *state, sp<SerializationNode> node, sp<Sample> &ptr)
{
	if (!node)
		return;
	ptr = fw().data->load_sample(node->getValue());
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, VoxelMap &map)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("size"), map.size);
	serializeIn(state, node->getNode("slices"), map.slices);
}

template <>
void serializeIn(const GameState *state, sp<SerializationNode> node, VEquipmentType::User &user)
{
	if (!node)
		return;
	serializeIn(state, node, user, VEquipmentType::UserMap);
}

template <typename T>
void serializeIn(const GameState *state, sp<SerializationNode> node, std::set<T> &set)
{
	if (!node)
		return;
	auto entry = node->getNodeOpt("entry");
	while (entry)
	{
		T type;
		serializeIn(state, entry, type);
		set.insert(type);
		entry = entry->getNextSiblingOpt("entry");
	}
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, VehicleType &type)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("numCreated"), type.numCreated);
	serializeIn(state, node->getNode("type"), type.type, type.TypeMap);
	serializeIn(state, node->getNode("name"), type.name);
	serializeIn(state, node->getNode("manufacturer"), type.manufacturer);
	serializeIn(state, node->getNode("size"), type.size);
	serializeIn(state, node->getNode("image_offset"), type.image_offset);
	serializeIn(state, node->getNode("acceleration"), type.acceleration);
	serializeIn(state, node->getNode("top_speed"), type.top_speed);
	serializeIn(state, node->getNode("health"), type.health);
	serializeIn(state, node->getNode("crash_health"), type.crash_health);
	serializeIn(state, node->getNode("weight"), type.weight);
	serializeIn(state, node->getNode("armour"), type.armour, type.ArmourDirectionMap);
	serializeIn(state, node->getNode("passengers"), type.passengers);
	serializeIn(state, node->getNode("aggressiveness"), type.aggressiveness);
	serializeIn(state, node->getNode("score"), type.score);
	serializeIn(state, node->getNode("icon"), type.icon);
	serializeIn(state, node->getNode("equipment_screen"), type.equipment_screen);
	serializeIn(state, node->getNode("equip_icon_big"), type.equip_icon_big);
	serializeIn(state, node->getNode("equip_icon_small"), type.equip_icon_small);
	serializeIn(state, node->getNode("directional_strategy_sprites"),
	            type.directional_strategy_sprites);
	serializeIn(state, node->getNode("directional_sprites"), type.directional_sprites,
	            type.BankingMap);
	serializeIn(state, node->getNode("shadow_offset"), type.shadow_offset);
	serializeIn(state, node->getNode("directional_shadow_sprites"),
	            type.directional_shadow_sprites);
	serializeIn(state, node->getNode("animation_sprites"), type.animation_sprites);
	serializeIn(state, node->getNode("crashed_sprite"), type.crashed_sprite);
	serializeIn(state, node->getNode("voxelMap"), type.voxelMap);
	serializeIn(state, node->getNode("equipment_layout_slots"), type.equipment_layout_slots);
	serializeIn(state, node->getNode("initial_equipment_list"), type.initial_equipment_list);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Organisation &org)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("name"), org.name);
	serializeIn(state, node->getNode("balance"), org.balance);
	serializeIn(state, node->getNode("income"), org.income);
	serializeIn(state, node->getNode("current_relations"), org.current_relations);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, FacilityType &f)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("name"), f.name);
	serializeIn(state, node->getNode("fixed"), f.fixed);
	serializeIn(state, node->getNode("buildCost"), f.buildCost);
	serializeIn(state, node->getNode("buildTime"), f.buildTime);
	serializeIn(state, node->getNode("weeklyCost"), f.weeklyCost);
	serializeIn(state, node->getNode("capacityType"), f.capacityType, FacilityType::CapacityMap);
	serializeIn(state, node->getNode("capacityAmount"), f.capacityAmount);
	serializeIn(state, node->getNode("size"), f.size);
	serializeIn(state, node->getNode("sprite"), f.sprite);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, DoodadFrame &f)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("image"), f.image);
	serializeIn(state, node->getNode("time"), f.time);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, DoodadType &d)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("lifetime"), d.lifetime);
	serializeIn(state, node->getNode("imageOffset"), d.imageOffset);
	serializeIn(state, node->getNode("frames"), d.frames);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, VEquipmentType &e)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("type"), e.type, VEquipmentType::TypeMap);
	serializeIn(state, node->getNode("id"), e.id);
	serializeIn(state, node->getNode("name"), e.name);
	serializeIn(state, node->getNode("weight"), e.weight);
	serializeIn(state, node->getNode("max_ammo"), e.max_ammo);
	serializeIn(state, node->getNode("ammo_type"), e.ammo_type);
	serializeIn(state, node->getNode("equipscreen_sprite"), e.equipscreen_sprite);
	serializeIn(state, node->getNode("equipscreen_size"), e.equipscreen_size);
	serializeIn(state, node->getNode("manufacturer"), e.manufacturer);
	serializeIn(state, node->getNode("store_space"), e.store_space);
	serializeIn(state, node->getNode("users"), e.users);
	// Weapons
	serializeIn(state, node->getNode("speed"), e.speed);
	serializeIn(state, node->getNode("projectile_image"), e.projectile_image);
	serializeIn(state, node->getNode("damage"), e.damage);
	serializeIn(state, node->getNode("accuracy"), e.accuracy);
	serializeIn(state, node->getNode("fire_delay"), e.fire_delay);
	serializeIn(state, node->getNode("tail_size"), e.tail_size);
	serializeIn(state, node->getNode("guided"), e.guided);
	serializeIn(state, node->getNode("turn_rate"), e.turn_rate);
	serializeIn(state, node->getNode("range"), e.range);
	serializeIn(state, node->getNode("firing_arc_1"), e.firing_arc_1);
	serializeIn(state, node->getNode("firing_arc_2"), e.firing_arc_2);
	serializeIn(state, node->getNode("point_defence"), e.point_defence);
	serializeIn(state, node->getNode("fire_sfx"), e.fire_sfx);
	serializeIn(state, node->getNode("explosion_graphic"), e.explosion_graphic);
	serializeIn(state, node->getNode("icon"), e.icon);
	// Engine
	serializeIn(state, node->getNode("power"), e.power);
	serializeIn(state, node->getNode("top_speed"), e.top_speed);
	// General
	serializeIn(state, node->getNode("accuracy_modifier"), e.accuracy_modifier);
	serializeIn(state, node->getNode("cargo_space"), e.cargo_space);
	serializeIn(state, node->getNode("passengers"), e.passengers);
	serializeIn(state, node->getNode("alien_space"), e.alien_space);
	serializeIn(state, node->getNode("missile_jamming"), e.missile_jamming);
	serializeIn(state, node->getNode("shielding"), e.shielding);
	serializeIn(state, node->getNode("cloaking"), e.cloaking);
	serializeIn(state, node->getNode("teleporting"), e.teleporting);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Colour &c)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("r"), c.r);
	serializeIn(state, node->getNode("g"), c.g);
	serializeIn(state, node->getNode("b"), c.b);
	serializeIn(state, node->getNode("a"), c.a);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, SceneryTileType &t)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("sprite"), t.sprite);
	serializeIn(state, node->getNode("strategySprite"), t.strategySprite);
	serializeIn(state, node->getNode("overlaySprite"), t.overlaySprite);
	serializeIn(state, node->getNode("voxelMap"), t.voxelMap);
	serializeIn(state, node->getNode("damagedTile"), t.damagedTile);
	serializeIn(state, node->getNode("imageOffset"), t.imageOffset);
	serializeIn(state, node->getNode("isLandingPad"), t.isLandingPad);
	serializeIn(state, node->getNode("minimap_colour"), t.minimap_colour);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, BaseLayout &l)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("baseCorridors"), l.baseCorridors);
	serializeIn(state, node->getNode("baseLift"), l.baseLift);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Building &b)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("name"), b.name);
	serializeIn(state, node->getNode("owner"), b.owner);
	serializeIn(state, node->getNode("bounds"), b.bounds);
	serializeIn(state, node->getNode("base_layout"), b.base_layout);
	// FIXME: Are landing pad locations useful to serialize?
	serializeIn(state, node->getNode("landingPadLocations"), b.landingPadLocations);
	serializeIn(state, node->getNode("landed_vehicles"), b.landed_vehicles);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Scenery &s)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("type"), s.type);
	serializeIn(state, node->getNode("initialPosition"), s.initialPosition);
	serializeIn(state, node->getNode("currentPosition"), s.currentPosition);
	serializeIn(state, node->getNode("damaged"), s.damaged);
	serializeIn(state, node->getNode("falling"), s.falling);
	serializeIn(state, node->getNode("destroyed"), s.destroyed);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Doodad &doodad)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("position"), doodad.position);
	serializeIn(state, node->getNode("imageOffset"), doodad.imageOffset);
	serializeIn(state, node->getNode("temporary"), doodad.temporary);
	serializeIn(state, node->getNode("age"), doodad.age);
	serializeIn(state, node->getNode("lifetime"), doodad.lifetime);
	serializeIn(state, node->getNode("sprite"), doodad.sprite);
	serializeIn(state, node->getNode("type"), doodad.type);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Projectile &p)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("type"), p.type, Projectile::TypeMap);
	serializeIn(state, node->getNode("position"), p.position);
	serializeIn(state, node->getNode("velocity"), p.velocity);
	serializeIn(state, node->getNode("age"), p.age);
	serializeIn(state, node->getNode("lifetime"), p.lifetime);
	serializeIn(state, node->getNode("firer"), p.firer);
	serializeIn(state, node->getNode("previousPosition"), p.previousPosition);
	serializeIn(state, node->getNode("colour"), p.colour);
	serializeIn(state, node->getNode("beamLength"), p.beamLength);
	serializeIn(state, node->getNode("beamWidth"), p.beamWidth);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, City &city)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("size"), city.size);
	serializeIn(state, node->getSection("tile_types"), city.tile_types);
	serializeIn(state, node->getSection("initial_tiles"), city.initial_tiles);
	serializeIn(state, node->getSection("buildings"), city.buildings);
	serializeIn(state, node->getSection("scenery"), city.scenery);
	serializeIn(state, node->getSection("doodads"), city.doodads);
	serializeIn(state, node->getSection("projectiles"), city.projectiles);
}

template <>
void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchTopic::Type &t)
{
	serializeIn(state, node, t, ResearchTopic::TypeMap);
}

template <>
void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchTopic::LabSize &s)
{
	serializeIn(state, node, s, ResearchTopic::LabSizeMap);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Lab &lab)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("size"), lab.size);
	serializeIn(state, node->getNode("type"), lab.type);
	serializeIn(state, node->getNode("current_project"), lab.current_project);
	serializeIn(state, node->getNode("assigned_agents"), lab.assigned_agents);
	serializeIn(state, node->getNode("ticks_since_last_progress"), lab.ticks_since_last_progress);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Facility &facility)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("type"), facility.type);
	serializeIn(state, node->getNode("pos"), facility.pos);
	serializeIn(state, node->getNode("buildTime"), facility.buildTime);
	serializeIn(state, node->getNode("lab"), facility.lab);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Base &base)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("corridors"), base.corridors);
	serializeIn(state, node->getNode("facilities"), base.facilities);
	serializeIn(state, node->getNode("inventory"), base.inventory);
	serializeIn(state, node->getNode("name"), base.name);
	serializeIn(state, node->getNode("building"), base.building);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, VehicleMission &m)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("type"), m.type, VehicleMission::TypeMap);
	serializeIn(state, node->getNode("target_location"), m.targetLocation);
	serializeIn(state, node->getNode("target_building"), m.targetBuilding);
	serializeIn(state, node->getNode("target_vehicle"), m.targetVehicle);
	serializeIn(state, node->getNode("time_to_snooze"), m.timeToSnooze);
	serializeIn(state, node->getNode("current_planned_path"), m.currentPlannedPath);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, VEquipment &e)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("type"), e.type);
	serializeIn(state, node->getNode("equipped_position"), e.equippedPosition);
	serializeIn(state, node->getNode("weapon_state"), e.weaponState, VEquipment::WeaponStateMap);
	serializeIn(state, node->getNode("owner"), e.owner);
	serializeIn(state, node->getNode("ammo"), e.ammo);
	serializeIn(state, node->getNode("reload_time"), e.reloadTime);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Vehicle &v)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("type"), v.type);
	serializeIn(state, node->getNode("owner"), v.owner);
	serializeIn(state, node->getNode("name"), v.name);
	serializeIn(state, node->getNode("missions"), v.missions);
	serializeIn(state, node->getNode("equipment"), v.equipment);
	serializeIn(state, node->getNode("position"), v.position);
	serializeIn(state, node->getNode("velocity"), v.velocity);
	serializeIn(state, node->getNode("facing"), v.facing);
	serializeIn(state, node->getNode("city"), v.city);
	serializeIn(state, node->getNode("health"), v.health);
	serializeIn(state, node->getNode("shield"), v.shield);
	serializeIn(state, node->getNode("homeBuilding"), v.homeBuilding);
	serializeIn(state, node->getNode("currentlyLandedBuilding"), v.currentlyLandedBuilding);
}

template <>
void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchDependency::Type &t)
{
	serializeIn(state, node, t, ResearchDependency::TypeMap);
}

template <>
void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchDependency &d)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("type"), d.type);
	serializeIn(state, node->getNode("topics"), d.topics);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, UfopaediaEntry &e)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("title"), e.title);
	serializeIn(state, node->getNode("description"), e.description);
	serializeIn(state, node->getNode("background"), e.background);
	serializeIn(state, node->getNode("data_id"), e.data_id);
	serializeIn(state, node->getNode("data_type"), e.data_type, UfopaediaEntry::DataMap);
	serializeIn(state, node->getNode("dependency"), e.dependency);
}

template <>
void serializeIn(const GameState *state, sp<SerializationNode> node, UfopaediaCategory &c)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("title"), c.title);
	serializeIn(state, node->getNode("description"), c.description);
	serializeIn(state, node->getNode("background"), c.background);
	serializeIn(state, node->getNode("entries"), c.entries);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, ItemDependency &d)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("items"), d.items);
}

template <>
void serializeIn(const GameState *state, sp<SerializationNode> node, ProjectDependencies &d)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("research"), d.research);
	serializeIn(state, node->getNode("items"), d.items);
}
template <> void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchTopic &r)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("name"), r.name);
	serializeIn(state, node->getNode("description"), r.description);
	serializeIn(state, node->getNode("ufopaedia_entry"), r.ufopaedia_entry);
	serializeIn(state, node->getNode("man_hours"), r.man_hours);
	serializeIn(state, node->getNode("man_hours_progress"), r.man_hours_progress);
	serializeIn(state, node->getNode("type"), r.type);
	serializeIn(state, node->getNode("required_lab_size"), r.required_lab_size);
	serializeIn(state, node->getNode("current_lab"), r.current_lab);
	serializeIn(state, node->getNode("score"), r.score);
	serializeIn(state, node->getNode("started"), r.started);
	serializeIn(state, node->getNode("dependencies"), r.dependencies);
}

// FIXME: These enums don't quite work the same way as the other map style - maybe a common way
// could be used?
template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Agent::Type &t)
{
	serializeIn(state, node, t, Agent::TypeMap);
}
template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Agent::Gender &g)
{
	serializeIn(state, node, g, Agent::GenderMap);
}
template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Agent::Species &s)
{
	serializeIn(state, node, s, Agent::SpeciesMap);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, AgentStats &s)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("health"), s.health);
	serializeIn(state, node->getNode("accuracy"), s.accuracy);
	serializeIn(state, node->getNode("reactions"), s.reactions);
	serializeIn(state, node->getNode("speed"), s.speed);
	serializeIn(state, node->getNode("stamina"), s.stamina);
	serializeIn(state, node->getNode("bravery"), s.bravery);
	serializeIn(state, node->getNode("strength"), s.strength);
	serializeIn(state, node->getNode("psi_energy"), s.psi_energy);
	serializeIn(state, node->getNode("psi_attack"), s.psi_attack);
	serializeIn(state, node->getNode("psi_defence"), s.psi_defence);
	serializeIn(state, node->getNode("physics_skill"), s.physics_skill);
	serializeIn(state, node->getNode("biochem_skill"), s.biochem_skill);
	serializeIn(state, node->getNode("engineering_skill"), s.engineering_skill);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, AgentPortrait &p)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("photo"), p.photo);
	serializeIn(state, node->getNode("icon"), p.icon);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, Agent &a)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("name"), a.name);
	serializeIn(state, node->getNode("portrait"), a.portrait);
	serializeIn(state, node->getNode("type"), a.type, Agent::TypeMap);
	serializeIn(state, node->getNode("species"), a.species, Agent::SpeciesMap);
	serializeIn(state, node->getNode("gender"), a.gender, Agent::GenderMap);
	serializeIn(state, node->getNode("initial_stats"), a.initial_stats);
	serializeIn(state, node->getNode("current_stats"), a.current_stats);
	serializeIn(state, node->getNode("home_base"), a.home_base);
	serializeIn(state, node->getNode("owner"), a.owner);
	serializeIn(state, node->getNode("assigned_to_lab"), a.assigned_to_lab);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, AgentGenerator &g)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("num_created"), g.num_created);
	serializeIn(state, node->getNode("first_names"), g.first_names);
	serializeIn(state, node->getNode("second_names"), g.second_names);
	serializeIn(state, node->getNode("type_chance"), g.type_chance);
	serializeIn(state, node->getNode("species_chance"), g.species_chance);
	serializeIn(state, node->getNode("gender_chance"), g.gender_chance);
	serializeIn(state, node->getNode("portraits"), g.portraits);
	serializeIn(state, node->getNode("min_stats"), g.min_stats);
	serializeIn(state, node->getNode("max_stats"), g.max_stats);
}

template <> void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchState &r)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("topics"), r.topics);
	serializeIn(state, node->getNode("labs"), r.labs);
	serializeIn(state, node->getNode("num_labs_created"), r.num_labs_created);
}

void serializeIn(const GameState *state, sp<SerializationNode> node, GameState &s)
{
	if (!node)
		return;
	serializeIn(state, node->getSection("vehicle_types"), s.vehicle_types);
	serializeIn(state, node->getSection("organisations"), s.organisations);
	serializeIn(state, node->getSection("facility_types"), s.facility_types);
	serializeIn(state, node->getSection("doodad_types"), s.doodad_types);
	serializeIn(state, node->getSection("vehicle_equipment"), s.vehicle_equipment);
	serializeInSectionMap(state, node->getSection("cities"), s.cities);
	serializeIn(state, node->getSection("base_layouts"), s.base_layouts);
	serializeIn(state, node->getSection("player_bases"), s.player_bases);
	serializeIn(state, node->getSection("vehicles"), s.vehicles);
	serializeIn(state, node->getSection("ufopaedia"), s.ufopaedia);
	serializeIn(state, node->getSection("research"), s.research);
	serializeIn(state, node->getSection("agents"), s.agents);
	serializeIn(state, node->getSection("agent_generator"), s.agent_generator);
	serializeIn(state, node->getNode("initial_agents"), s.initial_agents);
	serializeIn(state, node->getNode("initial_facilities"), s.initial_facilities);
	serializeIn(state, node->getNode("player"), s.player);
	serializeIn(state, node->getNode("current_city"), s.current_city);
	serializeIn(state, node->getNode("time"), s.time);
}

void serializeOut(sp<SerializationNode> node, const UString &string) { node->setValue(string); }

void serializeOut(sp<SerializationNode> node, const unsigned int &val) { node->setValueUInt(val); }

void serializeOut(sp<SerializationNode> node, const unsigned char &val)
{
	node->setValueUChar(val);
}

void serializeOut(sp<SerializationNode> node, const float &val) { node->setValueFloat(val); }

void serializeOut(sp<SerializationNode> node, const int &val) { node->setValueInt(val); }

void serializeOut(sp<SerializationNode> node, const bool &val) { node->setValueBool(val); }

template <typename T> void serializeOut(sp<SerializationNode> node, const StateRef<T> &ref)
{
	// node->setValue("") causes an extra unnecessary <tag></tag> instead of <tag />
	if (ref.id != "")
		node->setValue(ref.id);
}

template <typename T> void serializeOut(sp<SerializationNode> node, const Vec3<T> &val)
{
	serializeOut(node->addNode("x"), val.x);
	serializeOut(node->addNode("y"), val.y);
	serializeOut(node->addNode("z"), val.z);
}

template <typename T> void serializeOut(sp<SerializationNode> node, const Vec2<T> &val)
{
	serializeOut(node->addNode("x"), val.x);
	serializeOut(node->addNode("y"), val.y);
}

template <typename T> void serializeOut(sp<SerializationNode> node, const Rect<T> &val)
{
	serializeOut(node->addNode("p0"), val.p0);
	serializeOut(node->addNode("p1"), val.p1);
}

template <typename T>
void serializeOut(sp<SerializationNode> node, const T &val, const std::map<T, UString> &valueMap)
{
	auto it = valueMap.find(val);
	if (it == valueMap.end())
	{
		LogError("Invalid enum value for %s: %d", typeid(T).name(), (int)val);
	}
	node->setValue(it->second);
}

template <typename T> void serializeOut(sp<SerializationNode> node, const up<T> &ptr)
{
	if (ptr)
	{
		serializeOut(node->addNode("up"), *ptr);
	}
}

template <typename T> void serializeOut(sp<SerializationNode> node, const sp<T> &ptr)
{
	if (ptr)
	{
		serializeOut(node->addNode("sp"), *ptr);
	}
}

template <> void serializeOut<LazyImage>(sp<SerializationNode> node, const sp<LazyImage> &ptr)
{
	if (ptr != nullptr)
	{
		node->setValue(ptr->path);
	}
}

template <> void serializeOut<Image>(sp<SerializationNode> node, const sp<Image> &ptr)
{
	if (ptr != nullptr)
	{
		node->setValue(ptr->path);
	}
}

template <> void serializeOut<VoxelSlice>(sp<SerializationNode> node, const sp<VoxelSlice> &ptr)
{
	if (ptr)
	{
		node->setValue(ptr->path);
	}
}

template <> void serializeOut<Sample>(sp<SerializationNode> node, const sp<Sample> &ptr)
{
	if (ptr)
	{
		node->setValue(ptr->path);
	}
}

template <typename T> void serializeOut(sp<SerializationNode> node, const std::list<T> &list);

template <typename Key, typename Value>
void serializeOut(sp<SerializationNode> node, const std::map<Key, Value> &map)
{
	for (const auto &pair : map)
	{
		auto entry = node->addNode("entry");
		serializeOut(entry->addNode("key"), pair.first);
		serializeOut(entry->addNode("value"), pair.second);
	}
}

template <typename Value>
void serializeOutSectionMap(sp<SerializationNode> node, const std::map<UString, Value> &map)
{
	for (const auto &pair : map)
	{
		auto entry = node->addNode("entry");
		serializeOut(entry->addNode("key"), pair.first);
		serializeOut(entry->addSection(pair.first), pair.second);
	}
}

template <typename Key, typename Value>
void serializeOut(sp<SerializationNode> node, const std::map<Key, Value> &map,
                  const std::map<Key, UString> &keyMap)
{
	for (const auto &pair : map)
	{
		auto entry = node->addNode("entry");
		serializeOut(entry->addNode("key"), pair.first, keyMap);
		serializeOut(entry->addNode("value"), pair.second);
	}
}

template <typename T> void serializeOut(sp<SerializationNode> node, const std::set<T> &set)
{
	for (const auto &entry : set)
	{
		serializeOut(node->addNode("entry"), entry);
	}
}

template <typename A, typename B>
void serializeOut(sp<SerializationNode> node, const std::pair<A, B> &pair)
{
	serializeOut(node->addNode("first"), pair.first);
	serializeOut(node->addNode("second"), pair.second);
}

template <typename T> void serializeOut(sp<SerializationNode> node, const std::list<T> &list)
{
	for (auto &entry : list)
	{
		serializeOut(node->addNode("entry"), entry);
	}
}

template <typename T> void serializeOut(sp<SerializationNode> node, const std::vector<T> &vector)
{
	for (auto &entry : vector)
	{
		serializeOut(node->addNode("entry"), entry);
	}
}

template <> void serializeOut(sp<SerializationNode> node, const std::vector<bool> &vector)
{
	node->setValueBoolVector(vector);
}

template <>
void serializeOut(sp<SerializationNode> node, const VehicleType::EquipmentLayoutSlot &slot)
{
	serializeOut(node->addNode("type"), slot.type, VEquipmentType::TypeMap);
	serializeOut(node->addNode("align_x"), slot.align_x, VehicleType::AlignmentXMap);
	serializeOut(node->addNode("align_y"), slot.align_y, VehicleType::AlignmentYMap);
	serializeOut(node->addNode("bounds"), slot.bounds);
}

template <> void serializeOut(sp<SerializationNode> node, const VoxelMap &map)
{
	serializeOut(node->addNode("size"), map.size);
	serializeOut(node->addNode("slices"), map.slices);
}

template <> void serializeOut(sp<SerializationNode> node, const VehicleType &type)
{
	serializeOut(node->addNode("numCreated"), type.numCreated);
	serializeOut(node->addNode("type"), type.type, type.TypeMap);
	serializeOut(node->addNode("name"), type.name);
	serializeOut(node->addNode("manufacturer"), type.manufacturer);
	serializeOut(node->addNode("size"), type.size);
	serializeOut(node->addNode("image_offset"), type.image_offset);
	serializeOut(node->addNode("acceleration"), type.acceleration);
	serializeOut(node->addNode("top_speed"), type.top_speed);
	serializeOut(node->addNode("health"), type.health);
	serializeOut(node->addNode("crash_health"), type.crash_health);
	serializeOut(node->addNode("weight"), type.weight);
	serializeOut(node->addNode("armour"), type.armour, type.ArmourDirectionMap);
	serializeOut(node->addNode("passengers"), type.passengers);
	serializeOut(node->addNode("aggressiveness"), type.aggressiveness);
	serializeOut(node->addNode("score"), type.score);
	serializeOut(node->addNode("icon"), type.icon);
	serializeOut(node->addNode("equipment_screen"), type.equipment_screen);
	serializeOut(node->addNode("equip_icon_big"), type.equip_icon_big);
	serializeOut(node->addNode("equip_icon_small"), type.equip_icon_small);
	serializeOut(node->addNode("directional_strategy_sprites"), type.directional_strategy_sprites);
	serializeOut(node->addNode("directional_sprites"), type.directional_sprites, type.BankingMap);
	serializeOut(node->addNode("shadow_offset"), type.shadow_offset);
	serializeOut(node->addNode("directional_shadow_sprites"), type.directional_shadow_sprites);
	serializeOut(node->addNode("animation_sprites"), type.animation_sprites);
	serializeOut(node->addNode("crashed_sprite"), type.crashed_sprite);
	serializeOut(node->addNode("voxelMap"), type.voxelMap);
	serializeOut(node->addNode("equipment_layout_slots"), type.equipment_layout_slots);
	serializeOut(node->addNode("initial_equipment_list"), type.initial_equipment_list);
}

template <> void serializeOut(sp<SerializationNode> node, const Organisation &org)
{
	serializeOut(node->addNode("name"), org.name);
	serializeOut(node->addNode("balance"), org.balance);
	serializeOut(node->addNode("income"), org.income);
	serializeOut(node->addNode("current_relations"), org.current_relations);
}

template <> void serializeOut(sp<SerializationNode> node, const FacilityType &f)
{
	serializeOut(node->addNode("name"), f.name);
	serializeOut(node->addNode("fixed"), f.fixed);
	serializeOut(node->addNode("buildCost"), f.buildCost);
	serializeOut(node->addNode("buildTime"), f.buildTime);
	serializeOut(node->addNode("weeklyCost"), f.weeklyCost);
	serializeOut(node->addNode("capacityType"), f.capacityType, FacilityType::CapacityMap);
	serializeOut(node->addNode("capacityAmount"), f.capacityAmount);
	serializeOut(node->addNode("size"), f.size);
	serializeOut(node->addNode("sprite"), f.sprite);
}

template <> void serializeOut(sp<SerializationNode> node, const DoodadFrame &f)
{
	serializeOut(node->addNode("image"), f.image);
	serializeOut(node->addNode("time"), f.time);
}

template <> void serializeOut(sp<SerializationNode> node, const DoodadType &d)
{
	serializeOut(node->addNode("lifetime"), d.lifetime);
	serializeOut(node->addNode("imageOffset"), d.imageOffset);
	serializeOut(node->addNode("frames"), d.frames);
}

template <> void serializeOut(sp<SerializationNode> node, const VEquipmentType::User &user)
{
	serializeOut(node, user, VEquipmentType::UserMap);
}

template <> void serializeOut(sp<SerializationNode> node, const VEquipmentType &e)
{
	serializeOut(node->addNode("type"), e.type, VEquipmentType::TypeMap);
	serializeOut(node->addNode("id"), e.id);
	serializeOut(node->addNode("name"), e.name);
	serializeOut(node->addNode("weight"), e.weight);
	serializeOut(node->addNode("max_ammo"), e.max_ammo);
	serializeOut(node->addNode("ammo_type"), e.ammo_type);
	serializeOut(node->addNode("equipscreen_sprite"), e.equipscreen_sprite);
	serializeOut(node->addNode("equipscreen_size"), e.equipscreen_size);
	serializeOut(node->addNode("manufacturer"), e.manufacturer);
	serializeOut(node->addNode("store_space"), e.store_space);
	serializeOut(node->addNode("users"), e.users);
	// WeaponOut
	serializeOut(node->addNode("speed"), e.speed);
	serializeOut(node->addNode("projectile_image"), e.projectile_image);
	serializeOut(node->addNode("damage"), e.damage);
	serializeOut(node->addNode("accuracy"), e.accuracy);
	serializeOut(node->addNode("fire_delay"), e.fire_delay);
	serializeOut(node->addNode("tail_size"), e.tail_size);
	serializeOut(node->addNode("guided"), e.guided);
	serializeOut(node->addNode("turn_rate"), e.turn_rate);
	serializeOut(node->addNode("range"), e.range);
	serializeOut(node->addNode("firing_arc_1"), e.firing_arc_1);
	serializeOut(node->addNode("firing_arc_2"), e.firing_arc_2);
	serializeOut(node->addNode("point_defence"), e.point_defence);
	serializeOut(node->addNode("fire_sfx"), e.fire_sfx);
	serializeOut(node->addNode("explosion_graphic"), e.explosion_graphic);
	serializeOut(node->addNode("icon"), e.icon);
	// EngineOut
	serializeOut(node->addNode("power"), e.power);
	serializeOut(node->addNode("top_speed"), e.top_speed);
	// GeneraOut
	serializeOut(node->addNode("accuracy_modifier"), e.accuracy_modifier);
	serializeOut(node->addNode("cargo_space"), e.cargo_space);
	serializeOut(node->addNode("passengers"), e.passengers);
	serializeOut(node->addNode("alien_space"), e.alien_space);
	serializeOut(node->addNode("missile_jamming"), e.missile_jamming);
	serializeOut(node->addNode("shielding"), e.shielding);
	serializeOut(node->addNode("cloaking"), e.cloaking);
	serializeOut(node->addNode("teleporting"), e.teleporting);
}

template <> void serializeOut(sp<SerializationNode> node, const Colour &c)
{
	serializeOut(node->addNode("r"), c.r);
	serializeOut(node->addNode("g"), c.g);
	serializeOut(node->addNode("b"), c.b);
	serializeOut(node->addNode("a"), c.a);
}

template <> void serializeOut(sp<SerializationNode> node, const SceneryTileType &t)
{
	serializeOut(node->addNode("sprite"), t.sprite);
	serializeOut(node->addNode("strategySprite"), t.strategySprite);
	serializeOut(node->addNode("overlaySprite"), t.overlaySprite);
	serializeOut(node->addNode("voxelMap"), t.voxelMap);
	serializeOut(node->addNode("damagedTile"), t.damagedTile);
	serializeOut(node->addNode("imageOffset"), t.imageOffset);
	serializeOut(node->addNode("isLandingPad"), t.isLandingPad);
	serializeOut(node->addNode("minimap_colour"), t.minimap_colour);
}

template <> void serializeOut(sp<SerializationNode> node, const BaseLayout &l)
{
	serializeOut(node->addNode("baseCorridors"), l.baseCorridors);
	serializeOut(node->addNode("baseLift"), l.baseLift);
}

template <> void serializeOut(sp<SerializationNode> node, const Building &b)
{
	serializeOut(node->addNode("name"), b.name);
	serializeOut(node->addNode("owner"), b.owner);
	serializeOut(node->addNode("bounds"), b.bounds);
	serializeOut(node->addNode("base_layout"), b.base_layout);
	// FIXME: Are landing pad locations useful to serialize?
	serializeOut(node->addNode("landingPadLocations"), b.landingPadLocations);
	serializeOut(node->addNode("landed_vehicles"), b.landed_vehicles);
}

template <> void serializeOut(sp<SerializationNode> node, const Scenery &s)
{
	serializeOut(node->addNode("type"), s.type);
	serializeOut(node->addNode("initialPosition"), s.initialPosition);
	serializeOut(node->addNode("currentPosition"), s.currentPosition);
	serializeOut(node->addNode("damaged"), s.damaged);
	serializeOut(node->addNode("falling"), s.falling);
	serializeOut(node->addNode("destroyed"), s.destroyed);
}

template <> void serializeOut(sp<SerializationNode> node, const Doodad &doodad)
{
	serializeOut(node->addNode("position"), doodad.position);
	serializeOut(node->addNode("imageOffset"), doodad.imageOffset);
	serializeOut(node->addNode("temporary"), doodad.temporary);
	serializeOut(node->addNode("age"), doodad.age);
	serializeOut(node->addNode("lifetime"), doodad.lifetime);
	serializeOut(node->addNode("sprite"), doodad.sprite);
	serializeOut(node->addNode("type"), doodad.type);
}

template <> void serializeOut(sp<SerializationNode> node, const ResearchTopic::Type &t)
{
	serializeOut(node, t, ResearchTopic::TypeMap);
}
template <> void serializeOut(sp<SerializationNode> node, const ResearchTopic::LabSize &s)
{
	serializeOut(node, s, ResearchTopic::LabSizeMap);
}

template <> void serializeOut(sp<SerializationNode> node, const Lab &lab)
{
	serializeOut(node->addNode("size"), lab.size);
	serializeOut(node->addNode("type"), lab.type);
	serializeOut(node->addNode("current_project"), lab.current_project);
	serializeOut(node->addNode("assigned_agents"), lab.assigned_agents);
	serializeOut(node->addNode("ticks_since_last_progress"), lab.ticks_since_last_progress);
}

template <> void serializeOut(sp<SerializationNode> node, const Facility &facility)
{
	serializeOut(node->addNode("type"), facility.type);
	serializeOut(node->addNode("pos"), facility.pos);
	serializeOut(node->addNode("buildTime"), facility.buildTime);
	serializeOut(node->addNode("lab"), facility.lab);
}

template <> void serializeOut(sp<SerializationNode> node, const Base &base)
{
	serializeOut(node->addNode("corridors"), base.corridors);
	serializeOut(node->addNode("facilities"), base.facilities);
	serializeOut(node->addNode("inventory"), base.inventory);
	serializeOut(node->addNode("name"), base.name);
	serializeOut(node->addNode("building"), base.building);
}

template <> void serializeOut(sp<SerializationNode> node, const Projectile &p)
{
	serializeOut(node->addNode("type"), p.type, Projectile::TypeMap);
	serializeOut(node->addNode("position"), p.position);
	serializeOut(node->addNode("velocity"), p.velocity);
	serializeOut(node->addNode("age"), p.age);
	serializeOut(node->addNode("lifetime"), p.lifetime);
	serializeOut(node->addNode("firer"), p.firer);
	serializeOut(node->addNode("previousPosition"), p.previousPosition);
	serializeOut(node->addNode("colour"), p.colour);
	serializeOut(node->addNode("beamLength"), p.beamLength);
	serializeOut(node->addNode("beamWidth"), p.beamWidth);
}

template <> void serializeOut(sp<SerializationNode> node, const City &city)
{
	serializeOut(node->addNode("size"), city.size);
	serializeOut(node->addSection("tile_types"), city.tile_types);
	serializeOut(node->addSection("initial_tiles"), city.initial_tiles);
	serializeOut(node->addSection("buildings"), city.buildings);
	serializeOut(node->addSection("scenery"), city.scenery);
	serializeOut(node->addSection("doodads"), city.doodads);
	serializeOut(node->addSection("projectiles"), city.projectiles);
}

template <> void serializeOut(sp<SerializationNode> node, const VehicleMission &m)
{
	serializeOut(node->addNode("type"), m.type, VehicleMission::TypeMap);
	serializeOut(node->addNode("target_location"), m.targetLocation);
	serializeOut(node->addNode("target_building"), m.targetBuilding);
	serializeOut(node->addNode("target_vehicle"), m.targetVehicle);
	serializeOut(node->addNode("time_to_snooze"), m.timeToSnooze);
	serializeOut(node->addNode("current_planned_path"), m.currentPlannedPath);
}

template <> void serializeOut(sp<SerializationNode> node, const VEquipment &e)
{
	serializeOut(node->addNode("type"), e.type);
	serializeOut(node->addNode("equipped_position"), e.equippedPosition);
	serializeOut(node->addNode("weapon_state"), e.weaponState, VEquipment::WeaponStateMap);
	serializeOut(node->addNode("owner"), e.owner);
	serializeOut(node->addNode("ammo"), e.ammo);
	serializeOut(node->addNode("reload_time"), e.reloadTime);
}

template <> void serializeOut(sp<SerializationNode> node, const Vehicle &v)
{
	serializeOut(node->addNode("type"), v.type);
	serializeOut(node->addNode("owner"), v.owner);
	serializeOut(node->addNode("name"), v.name);
	serializeOut(node->addNode("missions"), v.missions);
	serializeOut(node->addNode("equipment"), v.equipment);
	serializeOut(node->addNode("position"), v.position);
	serializeOut(node->addNode("velocity"), v.velocity);
	serializeOut(node->addNode("facing"), v.facing);
	serializeOut(node->addNode("city"), v.city);
	serializeOut(node->addNode("health"), v.health);
	serializeOut(node->addNode("shield"), v.shield);
	serializeOut(node->addNode("homeBuilding"), v.homeBuilding);
	serializeOut(node->addNode("currentlyLandedBuilding"), v.currentlyLandedBuilding);
}

template <> void serializeOut(sp<SerializationNode> node, const ResearchDependency::Type &t)
{
	serializeOut(node, t, ResearchDependency::TypeMap);
}

template <> void serializeOut(sp<SerializationNode> node, const ResearchDependency &d)
{
	serializeOut(node->addNode("type"), d.type);
	serializeOut(node->addNode("topics"), d.topics);
}

template <> void serializeOut(sp<SerializationNode> node, const UfopaediaEntry &e)
{
	serializeOut(node->addNode("title"), e.title);
	serializeOut(node->addNode("description"), e.description);
	serializeOut(node->addNode("background"), e.background);
	serializeOut(node->addNode("data_id"), e.data_id);
	serializeOut(node->addNode("data_type"), e.data_type, UfopaediaEntry::DataMap);
	serializeOut(node->addNode("dependency"), e.dependency);
}

template <> void serializeOut(sp<SerializationNode> node, const UfopaediaCategory &c)
{
	serializeOut(node->addNode("title"), c.title);
	serializeOut(node->addNode("description"), c.description);
	serializeOut(node->addNode("background"), c.background);
	serializeOut(node->addNode("entries"), c.entries);
}

template <> void serializeOut(sp<SerializationNode> node, const ProjectDependencies &d)
{
	serializeOut(node->addNode("research"), d.research);
	serializeOut(node->addNode("items"), d.items);
}

template <> void serializeOut(sp<SerializationNode> node, const ItemDependency &d)
{
	serializeOut(node->addNode("items"), d.items);
}

template <> void serializeOut(sp<SerializationNode> node, const ResearchTopic &r)
{
	serializeOut(node->addNode("name"), r.name);
	serializeOut(node->addNode("description"), r.description);
	serializeOut(node->addNode("ufopaedia_entry"), r.ufopaedia_entry);
	serializeOut(node->addNode("man_hours"), r.man_hours);
	serializeOut(node->addNode("man_hours_progress"), r.man_hours_progress);
	serializeOut(node->addNode("type"), r.type);
	serializeOut(node->addNode("required_lab_size"), r.required_lab_size);
	serializeOut(node->addNode("current_lab"), r.current_lab);
	serializeOut(node->addNode("score"), r.score);
	serializeOut(node->addNode("started"), r.started);
	serializeOut(node->addNode("dependencies"), r.dependencies);
}

// FIXME: These enums don't quite work the same way as the other map style - maybe a common way
// could be used?
template <> void serializeOut(sp<SerializationNode> node, const Agent::Type &t)
{
	serializeOut(node, t, Agent::TypeMap);
}
template <> void serializeOut(sp<SerializationNode> node, const Agent::Gender &g)
{
	serializeOut(node, g, Agent::GenderMap);
}
template <> void serializeOut(sp<SerializationNode> node, const Agent::Species &s)
{
	serializeOut(node, s, Agent::SpeciesMap);
}

template <> void serializeOut(sp<SerializationNode> node, const AgentStats &s)
{
	serializeOut(node->addNode("health"), s.health);
	serializeOut(node->addNode("accuracy"), s.accuracy);
	serializeOut(node->addNode("reactions"), s.reactions);
	serializeOut(node->addNode("speed"), s.speed);
	serializeOut(node->addNode("stamina"), s.stamina);
	serializeOut(node->addNode("bravery"), s.bravery);
	serializeOut(node->addNode("strength"), s.strength);
	serializeOut(node->addNode("psi_energy"), s.psi_energy);
	serializeOut(node->addNode("psi_attack"), s.psi_attack);
	serializeOut(node->addNode("psi_defence"), s.psi_defence);
	serializeOut(node->addNode("physics_skill"), s.physics_skill);
	serializeOut(node->addNode("biochem_skill"), s.biochem_skill);
	serializeOut(node->addNode("engineering_skill"), s.engineering_skill);
}

template <> void serializeOut(sp<SerializationNode> node, const AgentPortrait &p)
{
	serializeOut(node->addNode("photo"), p.photo);
	serializeOut(node->addNode("icon"), p.icon);
}

template <> void serializeOut(sp<SerializationNode> node, const Agent &a)
{
	serializeOut(node->addNode("name"), a.name);
	serializeOut(node->addNode("portrait"), a.portrait);
	serializeOut(node->addNode("type"), a.type, Agent::TypeMap);
	serializeOut(node->addNode("species"), a.species, Agent::SpeciesMap);
	serializeOut(node->addNode("gender"), a.gender, Agent::GenderMap);
	serializeOut(node->addNode("initial_stats"), a.initial_stats);
	serializeOut(node->addNode("current_stats"), a.current_stats);
	serializeOut(node->addNode("home_base"), a.home_base);
	serializeOut(node->addNode("owner"), a.owner);
	serializeOut(node->addNode("assigned_to_lab"), a.assigned_to_lab);
}

template <> void serializeOut(sp<SerializationNode> node, const AgentGenerator &g)
{
	serializeOut(node->addNode("num_created"), g.num_created);
	serializeOut(node->addNode("first_names"), g.first_names);
	serializeOut(node->addNode("second_names"), g.second_names);
	serializeOut(node->addNode("type_chance"), g.type_chance);
	serializeOut(node->addNode("species_chance"), g.species_chance);
	serializeOut(node->addNode("gender_chance"), g.gender_chance);
	serializeOut(node->addNode("portraits"), g.portraits);
	serializeOut(node->addNode("min_stats"), g.min_stats);
	serializeOut(node->addNode("max_stats"), g.max_stats);
}

template <> void serializeOut(sp<SerializationNode> node, const ResearchState &r)
{
	serializeOut(node->addNode("topics"), r.topics);
	serializeOut(node->addNode("labs"), r.labs);
	serializeOut(node->addNode("num_labs_created"), r.num_labs_created);
}

void serializeOut(sp<SerializationNode> node, const GameState &state)
{
	serializeOut(node->addSection("vehicle_types"), state.vehicle_types);
	serializeOut(node->addSection("organisations"), state.organisations);
	serializeOut(node->addSection("facility_types"), state.facility_types);
	serializeOut(node->addSection("doodad_types"), state.doodad_types);
	serializeOut(node->addSection("vehicle_equipment"), state.vehicle_equipment);
	serializeOutSectionMap(node->addSection("cities"), state.cities);
	serializeOut(node->addSection("base_layouts"), state.base_layouts);
	serializeOut(node->addSection("player_bases"), state.player_bases);
	serializeOut(node->addSection("vehicles"), state.vehicles);
	serializeOut(node->addSection("ufopaedia"), state.ufopaedia);
	serializeOut(node->addSection("research"), state.research);
	serializeOut(node->addSection("agents"), state.agents);
	serializeOut(node->addSection("agent_generator"), state.agent_generator);
	serializeOut(node->addNode("initial_agents"), state.initial_agents);
	serializeOut(node->addNode("initial_facilities"), state.initial_facilities);
	serializeOut(node->addNode("current_city"), state.current_city);
	serializeOut(node->addNode("player"), state.player);
	serializeOut(node->addNode("time"), state.time);
}

} // anonymous namespace

bool GameState::saveGame(const UString &path, bool pack)
{
	auto archive = SerializationArchive::createArchive();
	try
	{

		serializeOut(archive->newRoot("", "gamestate"), *this);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\" at %s", e.what(), e.node->getFullPath().c_str());
	}
	archive->write(path, pack);
	return true;
}

bool GameState::loadGame(const UString &path)
{

	auto archive = SerializationArchive::readArchive(path);
	if (!archive)
	{
		LogError("Failed to read \"%s\"", path.c_str());
		return false;
	}

	try
	{
		serializeIn(this, archive->getRoot("", "gamestate"), *this);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\" at %s", e.what(), e.node->getFullPath().c_str());
		return false;
	}
	return true;
}

} // namespace OpenApoc

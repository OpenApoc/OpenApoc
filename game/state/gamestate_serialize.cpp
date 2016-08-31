#include "game/state/gamestate.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/serialization/serialize.h"
#include "framework/trace.h"
#include "game/state/base/facility.h"
#include "game/state/battlemappart.h"
#include "game/state/battlemappart_type.h"
#include "game/state/city/baselayout.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/doodad.h"
#include "game/state/city/projectile.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate_serialize_generated.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/rules/vammo_type.h"
#include "game/state/rules/vequipment_type.h"
#include "library/voxel.h"

namespace OpenApoc
{

/* Avoid out-of-order declarations breaking */
template <typename T> void serializeIn(const GameState *state, sp<SerializationNode> node, T &val);
template <typename T> void serializeOut(sp<SerializationNode> node, const T &val);

template <typename T>
void serializeIn(const GameState *state, sp<SerializationNode> node, T &val,
                 const std::map<T, UString> &valueMap);
template <typename T>
void serializeOut(sp<SerializationNode> node, const T &val, const std::map<T, UString> &valueMap);

void serializeIn(const GameState *, sp<SerializationNode> node, UString &str)
{
	if (!node)
		return;
	str = node->getValue();
}

void serializeIn(const GameState *, sp<SerializationNode> node, unsigned int &val)
{
	if (!node)
		return;
	val = node->getValueUInt();
}

void serializeIn(const GameState *, sp<SerializationNode> node, unsigned char &val)
{
	if (!node)
		return;
	val = node->getValueUChar();
}

void serializeIn(const GameState *, sp<SerializationNode> node, float &val)
{
	if (!node)
		return;
	val = node->getValueFloat();
}

void serializeIn(const GameState *, sp<SerializationNode> node, int &val)
{
	if (!node)
		return;
	val = node->getValueInt();
}

void serializeIn(const GameState *, sp<SerializationNode> node, uint64_t &val)
{
	if (!node)
		return;
	val = node->getValueUInt64();
}

void serializeIn(const GameState *, sp<SerializationNode> node, bool &val)
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

void serializeIn(const GameState *, sp<SerializationNode> node, sp<LazyImage> &ptr)
{
	if (!node)
		return;
	ptr = std::static_pointer_cast<LazyImage>(fw().data->loadImage(node->getValue(), true));
}

void serializeIn(const GameState *, sp<SerializationNode> node, sp<Image> &ptr)
{
	if (!node)
		return;
	ptr = fw().data->loadImage(node->getValue());
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
void serializeIn(const GameState *, sp<SerializationNode> node, T &val,
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
	    UString::format("Invalid enum value for %s: \"%s\"", typeid(T).name(), str), node);
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
		Key key = {};
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
void serializeIn(const GameState *, sp<SerializationNode> node, std::vector<bool> &vector)
{
	if (!node)
		return;
	vector = node->getValueBoolVector();
}

void serializeIn(const GameState *, sp<SerializationNode> node, sp<VoxelSlice> &ptr)
{
	if (!node)
		return;
	ptr = fw().data->loadVoxelSlice(node->getValue());
}

void serializeIn(const GameState *, sp<SerializationNode> node, sp<Sample> &ptr)
{
	if (!node)
		return;
	ptr = fw().data->loadSample(node->getValue());
}

void serializeIn(const GameState *state, sp<SerializationNode> node, VoxelMap &map)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("size"), map.size);
	serializeIn(state, node->getNode("slices"), map.slices);
}

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
		T type = {};
		serializeIn(state, entry, type);
		set.insert(type);
		entry = entry->getNextSiblingOpt("entry");
	}
}

void serializeIn(const GameState *state, sp<SerializationNode> node,
                 UFOIncursion::PrimaryMission &t)
{
	serializeIn(state, node, t, UFOIncursion::primaryMissionMap);
}

void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchDependency::Type &t)
{
	serializeIn(state, node, t, ResearchDependency::TypeMap);
}

void serializeIn(const GameState *state, sp<SerializationNode> node, Colour &c)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("r"), c.r);
	serializeIn(state, node->getNode("g"), c.g);
	serializeIn(state, node->getNode("b"), c.b);
	serializeIn(state, node->getNode("a"), c.a);
}

void serializeIn(const GameState *state, sp<SerializationNode> node, VehicleType::AlignmentX &t)
{
	serializeIn(state, node, t, VehicleType::AlignmentXMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, VehicleType::AlignmentY &t)
{
	serializeIn(state, node, t, VehicleType::AlignmentYMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, Projectile::Type &t)
{
	serializeIn(state, node, t, Projectile::TypeMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, SceneryTileType::TileType &t)
{
	serializeIn(state, node, t, SceneryTileType::TileTypeMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, SceneryTileType::RoadType &t)
{
	serializeIn(state, node, t, SceneryTileType::RoadTypeMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, SceneryTileType::WalkMode &t)
{
	serializeIn(state, node, t, SceneryTileType::WalkModeMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, VEquipmentType::Type &t)
{
	serializeIn(state, node, t, VEquipmentType::TypeMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, FacilityType::Capacity &t)
{
	serializeIn(state, node, t, FacilityType::CapacityMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, VehicleType::Type &t)
{
	serializeIn(state, node, t, VehicleType::TypeMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, UfopaediaEntry::Data &t)
{
	serializeIn(state, node, t, UfopaediaEntry::DataMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node,
                 VehicleType::ArmourDirection &t)
{
	serializeIn(state, node, t, VehicleType::ArmourDirectionMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, VehicleType::Banking &t)
{
	serializeIn(state, node, t, VehicleType::BankingMap);
}

template <>
void serializeIn(const GameState *state, sp<SerializationNode> node,
                 BattleMapPartType::ExplosionType &t)
{
	serializeIn(state, node, t, BattleMapPartType::ExplosionTypeMap);
}

void serializeIn(const GameState *state, sp<SerializationNode> node, BattleMapPartType::Type &t)
{
	serializeIn(state, node, t, BattleMapPartType::TypeMap);
}

void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchTopic::Type &t)
{
	serializeIn(state, node, t, ResearchTopic::TypeMap);
}

void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchTopic::LabSize &s)
{
	serializeIn(state, node, s, ResearchTopic::LabSizeMap);
}

void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchTopic::ItemType &s)
{
	serializeIn(state, node, s, ResearchTopic::ItemTypeMap);
}

void serializeIn(const GameState *state, sp<SerializationNode> node, Vehicle::AttackMode &t)
{
	serializeIn(state, node, t, Vehicle::AttackModeMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, Vehicle::Altitude &t)
{
	serializeIn(state, node, t, Vehicle::AltitudeMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, VEquipment::WeaponState &t)
{
	serializeIn(state, node, t, VEquipment::WeaponStateMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, VehicleMission::MissionType &t)
{
	serializeIn(state, node, t, VehicleMission::TypeMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, Agent::Type &t)
{
	serializeIn(state, node, t, Agent::TypeMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, Agent::Gender &g)
{
	serializeIn(state, node, g, Agent::GenderMap);
}
void serializeIn(const GameState *state, sp<SerializationNode> node, Agent::Species &s)
{
	serializeIn(state, node, s, Agent::SpeciesMap);
}

void serializeIn(const GameState *state, sp<SerializationNode> node, Xorshift128Plus<uint32_t> &t)
{
	if (!node)
		return;

	uint32_t s[2] = {0, 0};
	serializeIn(state, node->getNode("s0"), s[0]);
	serializeIn(state, node->getNode("s1"), s[1]);
	t.setState(s);
}

void serializeOut(sp<SerializationNode> node, const UString &string) { node->setValue(string); }

void serializeOut(sp<SerializationNode> node, const unsigned int &val) { node->setValueUInt(val); }

void serializeOut(sp<SerializationNode> node, const unsigned char &val)
{
	node->setValueUChar(val);
}

void serializeOut(sp<SerializationNode> node, const float &val) { node->setValueFloat(val); }

void serializeOut(sp<SerializationNode> node, const int &val) { node->setValueInt(val); }

void serializeOut(sp<SerializationNode> node, const uint64_t &val) { node->setValueUInt64(val); }

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

template <typename T> void serializeOut(sp<SerializationNode> node, const StateRefMap<T> &map)
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

template <> void serializeOut(sp<SerializationNode> node, const ResearchDependency::Type &t)
{
	serializeOut(node, t, ResearchDependency::TypeMap);
}

template <> void serializeOut(sp<SerializationNode> node, const ResearchDependency &d)
{
	serializeOut(node->addNode("type"), d.type);
	serializeOut(node->addNode("topics"), d.topics);
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
	serializeOut(node->addNode("dependency"), f.dependency);
	serializeOut(node->addNode("ufopaedia_entry"), f.ufopaedia_entry);
}

template <> void serializeOut(sp<SerializationNode> node, const DoodadFrame &f)
{
	serializeOut(node->addNode("image"), f.image);
	serializeOut(node->addNode("time"), f.time);
}

template <> void serializeOut(sp<SerializationNode> node, const DoodadType &d)
{
	serializeOut(node->addNode("lifetime"), d.lifetime);
	serializeOut(node->addNode("repeatable"), d.repeatable);
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
	serializeOut(node->addNode("projectile_sprites"), e.projectile_sprites);
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

template <> void serializeOut(sp<SerializationNode> node, const AEquipmentType &e)
{
	serializeOut(node->addNode("id"), e.id);
	serializeOut(node->addNode("name"), e.name);
	serializeOut(node->addNode("equipscreen_sprite"), e.equipscreen_sprite);
	serializeOut(node->addNode("equipscreen_size"), e.equipscreen_size);
	serializeOut(node->addNode("manufacturer"), e.manufacturer);
	serializeOut(node->addNode("store_space"), e.store_space);
}

template <> void serializeOut(sp<SerializationNode> node, const VAmmoType &e)
{
	serializeOut(node->addNode("id"), e.id);
	serializeOut(node->addNode("name"), e.name);
	serializeOut(node->addNode("weight"), e.weight);
	serializeOut(node->addNode("ammo_id"), e.ammo_id);
	serializeOut(node->addNode("manufacturer"), e.manufacturer);
	serializeOut(node->addNode("store_space"), e.store_space);
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
	serializeOut(node->addNode("tile_type"), t.tile_type, SceneryTileType::TileTypeMap);
	serializeOut(node->addNode("road_type"), t.road_type, SceneryTileType::RoadTypeMap);
	serializeOut(node->addNode("walk_mode"), t.walk_mode, SceneryTileType::WalkModeMap);
	serializeOut(node->addNode("sprite"), t.sprite);
	serializeOut(node->addNode("strategySprite"), t.strategySprite);
	serializeOut(node->addNode("overlaySprite"), t.overlaySprite);
	serializeOut(node->addNode("voxelMap"), t.voxelMap);
	serializeOut(node->addNode("damagedTile"), t.damagedTile);
	serializeOut(node->addNode("imageOffset"), t.imageOffset);
	serializeOut(node->addNode("isLandingPad"), t.isLandingPad);
	serializeOut(node->addNode("minimap_colour"), t.minimap_colour);
	serializeOut(node->addNode("constitution"), t.constitution);
	serializeOut(node->addNode("value"), t.value);
	serializeOut(node->addNode("mass"), t.mass);
	serializeOut(node->addNode("strength"), t.strength);
	serializeOut(node->addNode("isHill"), t.isHill);
}

template <> void serializeOut(sp<SerializationNode> node, const BaseLayout &l)
{
	serializeOut(node->addNode("baseCorridors"), l.baseCorridors);
	serializeOut(node->addNode("baseLift"), l.baseLift);
}

template <> void serializeOut(sp<SerializationNode> node, const Building &b)
{
	serializeOut(node->addNode("name"), b.name);
	serializeOut(node->addNode("function"), b.function);
	serializeOut(node->addNode("owner"), b.owner);
	serializeOut(node->addNode("bounds"), b.bounds);
	serializeOut(node->addNode("base_layout"), b.base_layout);
	// FIXME: Are landing pad locations useful to serialize?
	serializeOut(node->addNode("landingPadLocations"), b.landingPadLocations);
	serializeOut(node->addNode("landed_vehicles"), b.landed_vehicles);
}

template <> void serializeOut(sp<SerializationNode> node, const UFOGrowth &g)
{
	serializeOut(node->addNode("week"), g.week);
	serializeOut(node->addNode("vehicleTypeList"), g.vehicleTypeList);
}

template <> void serializeOut(sp<SerializationNode> node, const UFOIncursion &i)
{
	serializeOut(node->addNode("primaryMission"), i.primaryMission,
	             UFOIncursion::primaryMissionMap);
	serializeOut(node->addNode("primaryList"), i.primaryList);
	serializeOut(node->addNode("escortList"), i.escortList);
	serializeOut(node->addNode("attackList"), i.attackList);
	serializeOut(node->addNode("priority"), i.priority);
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

template <> void serializeOut(sp<SerializationNode> node, const BattleMapPart &s)
{
	serializeOut(node->addNode("type"), s.type);
	serializeOut(node->addNode("initialPosition"), s.initialPosition);
	serializeOut(node->addNode("currentPosition"), s.currentPosition);
	serializeOut(node->addNode("damaged"), s.damaged);
	serializeOut(node->addNode("falling"), s.falling);
	serializeOut(node->addNode("destroyed"), s.destroyed);
}

template <> void serializeOut(sp<SerializationNode> node, const ResearchTopic::Type &t)
{
	serializeOut(node, t, ResearchTopic::TypeMap);
}

template <> void serializeOut(sp<SerializationNode> node, const ResearchTopic::LabSize &s)
{
	serializeOut(node, s, ResearchTopic::LabSizeMap);
}

template <> void serializeOut(sp<SerializationNode> node, const ResearchTopic::ItemType &s)
{
	serializeOut(node, s, ResearchTopic::ItemTypeMap);
}

template <> void serializeOut(sp<SerializationNode> node, const Lab &lab)
{
	serializeOut(node->addNode("size"), lab.size);
	serializeOut(node->addNode("type"), lab.type);
	serializeOut(node->addNode("current_project"), lab.current_project);
	serializeOut(node->addNode("assigned_agents"), lab.assigned_agents);
	serializeOut(node->addNode("ticks_since_last_progress"), lab.ticks_since_last_progress);
	serializeOut(node->addNode("manufacture_done"), lab.manufacture_done);
	serializeOut(node->addNode("manufacture_goal"), lab.manufacture_goal);
	serializeOut(node->addNode("manufacture_man_hours_invested"),
	             lab.manufacture_man_hours_invested);
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
	serializeOut(node->addNode("inventoryAgentEquipment"), base.inventoryAgentEquipment);
	serializeOut(node->addNode("inventoryVehicleEquipment"), base.inventoryVehicleEquipment);
	serializeOut(node->addNode("inventoryVehicleAmmo"), base.inventoryVehicleAmmo);
	serializeOut(node->addNode("name"), base.name);
	serializeOut(node->addNode("building"), base.building);
}

template <> void serializeOut(sp<SerializationNode> node, const Projectile &p)
{
	serializeOut(node->addNode("type"), p.type, Projectile::TypeMap);
	serializeOut(node->addNode("position"), p.position);
	serializeOut(node->addNode("velocity"), p.velocity);
	serializeOut(node->addNode("age"), p.age);
	serializeOut(node->addNode("damage"), p.damage);
	serializeOut(node->addNode("lifetime"), p.lifetime);
	serializeOut(node->addNode("firer"), p.firer);
	serializeOut(node->addNode("previousPosition"), p.previousPosition);
	serializeOut(node->addNode("tail_length"), p.tail_length);
	serializeOut(node->addNode("projectile_sprites"), p.projectile_sprites);
}

template <> void serializeOut(sp<SerializationNode> node, const City &city)
{
	serializeOut(node->addNode("size"), city.size);
	serializeOut(node->addSection("tile_types"), city.tile_types);
	serializeOut(node->addSection("initial_tiles"), city.initial_tiles);
	serializeOut(node->addSection("buildings"), city.buildings);
	serializeOut(node->addSection("scenery"), city.scenery);
	serializeOut(node->addSection("doodads"), city.doodads);
	serializeOut(node->addSection("portals"), city.portals);
	serializeOut(node->addSection("projectiles"), city.projectiles);
}

template <> void serializeOut(sp<SerializationNode> node, const Battle &battle)
{
	serializeOut(node->addNode("size"), battle.size);
	serializeOut(node->addSection("map_part_types"), battle.map_part_types);
	serializeOut(node->addSection("initial_grounds"), battle.initial_grounds);
	serializeOut(node->addSection("initial_left_walls"), battle.initial_left_walls);
	serializeOut(node->addSection("initial_right_walls"), battle.initial_right_walls);
	serializeOut(node->addSection("initial_scenery"), battle.initial_scenery);
	serializeOut(node->addSection("map_parts"), battle.map_parts);
}

template <> void serializeOut(sp<SerializationNode> node, const BattleMapPartType::Type &t)
{
	serializeOut(node, t, BattleMapPartType::TypeMap);
}

template <> void serializeOut(sp<SerializationNode> node, const BattleMapPartType::ExplosionType &t)
{
	serializeOut(node, t, BattleMapPartType::ExplosionTypeMap);
}

template <> void serializeOut(sp<SerializationNode> node, const BattleMapPartType &t)
{
	serializeOut(node->addNode("type"), t.type);
	serializeOut(node->addNode("sprite"), t.sprite);
	serializeOut(node->addNode("strategySprite"), t.strategySprite);
	serializeOut(node->addNode("voxelMapLOF"), t.voxelMapLOF);
	serializeOut(node->addNode("voxelMapLOS"), t.voxelMapLOS);
	serializeOut(node->addNode("imageOffset"), t.imageOffset);
	serializeOut(node->addNode("constitution"), t.constitution);
	serializeOut(node->addNode("explosion_power"), t.explosion_power);
	serializeOut(node->addNode("explosion_radius_divizor"), t.explosion_radius_divizor);
	serializeOut(node->addNode("explosion_type"), t.explosion_type);
	serializeOut(node->addNode("damaged_map_part"), t.damaged_map_part);
	serializeOut(node->addNode("animation_frames"), t.animation_frames);
}

template <> void serializeOut(sp<SerializationNode> node, const VehicleMission &m)
{
	serializeOut(node->addNode("type"), m.type, VehicleMission::TypeMap);
	serializeOut(node->addNode("targetLocation"), m.targetLocation);
	serializeOut(node->addNode("targetBuilding"), m.targetBuilding);
	serializeOut(node->addNode("targetVehicle"), m.targetVehicle);
	serializeOut(node->addNode("timeToSnooze"), m.timeToSnooze);
	serializeOut(node->addNode("missionCounter"), m.missionCounter);
	serializeOut(node->addNode("currentPlannedPath"), m.currentPlannedPath);
}

template <> void serializeOut(sp<SerializationNode> node, const VEquipment &e)
{
	serializeOut(node->addNode("type"), e.type);
	serializeOut(node->addNode("equippedPosition"), e.equippedPosition);
	serializeOut(node->addNode("weaponState"), e.weaponState, VEquipment::WeaponStateMap);
	serializeOut(node->addNode("owner"), e.owner);
	serializeOut(node->addNode("ammo"), e.ammo);
	serializeOut(node->addNode("reload_time"), e.reloadTime);
}

template <> void serializeOut(sp<SerializationNode> node, const Vehicle &v)
{
	serializeOut(node->addNode("type"), v.type);
	serializeOut(node->addNode("owner"), v.owner);
	serializeOut(node->addNode("name"), v.name);
	serializeOut(node->addNode("attackMode"), v.attackMode, Vehicle::AttackModeMap);
	serializeOut(node->addNode("altitude"), v.altitude, Vehicle::AltitudeMap);
	serializeOut(node->addNode("missions"), v.missions);
	serializeOut(node->addNode("equipment"), v.equipment);
	serializeOut(node->addNode("position"), v.position);
	serializeOut(node->addNode("velocity"), v.velocity);
	serializeOut(node->addNode("facing"), v.facing);
	serializeOut(node->addNode("city"), v.city);
	serializeOut(node->addNode("health"), v.health);
	serializeOut(node->addNode("shield"), v.shield);
	serializeOut(node->addNode("shieldRecharge"), v.shieldRecharge);
	serializeOut(node->addNode("homeBuilding"), v.homeBuilding);
	serializeOut(node->addNode("currentlyLandedBuilding"), v.currentlyLandedBuilding);
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
	// Shared Research & Manufacture
	serializeOut(node->addNode("name"), r.name);
	serializeOut(node->addNode("order"), r.order);
	serializeOut(node->addNode("description"), r.description);
	serializeOut(node->addNode("man_hours"), r.man_hours);
	serializeOut(node->addNode("type"), r.type);
	serializeOut(node->addNode("required_lab_size"), r.required_lab_size);
	serializeOut(node->addNode("dependencies"), r.dependencies);
	// Research only
	if (r.type == ResearchTopic::Type::BioChem || r.type == ResearchTopic::Type::Physics)
	{
		serializeOut(node->addNode("ufopaedia_entry"), r.ufopaedia_entry);
		serializeOut(node->addNode("man_hours_progress"), r.man_hours_progress);
		serializeOut(node->addNode("current_lab"), r.current_lab);
		serializeOut(node->addNode("score"), r.score);
		serializeOut(node->addNode("started"), r.started);
	}
	// Manufacture only
	if (r.type == ResearchTopic::Type::Engineering)
	{
		serializeOut(node->addNode("cost"), r.cost);
		serializeOut(node->addNode("item_type"), r.item_type);
		serializeOut(node->addNode("item_produced"), r.item_produced);
	}
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

void serializeOut(sp<SerializationNode> node, const GameTime &time)
{
	serializeOut(node->addNode("ticks"), time.getTicks());
}

template <> void serializeOut(sp<SerializationNode> node, const Xorshift128Plus<uint32_t> &t)
{
	if (!node)
		return;

	uint32_t s[2] = {0, 0};
	t.getState(s);
	serializeOut(node->addNode("s0"), s[0]);
	serializeOut(node->addNode("s1"), s[1]);
}

template <> void serializeOut(sp<SerializationNode> node, const EventMessage &m)
{
	serializeOut(node->addNode("time"), m.time);
	serializeOut(node->addNode("text"), m.text);
	serializeOut(node->addNode("location"), m.location);
}

void serializeOut(sp<SerializationNode> node, const GameState &state)
{
	serializeOut(node->addNode("lastVehicle"), state.lastVehicle);
	serializeOut(node->addSection("vehicle_types"), state.vehicle_types);
	serializeOut(node->addSection("organisations"), state.organisations);
	serializeOut(node->addSection("facility_types"), state.facility_types);
	serializeOut(node->addSection("doodad_types"), state.doodad_types);
	serializeOut(node->addSection("vehicle_equipment"), state.vehicle_equipment);
	serializeOut(node->addSection("agent_equipment"), state.agent_equipment);
	serializeOut(node->addSection("vehicle_ammo"), state.vehicle_ammo);
	serializeOutSectionMap(node->addSection("cities"), state.cities);
	serializeOut(node->addSection("ufo_growth_lists"), state.ufo_growth_lists);
	serializeOut(node->addSection("ufo_incursions"), state.ufo_incursions);
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
	serializeOut(node->addNode("current_base"), state.current_base);
	serializeOut(node->addNode("player"), state.player);
	serializeOut(node->addNode("rng"), state.rng);
	serializeOut(node->addNode("gameTime"), state.gameTime);
	serializeOut(node->addSection("battle"), state.battle);
	serializeOut(node->addNode("messages"), state.messages);
}

bool GameState::saveGame(const UString &path, bool pack)
{
	TRACE_FN_ARGS1("path", path);
	auto archive = SerializationArchive::createArchive();
	if (serialize(archive))
	{
		archive->write(path, pack);
		return true;
	}
	return false;
}

bool GameState::loadGame(const UString &path)
{

	TRACE_FN_ARGS1("path", path);
	auto archive = SerializationArchive::readArchive(path);
	if (!archive)
	{
		LogError("Failed to read \"%s\"", path.cStr());
		return false;
	}

	return deserialize(archive);
}

bool GameState::serialize(sp<SerializationArchive> archive) const
{
	try
	{
		serializeOut(archive->newRoot("", "gamestate"), *this);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\" at %s", e.what(), e.node->getFullPath().cStr());
		return false;
	}
	return true;
}

bool GameState::deserialize(const sp<SerializationArchive> archive)
{
	try
	{
		serializeIn(this, archive->getRoot("", "gamestate"), *this);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\" at %s", e.what(), e.node->getFullPath().cStr());
		return false;
	}
	return true;
}

} // namespace OpenApoc

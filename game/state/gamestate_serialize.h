#pragma once

#include "framework/framework.h"
#include "framework/image.h"
#include "framework/serialization/serialize.h"
#include "framework/trace.h"
#include "game/state/base/base.h"
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
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize_generated.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/rules/vammo_type.h"
#include "game/state/rules/vequipment_type.h"
#include "library/voxel.h"

namespace OpenApoc
{

template <typename T> bool operator==(const StateRefMap<T> &a, const StateRefMap<T> &b)
{
	if (a.size() != b.size())
	{
		return false;
	}
	auto itA = a.begin();
	auto itB = b.begin();
	while (itA != a.end() && itB != b.end())
	{
		if (itA->first != itB->first)
		{
			return false;
		}
		if (*itA->second != *itB->second)
		{
			return false;
		}
		itA++;
		itB++;
	}
	return true;
}

template <typename T> bool operator!=(const StateRefMap<T> &a, const StateRefMap<T> &b)

{
	return !(a == b);
}

template <typename T> bool operator==(const sp<T> &a, const sp<T> &b)
{
	if (a.get() == b.get())
	{
		return true;
	}
	if (!a || !b)
	{
		return false;
	}
	return *a == *b;
}

template <typename T> bool operator!=(const sp<T> &a, const sp<T> &b) { return !(a == b); }

void serializeIn(const GameState *, sp<SerializationNode> node, UString &str);

void serializeIn(const GameState *, sp<SerializationNode> node, unsigned int &val);

void serializeIn(const GameState *, sp<SerializationNode> node, unsigned char &val);

void serializeIn(const GameState *, sp<SerializationNode> node, float &val);

void serializeIn(const GameState *, sp<SerializationNode> node, int &val);

void serializeIn(const GameState *, sp<SerializationNode> node, uint64_t &val);

void serializeIn(const GameState *, sp<SerializationNode> node, bool &val);

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

void serializeIn(const GameState *, sp<SerializationNode> node, sp<LazyImage> &ptr);

void serializeIn(const GameState *, sp<SerializationNode> node, sp<Image> &ptr);

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
void serializeIn(const GameState *, sp<SerializationNode> node, std::vector<bool> &vector);

void serializeIn(const GameState *state, sp<SerializationNode> node,
                 VehicleType::EquipmentLayoutSlot &slot);

void serializeIn(const GameState *, sp<SerializationNode> node, sp<VoxelSlice> &ptr);

void serializeIn(const GameState *, sp<SerializationNode> node, sp<Sample> &ptr);

void serializeIn(const GameState *state, sp<SerializationNode> node, VoxelMap &map);

void serializeIn(const GameState *state, sp<SerializationNode> node, VEquipmentType::User &user);

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

void serializeIn(const GameState *state, sp<SerializationNode> node, VehicleType::AlignmentX &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, VehicleType::AlignmentY &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, Projectile::Type &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, SceneryTileType::TileType &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, SceneryTileType::RoadType &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, SceneryTileType::WalkMode &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, VEquipmentType::Type &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, FacilityType::Capacity &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, VehicleType::Type &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, UfopaediaEntry::Data &t);
void serializeIn(const GameState *state, sp<SerializationNode> node,
                 VehicleType::ArmourDirection &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, VehicleType::Banking &t);

void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchDependency::Type &t);

void serializeIn(const GameState *state, sp<SerializationNode> node, BattleMapPartType::Type &t);

void serializeIn(const GameState *state, sp<SerializationNode> node, BattleMapPartType &t);

void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchTopic::Type &t);

void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchTopic::LabSize &s);

void serializeIn(const GameState *state, sp<SerializationNode> node, ResearchTopic::ItemType &s);

void serializeIn(const GameState *state, sp<SerializationNode> node, Vehicle::AttackMode &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, Vehicle::Altitude &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, VEquipment::WeaponState &t);
void serializeIn(const GameState *state, sp<SerializationNode> node,
                 VehicleMission::MissionType &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, Agent::Type &t);
void serializeIn(const GameState *state, sp<SerializationNode> node, Agent::Gender &g);
void serializeIn(const GameState *state, sp<SerializationNode> node, Agent::Species &s);
void serializeIn(const GameState *state, sp<SerializationNode> node, Colour &c);
void serializeIn(const GameState *state, sp<SerializationNode> node,
                 UFOIncursion::PrimaryMission &t);

void serializeIn(const GameState *state, sp<SerializationNode> node, Xorshift128Plus<uint32_t> &t);

} // namespace OpenApoc

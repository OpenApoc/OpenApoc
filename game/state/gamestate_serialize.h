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
		if (!itA->second || !itB->second)
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
void serializeIn(const GameState *, sp<SerializationNode> node, sp<LazyImage> &ptr);
void serializeIn(const GameState *, sp<SerializationNode> node, sp<Image> &ptr);
void serializeIn(const GameState *, sp<SerializationNode> node, std::vector<bool> &vector);
void serializeIn(const GameState *, sp<SerializationNode> node, sp<VoxelSlice> &ptr);
void serializeIn(const GameState *, sp<SerializationNode> node, sp<Sample> &ptr);
void serializeIn(const GameState *state, sp<SerializationNode> node, VoxelMap &map);
void serializeIn(const GameState *state, sp<SerializationNode> node, Colour &c);
void serializeIn(const GameState *state, sp<SerializationNode> node, Xorshift128Plus<uint32_t> &t);

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

template <typename T>
void serializeOut(sp<SerializationNode> node, const T &val, const T &,
                  const std::map<T, UString> &valueMap)
{
	auto it = valueMap.find(val);
	if (it == valueMap.end())
	{
		LogError("Invalid enum value for %s: %d", typeid(T).name(), (int)val);
	}
	node->setValue(it->second);
}

void serializeOut(sp<SerializationNode> node, const UString &str, const UString &ref);
void serializeOut(sp<SerializationNode> node, const unsigned int &val, const unsigned int &ref);
void serializeOut(sp<SerializationNode> node, const unsigned char &val, const unsigned char &ref);
void serializeOut(sp<SerializationNode> node, const float &val, const float &ref);
void serializeOut(sp<SerializationNode> node, const int &val, const int &ref);
void serializeOut(sp<SerializationNode> node, const uint64_t &val, const uint64_t &ref);
void serializeOut(sp<SerializationNode> node, const bool &val, const bool &ref);
void serializeOut(sp<SerializationNode> node, const sp<LazyImage> &ptr, const sp<LazyImage> &ref);
void serializeOut(sp<SerializationNode> node, const sp<Image> &ptr, const sp<Image> &ref);
void serializeOut(sp<SerializationNode> node, const std::vector<bool> &vector,
                  const std::vector<bool> &ref);
void serializeOut(sp<SerializationNode> node, const sp<VoxelSlice> &ptr, const sp<VoxelSlice> &ref);
void serializeOut(sp<SerializationNode> node, const sp<Sample> &ptr, const sp<Sample> &ref);
void serializeOut(sp<SerializationNode> node, const VoxelMap &map, const VoxelMap &ref);
void serializeOut(sp<SerializationNode> node, const Colour &c, const Colour &ref);
void serializeOut(sp<SerializationNode> node, const Xorshift128Plus<uint32_t> &t,
                  const Xorshift128Plus<uint32_t> &ref);
template <typename T>
void serializeOut(sp<SerializationNode> node, const StateRef<T> &val, const StateRef<T> &)
{
	// node->setValue("") causes an extra unnecessary <tag></tag> instead of <tag />
	if (val.id != "")
		node->setValue(val.id);
}

template <typename T>
void serializeOut(sp<SerializationNode> node, const Vec3<T> &val, const Vec3<T> &ref)
{
	serializeOut(node->addNode("x"), val.x, ref.x);
	serializeOut(node->addNode("y"), val.y, ref.y);
	serializeOut(node->addNode("z"), val.z, ref.z);
}

template <typename T>
void serializeOut(sp<SerializationNode> node, const Vec2<T> &val, const Vec2<T> &ref)
{
	serializeOut(node->addNode("x"), val.x, ref.x);
	serializeOut(node->addNode("y"), val.y, ref.y);
}

template <typename T>
void serializeOut(sp<SerializationNode> node, const Rect<T> &val, const Rect<T> &ref)
{
	serializeOut(node->addNode("p0"), val.p0, ref.p0);
	serializeOut(node->addNode("p1"), val.p1, ref.p1);
}

template <typename T>
void serializeOut(sp<SerializationNode> node, const up<T> &ptr, const up<T> &ref)
{
	if (ptr)
	{
		if (ref)
			serializeOut(node->addNode("up"), *ptr, *ref);
		else
		{
			T defaultRef;
			serializeOut(node->addNode("up"), *ptr, defaultRef);
		}
	}
}

template <typename T>
void serializeOut(sp<SerializationNode> node, const sp<T> &ptr, const sp<T> &ref)
{
	if (ptr)
	{
		if (ref)
			serializeOut(node->addNode("sp"), *ptr, *ref);
		else
		{
			T defaultRef;
			serializeOut(node->addNode("sp"), *ptr, defaultRef);
		}
	}
}

template <typename Key, typename Value>
void serializeOut(sp<SerializationNode> node, const std::map<Key, Value> &map,
                  const std::map<Key, Value> &ref)
{
	Key defaultKey;
	Value defaultValue;
	for (const auto &pair : map)
	{
		auto refIt = ref.find(pair.first);
		if (refIt != ref.end())
		{
			if (refIt->second != pair.second)
			{
				auto entry = node->addNode("entry");
				serializeOut(entry->addNode("key"), pair.first, defaultKey);
				serializeOut(entry->addNode("value"), pair.second, refIt->second);
			}
			else
			{
				// key and value unchanged, nothing serialized out
			}
		}
		else
		{
			auto entry = node->addNode("entry");
			serializeOut(entry->addNode("key"), pair.first, defaultKey);
			serializeOut(entry->addNode("value"), pair.second, defaultValue);
		}
	}
}

template <typename T>
void serializeOut(sp<SerializationNode> node, const StateRefMap<T> &map, const StateRefMap<T> &ref)
{
	UString defaultKey;
	sp<T> defaultValue;
	for (const auto &pair : map)
	{
		auto refIt = ref.find(pair.first);
		if (refIt != ref.end())
		{
			if (refIt->second != pair.second)
			{
				auto entry = node->addNode("entry");
				serializeOut(entry->addNode("key"), pair.first, defaultKey);
				serializeOut(entry->addNode("value"), pair.second, refIt->second);
			}
			else
			{
				// key and value unchanged, nothing serialized out
			}
		}
		else
		{
			auto entry = node->addNode("entry");
			serializeOut(entry->addNode("key"), pair.first, defaultKey);
			serializeOut(entry->addNode("value"), pair.second, defaultValue);
		}
	}
}

template <typename Value>
void serializeOutSectionMap(sp<SerializationNode> node, const std::map<UString, Value> &map,
                            const std::map<UString, Value> &ref)
{
	UString defaultKey;
	Value defaultValue;
	for (const auto &pair : map)
	{
		auto refIt = ref.find(pair.first);
		if (refIt != ref.end())
		{
			if (refIt->second != pair.second)
			{
				auto entry = node->addNode("entry");
				serializeOut(entry->addNode("key"), pair.first, defaultKey);
				serializeOut(entry->addSection(pair.first), pair.second, refIt->second);
			}
			else
			{
				// key and value unchanged, nothing serialized out
			}
		}
		else
		{
			auto entry = node->addNode("entry");
			serializeOut(entry->addNode("key"), pair.first, defaultKey);
			serializeOut(entry->addSection(pair.first), pair.second, defaultValue);
		}
	}
}

template <typename T>
void serializeOut(sp<SerializationNode> node, const std::set<T> &set, const std::set<T> &)
{
	T defaultRef;
	for (const auto &entry : set)
	{
		serializeOut(node->addNode("entry"), entry, defaultRef);
	}
}

template <typename A, typename B>
void serializeOut(sp<SerializationNode> node, const std::pair<A, B> &pair,
                  const std::pair<A, B> &ref)
{
	serializeOut(node->addNode("first"), pair.first, ref.first);
	serializeOut(node->addNode("second"), pair.second, ref.second);
}

template <typename T>
void serializeOut(sp<SerializationNode> node, const std::list<T> &list, const std::list<T> &)
{
	T defaultRef;
	for (auto &entry : list)
	{
		serializeOut(node->addNode("entry"), entry, defaultRef);
	}
}

template <typename T>
void serializeOut(sp<SerializationNode> node, const std::vector<T> &vector, const std::vector<T> &)
{
	T defaultRef;
	for (auto &entry : vector)
	{
		serializeOut(node->addNode("entry"), entry, defaultRef);
	}
}

} // namespace OpenApoc

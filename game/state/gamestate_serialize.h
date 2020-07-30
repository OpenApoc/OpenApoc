#pragma once

#include "framework/framework.h"
#include "framework/image.h"
#include "framework/serialization/serialize.h"
#include "framework/sound.h"
#include "game/state/battle/ai/aitype.h"
#include "game/state/battle/ai/tacticalaivanilla.h"
#include "game/state/battle/ai/unitaibehavior.h"
#include "game/state/battle/ai/unitaidefault.h"
#include "game/state/battle/ai/unitaihardcore.h"
#include "game/state/battle/ai/unitailowmorale.h"
#include "game/state/battle/ai/unitaivanilla.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battleexplosion.h"
#include "game/state/battle/battlehazard.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlescanner.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/agentmission.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/economyinfo.h"
#include "game/state/city/facility.h"
#include "game/state/city/research.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/gametime.h"
#include "game/state/message.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlecommonimagelist.h"
#include "game/state/rules/battle/battlecommonsamplelist.h"
#include "game/state/rules/battle/battlemap.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/rules/battle/battlemaptileset.h"
#include "game/state/rules/battle/battleunitanimationpack.h"
#include "game/state/rules/battle/battleunitimagepack.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/rules/city/baselayout.h"
#include "game/state/rules/city/citycommonimagelist.h"
#include "game/state/rules/city/citycommonsamplelist.h"
#include "game/state/rules/city/facilitytype.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/rules/city/ufogrowth.h"
#include "game/state/rules/city/ufoincursion.h"
#include "game/state/rules/city/ufomissionpreference.h"
#include "game/state/rules/city/ufopaedia.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/rules/doodadtype.h"
#include "game/state/shared/aequipment.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/doodad.h"
#include "game/state/shared/organisation.h"
#include "game/state/shared/projectile.h"
#include "game/state/stateobject.h"
#include "library/strings_format.h"
#include "library/voxel.h"

// Include the generated file last to ensure all types are defined
#include "game/state/gamestate_serialize_generated.h"

namespace OpenApoc
{

static const UString DELETE_OP_ATTRIBUTE = "delete";

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

void serializeIn(const GameState *, SerializationNode *node, UString &str);
void serializeIn(const GameState *, SerializationNode *node, unsigned int &val);
void serializeIn(const GameState *, SerializationNode *node, unsigned char &val);
void serializeIn(const GameState *, SerializationNode *node, float &val);
void serializeIn(const GameState *, SerializationNode *node, int &val);
void serializeIn(const GameState *, SerializationNode *node, uint64_t &val);
void serializeIn(const GameState *, SerializationNode *node, bool &val);
void serializeIn(const GameState *, SerializationNode *node, sp<LazyImage> &ptr);
void serializeIn(const GameState *, SerializationNode *node, sp<Image> &ptr);
void serializeIn(const GameState *, SerializationNode *node, std::vector<bool> &vector);
void serializeIn(const GameState *, SerializationNode *node, sp<VoxelSlice> &ptr);
void serializeIn(const GameState *, SerializationNode *node, sp<Sample> &ptr);
void serializeIn(const GameState *state, SerializationNode *node, VoxelMap &map);
void serializeIn(const GameState *state, SerializationNode *node, Colour &c);
void serializeIn(const GameState *state, SerializationNode *node, Xorshift128Plus<uint32_t> &t);

template <typename T>
void serializeIn(const GameState *state, SerializationNode *node, StateRef<T> &ref)
{
	if (!node)
		return;
	ref = StateRef<T>{state, node->getValue()};
}

template <typename T> void serializeIn(const GameState *state, SerializationNode *node, up<T> &ptr)
{
	if (!node)
		return;
	if (!ptr)
	{
		ptr.reset(new T);
	}
	serializeIn(state, node, *ptr);
}

template <typename T> void serializeIn(const GameState *state, SerializationNode *node, sp<T> &ptr)
{
	if (!node)
		return;
	if (!ptr)
	{
		ptr = std::make_shared<T>();
	}
	serializeIn(state, node, *ptr);
}

template <typename T>
void serializeIn(const GameState *state, SerializationNode *node, Vec3<T> &val)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("x"), val.x);
	serializeIn(state, node->getNode("y"), val.y);
	serializeIn(state, node->getNode("z"), val.z);
}

template <typename T>
void serializeIn(const GameState *state, SerializationNode *node, Vec2<T> &val)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("x"), val.x);
	serializeIn(state, node->getNode("y"), val.y);
}

template <typename T>
void serializeIn(const GameState *state, SerializationNode *node, Rect<T> &val)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("p0"), val.p0);
	serializeIn(state, node->getNode("p1"), val.p1);
}

template <typename T>
void serializeIn(const GameState *, SerializationNode *node, T &val,
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
	throw SerializationException(format("Invalid enum value for %s: \"%s\"", typeid(T).name(), str),
	                             node);
}

template <typename Key, typename Value>
void serializeIn(const GameState *state, SerializationNode *node, std::map<Key, Value> &map)
{
	if (!node)
		return;

	const auto delete_map = (node->getAttribute("op") == DELETE_OP_ATTRIBUTE);
	if (delete_map)
	{
		map.clear();
	}
	auto entry = node->getNodeOpt("entry");
	while (entry)
	{
		Key key;
		const auto keyNode = entry->getNodeReq("key");
		serializeIn(state, keyNode, key);

		const auto delete_node = (entry->getAttribute("op") == DELETE_OP_ATTRIBUTE);

		if (delete_node)
		{
			map.erase(key);
		}

		const auto valueNode = entry->getNodeOpt("value");
		if (valueNode)
		{
			auto &value = map[key];
			serializeIn(state, valueNode, value);
		}

		entry = entry->getNextSiblingOpt("entry");
	}
}

template <typename Value>
void serializeInSectionMap(const GameState *state, SerializationNode *node,
                           std::map<UString, Value> &map)
{
	if (!node)
		return;
	const auto delete_map = (node->getAttribute("op") == DELETE_OP_ATTRIBUTE);
	if (delete_map)
	{
		map.clear();
	}
	auto entry = node->getNodeOpt("entry");
	while (entry)
	{
		UString key;
		const auto keyNode = entry->getNodeReq("key");
		serializeIn(state, keyNode, key);

		const auto delete_node = (entry->getAttribute("op") == DELETE_OP_ATTRIBUTE);
		if (delete_node)
		{
			map.erase(key);
		}

		const auto sectionNode = entry->getSectionOpt(key.c_str());
		if (sectionNode)
		{

			auto &value = map[key];
			serializeIn(state, sectionNode, value);
		}

		entry = entry->getNextSiblingOpt("entry");
	}
}

template <typename Key, typename Value>
void serializeIn(const GameState *state, SerializationNode *node, std::map<Key, Value> &map,
                 const std::map<Key, UString> &keyMap)
{
	if (!node)
		return;
	const auto delete_map = (node->getAttribute("op") == DELETE_OP_ATTRIBUTE);
	if (delete_map)
	{
		map.clear();
	}
	auto entry = node->getNodeOpt("entry");
	while (entry)
	{
		Key key = {};
		const auto keyNode = entry->getNodeReq("key");
		serializeIn(state, keyNode, key);

		const auto delete_node = (entry->getAttribute("op") == DELETE_OP_ATTRIBUTE);
		if (delete_node)
		{
			map.erase(key);
		}

		const auto valueNode = entry->getNodeReq("value");
		if (valueNode)
		{

			auto &value = map[key];
			serializeIn(state, valueNode, value);
		}

		entry = entry->getNextSiblingOpt("entry");
	}
}

template <typename A, typename B>
void serializeIn(const GameState *state, SerializationNode *node, std::pair<A, B> &pair)
{
	if (!node)
		return;
	serializeIn(state, node->getNodeReq("first"), pair.first);
	serializeIn(state, node->getNodeReq("second"), pair.second);
}

template <typename T>
void serializeIn(const GameState *state, SerializationNode *node, std::list<T> &list)
{
	if (!node)
		return;
	const auto delete_list = (node->getAttribute("op") == DELETE_OP_ATTRIBUTE);
	if (delete_list)
	{
		list.clear();
	}
	auto entry = node->getNodeOpt("entry");
	while (entry)
	{
		list.emplace_back();
		serializeIn(state, entry, list.back());
		entry = entry->getNextSiblingOpt("entry");
	}
}

template <typename T>
void serializeIn(const GameState *state, SerializationNode *node, std::vector<T> &vector)
{
	if (!node)
		return;
	const auto delete_vector = (node->getAttribute("op") == DELETE_OP_ATTRIBUTE);
	if (delete_vector)
	{
		vector.clear();
	}
	auto entry = node->getNodeOpt("entry");
	uint64_t sizeHint = 0;
	serializeIn(state, node->getNodeOpt("sizeHint"), sizeHint);
	if (sizeHint)
	{
		vector.reserve(vector.size() + sizeHint);
	}
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
void serializeIn(const GameState *state, SerializationNode *node, std::set<T> &set)
{
	if (!node)
		return;
	const auto delete_set = (node->getAttribute("op") == DELETE_OP_ATTRIBUTE);
	if (delete_set)
	{
		set.clear();
	}
	auto entry = node->getNodeOpt("entry");
	while (entry)
	{
		T type = {};
		serializeIn(state, entry, type);
		const auto delete_entry = (entry->getAttribute("op") == DELETE_OP_ATTRIBUTE);
		if (delete_entry)
		{
			set.erase(type);
		}
		else
		{
			set.insert(type);
		}
		entry = entry->getNextSiblingOpt("entry");
	}
}

template <typename T>
void serializeOut(SerializationNode *node, const T &val, const T &,
                  const std::map<T, UString> &valueMap)
{
	auto it = valueMap.find(val);
	if (it == valueMap.end())
	{
		LogError("Invalid enum value for %s: %d", typeid(T).name(), (int)val);
	}
	node->setValue(it->second);
}

void serializeOut(SerializationNode *node, const UString &str, const UString &ref);
void serializeOut(SerializationNode *node, const unsigned int &val, const unsigned int &ref);
void serializeOut(SerializationNode *node, const unsigned char &val, const unsigned char &ref);
void serializeOut(SerializationNode *node, const float &val, const float &ref);
void serializeOut(SerializationNode *node, const int &val, const int &ref);
void serializeOut(SerializationNode *node, const uint64_t &val, const uint64_t &ref);
void serializeOut(SerializationNode *node, const bool &val, const bool &ref);
void serializeOut(SerializationNode *node, const sp<LazyImage> &ptr, const sp<LazyImage> &ref);
void serializeOut(SerializationNode *node, const sp<Image> &ptr, const sp<Image> &ref);
void serializeOut(SerializationNode *node, const std::vector<bool> &vector,
                  const std::vector<bool> &ref);
void serializeOut(SerializationNode *node, const sp<VoxelSlice> &ptr, const sp<VoxelSlice> &ref);
void serializeOut(SerializationNode *node, const sp<Sample> &ptr, const sp<Sample> &ref);
void serializeOut(SerializationNode *node, const VoxelMap &map, const VoxelMap &ref);
void serializeOut(SerializationNode *node, const Colour &c, const Colour &ref);
void serializeOut(SerializationNode *node, const Xorshift128Plus<uint32_t> &t,
                  const Xorshift128Plus<uint32_t> &ref);

template <typename T>
void serializeOut(SerializationNode *node, const StateRef<T> &val, const StateRef<T> &)
{
	// node->setValue("") causes an extra unnecessary <tag></tag> instead of <tag />
	if (val.id != "")
		node->setValue(val.id);
}

template <typename T>
void serializeOut(SerializationNode *node, const Vec3<T> &val, const Vec3<T> &ref)
{
	serializeOut(node->addNode("x"), val.x, ref.x);
	serializeOut(node->addNode("y"), val.y, ref.y);
	serializeOut(node->addNode("z"), val.z, ref.z);
}

template <typename T>
void serializeOut(SerializationNode *node, const Vec2<T> &val, const Vec2<T> &ref)
{
	serializeOut(node->addNode("x"), val.x, ref.x);
	serializeOut(node->addNode("y"), val.y, ref.y);
}

template <typename T>
void serializeOut(SerializationNode *node, const Rect<T> &val, const Rect<T> &ref)
{
	serializeOut(node->addNode("p0"), val.p0, ref.p0);
	serializeOut(node->addNode("p1"), val.p1, ref.p1);
}

template <typename T> void serializeOut(SerializationNode *node, const up<T> &ptr, const up<T> &ref)
{
	if (ptr)
	{
		if (ref)
			serializeOut(node, *ptr, *ref);
		else
		{
			const T defaultRef{};
			serializeOut(node, *ptr, defaultRef);
		}
	}
}

template <typename T> void serializeOut(SerializationNode *node, const sp<T> &ptr, const sp<T> &ref)
{
	if (ptr)
	{
		if (ref)
			serializeOut(node, *ptr, *ref);
		else
		{
			// Here we need to mksp<>, as type T may have a protected destructor (e.g.
			// shared_from_this-enabled mobjects)

			const sp<T> defaultRef = mksp<T>();
			serializeOut(node, *ptr, *defaultRef);
		}
	}
}

template <typename Key, typename Value>
void serializeOut(SerializationNode *node, const std::map<Key, Value> &map,
                  const std::map<Key, Value> &ref)
{
	const Key defaultKey{};
	const Value defaultValue{};

	if (map.empty() && !ref.empty())
	{
		node->setAttribute("op", DELETE_OP_ATTRIBUTE);
		return;
	}
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
	// Find any removed entries
	for (const auto &pair : ref)
	{
		auto mapIt = map.find(pair.first);
		if (mapIt == map.end())
		{
			auto entry = node->addNode("entry");
			serializeOut(entry->addNode("key"), pair.first, defaultKey);
			entry->setAttribute("op", DELETE_OP_ATTRIBUTE);
		}
	}
}

template <typename T>
void serializeOut(SerializationNode *node, const StateRefMap<T> &map, const StateRefMap<T> &ref)
{
	const UString defaultKey{};
	const sp<T> defaultValue{};
	if (map.empty() && !ref.empty())
	{
		node->setAttribute("op", DELETE_OP_ATTRIBUTE);
		return;
	}
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
	// Find any removed entries
	for (const auto &pair : ref)
	{
		auto mapIt = map.find(pair.first);
		if (mapIt == map.end())
		{
			auto entry = node->addNode("entry");
			serializeOut(entry->addNode("key"), pair.first, defaultKey);
			entry->setAttribute("op", DELETE_OP_ATTRIBUTE);
		}
	}
}

template <typename Value>
void serializeOutSectionMap(SerializationNode *node, const std::map<UString, Value> &map,
                            const std::map<UString, Value> &ref)
{
	const UString defaultKey{};
	const Value defaultValue{};
	if (map.empty() && !ref.empty())
	{
		node->setAttribute("op", DELETE_OP_ATTRIBUTE);
		return;
	}
	for (const auto &pair : map)
	{
		auto refIt = ref.find(pair.first);
		if (refIt != ref.end())
		{
			if (refIt->second != pair.second)
			{
				auto entry = node->addNode("entry");
				serializeOut(entry->addNode("key"), pair.first, defaultKey);
				serializeOut(entry->addSection(pair.first.c_str()), pair.second, refIt->second);
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
			serializeOut(entry->addSection(pair.first.c_str()), pair.second, defaultValue);
		}
	}
	// Find any removed entries
	for (const auto &pair : ref)
	{
		auto mapIt = map.find(pair.first);
		if (mapIt == map.end())
		{
			auto entry = node->addNode("entry");
			serializeOut(entry->addNode("key"), pair.first, defaultKey);
			entry->setAttribute("op", DELETE_OP_ATTRIBUTE);
		}
	}
}

template <typename T>
void serializeOut(SerializationNode *node, const std::set<T> &set, const std::set<T> &ref)
{
	const T defaultRef{};
	// If ref isn't simply a subset of set (IE only appended items) we have to delete and start
	// again
	// TODO: Actually try to only add the appended items in that case
	if (!ref.empty())
	{
		node->setAttribute("op", DELETE_OP_ATTRIBUTE);
	}
	for (const auto &entry : set)
	{
		serializeOut(node->addNode("entry"), entry, defaultRef);
	}
}

template <typename A, typename B>
void serializeOut(SerializationNode *node, const std::pair<A, B> &pair, const std::pair<A, B> &ref)
{
	serializeOut(node->addNode("first"), pair.first, ref.first);
	serializeOut(node->addNode("second"), pair.second, ref.second);
}

template <typename T>
void serializeOut(SerializationNode *node, const std::list<T> &list, const std::list<T> &ref)
{
	const T defaultRef{};
	// If ref isn't simply a prefix of list (IE only appended items) we have to delete and start
	// again
	// TODO: Actually try to only add the appended items in that case
	if (!ref.empty())
	{
		node->setAttribute("op", DELETE_OP_ATTRIBUTE);
	}

	for (auto &entry : list)
	{
		serializeOut(node->addNode("entry"), entry, defaultRef);
	}
}

template <typename T>
void serializeOut(SerializationNode *node, const std::vector<T> &vector, const std::vector<T> &ref)
{
	const T defaultRef{};
	// If ref isn't simply a prefix of vector (IE only appended items) we have to delete and start
	// again
	// TODO: Actually try to only add the appended items in that case
	if (!ref.empty())
	{
		node->setAttribute("op", DELETE_OP_ATTRIBUTE);
	}
	for (auto &entry : vector)
	{
		serializeOut(node->addNode("entry"), entry, defaultRef);
	}
	uint64_t sizeHint = vector.size();
	uint64_t sizeHintDefault = 0;
	serializeOut(node->addNode("sizeHint"), sizeHint, sizeHintDefault);
}

void serializeIn(const GameState *, SerializationNode *node, sp<UnitAI> &ai);
void serializeIn(const GameState *, SerializationNode *node, sp<TacticalAI> &ai);

void serializeOut(SerializationNode *node, const sp<UnitAI> &ptr, const sp<UnitAI> &ref);
bool operator==(const UnitAI &a, const UnitAI &b);
bool operator!=(const UnitAI &a, const UnitAI &b);

void serializeOut(SerializationNode *node, const sp<TacticalAI> &ptr, const sp<TacticalAI> &ref);
bool operator==(const TacticalAI &a, const TacticalAI &b);
bool operator!=(const TacticalAI &a, const TacticalAI &b);

} // namespace OpenApoc

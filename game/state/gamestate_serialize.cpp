#include "game/state/gamestate_serialize.h"
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
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize_generated.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/rules/vammo_type.h"
#include "game/state/rules/vequipment_type.h"
#include "library/voxel.h"

namespace OpenApoc
{

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

void serializeIn(const GameState *state, sp<SerializationNode> node, Colour &c)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("r"), c.r);
	serializeIn(state, node->getNode("g"), c.g);
	serializeIn(state, node->getNode("b"), c.b);
	serializeIn(state, node->getNode("a"), c.a);
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

void serializeOut(sp<SerializationNode> node, const UString &string, const UString &)
{
	node->setValue(string);
}

void serializeOut(sp<SerializationNode> node, const unsigned int &val, const unsigned int &)
{
	node->setValueUInt(val);
}

void serializeOut(sp<SerializationNode> node, const unsigned char &val, const unsigned char &)
{
	node->setValueUChar(val);
}

void serializeOut(sp<SerializationNode> node, const float &val, const float &)
{
	node->setValueFloat(val);
}

void serializeOut(sp<SerializationNode> node, const int &val, const int &)
{
	node->setValueInt(val);
}

void serializeOut(sp<SerializationNode> node, const uint64_t &val, const uint64_t &)
{
	node->setValueUInt64(val);
}

void serializeOut(sp<SerializationNode> node, const bool &val, const bool &)
{
	node->setValueBool(val);
}

void serializeOut(sp<SerializationNode> node, const sp<LazyImage> &ptr, const sp<LazyImage> &)
{
	if (ptr != nullptr)
	{
		node->setValue(ptr->path);
	}
}

void serializeOut(sp<SerializationNode> node, const sp<Image> &ptr, const sp<Image> &)
{
	if (ptr != nullptr)
	{
		node->setValue(ptr->path);
	}
}

void serializeOut(sp<SerializationNode> node, const sp<VoxelSlice> &ptr, const sp<VoxelSlice> &)
{
	if (ptr)
	{
		node->setValue(ptr->path);
	}
}

void serializeOut(sp<SerializationNode> node, const sp<Sample> &ptr, const sp<Sample> &)
{
	if (ptr)
	{
		node->setValue(ptr->path);
	}
}

void serializeOut(sp<SerializationNode> node, const std::vector<bool> &vector,
                  const std::vector<bool> &)
{
	node->setValueBoolVector(vector);
}

void serializeOut(sp<SerializationNode> node, const VoxelMap &map, const VoxelMap &ref)
{
	serializeOut(node->addNode("size"), map.size, ref.size);
	serializeOut(node->addNode("slices"), map.slices, ref.slices);
}

void serializeOut(sp<SerializationNode> node, const Colour &c, const Colour &ref)
{
	serializeOut(node->addNode("r"), c.r, ref.r);
	serializeOut(node->addNode("g"), c.g, ref.g);
	serializeOut(node->addNode("b"), c.b, ref.b);
	serializeOut(node->addNode("a"), c.a, ref.a);
}

void serializeOut(sp<SerializationNode> node, const Xorshift128Plus<uint32_t> &t,
                  const Xorshift128Plus<uint32_t> &ref)
{
	if (!node)
		return;

	uint32_t s[2] = {0, 0};
	uint32_t sr[2] = {0, 0};
	t.getState(s);
	ref.getState(sr);
	serializeOut(node->addNode("s0"), s[0], sr[0]);
	serializeOut(node->addNode("s1"), s[1], sr[1]);
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
		GameState defaultState;
		serializeOut(archive->newRoot("", "gamestate"), *this, defaultState);
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

static bool serialize(const BattleMapTileset &tileSet, sp<SerializationArchive> archive)
{
	try
	{
		BattleMapTileset defaultTileset;
		serializeOut(archive->newRoot("", "tileset"), tileSet, defaultTileset);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\" at %s", e.what(), e.node->getFullPath().cStr());
		return false;
	}
	return true;
}

static bool deserialize(BattleMapTileset &tileSet, const GameState &state,
                        const sp<SerializationArchive> archive)
{
	try
	{
		serializeIn(&state, archive->getRoot("", "tileset"), tileSet);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\" at %s", e.what(), e.node->getFullPath().cStr());
		return false;
	}
	return true;
}

// FIXME: Move tilesetPath to some config variable?
const UString BattleMapTileset::tilesetPath = "data/tilesets";

bool BattleMapTileset::saveTileset(const UString &path, bool pack)
{
	TRACE_FN_ARGS1("path", path);
	auto archive = SerializationArchive::createArchive();
	if (serialize(*this, archive))
	{
		archive->write(path, pack);
		return true;
	}
	return false;
}

bool BattleMapTileset::loadTileset(GameState &state, const UString &path)
{

	TRACE_FN_ARGS1("path", path);
	auto archive = SerializationArchive::readArchive(path);
	if (!archive)
	{
		LogError("Failed to read \"%s\"", path.cStr());
		return false;
	}

	return deserialize(*this, state, archive);
}

static bool serialize(const BattleMapSectorTiles &mapSector, sp<SerializationArchive> archive)
{
	try
	{
		BattleMapSectorTiles defaultMapSector;
		serializeOut(archive->newRoot("", "mapSector"), mapSector, defaultMapSector);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\" at %s", e.what(), e.node->getFullPath().cStr());
		return false;
	}
	return true;
}

static bool deserialize(BattleMapSectorTiles &mapSector, const GameState &state,
                        const sp<SerializationArchive> archive)
{
	try
	{
		serializeIn(&state, archive->getRoot("", "mapSector"), mapSector);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\" at %s", e.what(), e.node->getFullPath().cStr());
		return false;
	}
	return true;
}

// FIXME: Move tilesetPath to some config variable?
const UString BattleMapSectorTiles::mapSectorPath = "data/maps";

bool BattleMapSectorTiles::saveSector(const UString &path, bool pack)
{
	TRACE_FN_ARGS1("path", path);
	auto archive = SerializationArchive::createArchive();
	if (serialize(*this, archive))
	{
		archive->write(path, pack);
		return true;
	}
	return false;
}

bool BattleMapSectorTiles::loadSector(GameState &state, const UString &path)
{

	TRACE_FN_ARGS1("path", path);
	auto archive = SerializationArchive::readArchive(path);
	if (!archive)
	{
		LogError("Failed to read \"%s\"", path.cStr());
		return false;
	}

	return deserialize(*this, state, archive);
}

} // namespace OpenApoc

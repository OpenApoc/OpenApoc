#include "game/state/gamestate_serialize.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/serialization/serialize.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/facility.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize_generated.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/rules/city/baselayout.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/shared/doodad.h"
#include "game/state/shared/projectile.h"
#include "library/voxel.h"

namespace OpenApoc
{

void serializeIn(const GameState *, SerializationNode *node, UString &str)
{
	if (!node)
		return;
	str = node->getValue();
}

void serializeIn(const GameState *, SerializationNode *node, unsigned int &val)
{
	if (!node)
		return;
	val = node->getValueUInt();
}

void serializeIn(const GameState *, SerializationNode *node, unsigned char &val)
{
	if (!node)
		return;
	val = node->getValueUChar();
}

void serializeIn(const GameState *, SerializationNode *node, float &val)
{
	if (!node)
		return;
	val = node->getValueFloat();
}

void serializeIn(const GameState *, SerializationNode *node, int &val)
{
	if (!node)
		return;
	val = node->getValueInt();
}

void serializeIn(const GameState *, SerializationNode *node, uint64_t &val)
{
	if (!node)
		return;
	val = node->getValueUInt64();
}

void serializeIn(const GameState *, SerializationNode *node, bool &val)
{
	if (!node)
		return;
	val = node->getValueBool();
}

void serializeIn(const GameState *, SerializationNode *node, sp<LazyImage> &ptr)
{
	if (!node)
		return;
	ptr = std::static_pointer_cast<LazyImage>(fw().data->loadImage(node->getValue(), true));
}

void serializeIn(const GameState *, SerializationNode *node, sp<Image> &ptr)
{
	if (!node)
		return;
	ptr = fw().data->loadImage(node->getValue());
}

// std::vector<bool> is special
void serializeIn(const GameState *, SerializationNode *node, std::vector<bool> &vector)
{
	if (!node)
		return;
	vector = node->getValueBoolVector();
}

void serializeIn(const GameState *, SerializationNode *node, sp<VoxelSlice> &ptr)
{
	if (!node)
		return;
	ptr = fw().data->loadVoxelSlice(node->getValue());
}

void serializeIn(const GameState *, SerializationNode *node, sp<Sample> &ptr)
{
	if (!node)
		return;
	ptr = fw().data->loadSample(node->getValue());
}

void serializeIn(const GameState *state, SerializationNode *node, VoxelMap &map)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("size"), map.size);
	serializeIn(state, node->getNode("slices"), map.slices);
}

void serializeIn(const GameState *state, SerializationNode *node, Colour &c)
{
	if (!node)
		return;
	serializeIn(state, node->getNode("r"), c.r);
	serializeIn(state, node->getNode("g"), c.g);
	serializeIn(state, node->getNode("b"), c.b);
	serializeIn(state, node->getNode("a"), c.a);
}

void serializeIn(const GameState *state, SerializationNode *node, Xorshift128Plus<uint32_t> &t)
{
	if (!node)
		return;

	uint64_t s[2] = {0, 0};
	serializeIn(state, node->getNode("s0"), s[0]);
	serializeIn(state, node->getNode("s1"), s[1]);
	t.setState(s);
}

void serializeOut(SerializationNode *node, const UString &string, const UString &)
{
	node->setValue(string);
}

void serializeOut(SerializationNode *node, const unsigned int &val, const unsigned int &)
{
	node->setValueUInt(val);
}

void serializeOut(SerializationNode *node, const unsigned char &val, const unsigned char &)
{
	node->setValueUChar(val);
}

void serializeOut(SerializationNode *node, const float &val, const float &)
{
	node->setValueFloat(val);
}

void serializeOut(SerializationNode *node, const int &val, const int &) { node->setValueInt(val); }

void serializeOut(SerializationNode *node, const uint64_t &val, const uint64_t &)
{
	node->setValueUInt64(val);
}

void serializeOut(SerializationNode *node, const bool &val, const bool &)
{
	node->setValueBool(val);
}

void serializeOut(SerializationNode *node, const sp<LazyImage> &ptr, const sp<LazyImage> &)
{
	if (ptr != nullptr)
	{
		node->setValue(ptr->path);
	}
}

void serializeOut(SerializationNode *node, const sp<Image> &ptr, const sp<Image> &)
{
	if (ptr != nullptr)
	{
		node->setValue(ptr->path);
	}
}

void serializeOut(SerializationNode *node, const sp<VoxelSlice> &ptr, const sp<VoxelSlice> &)
{
	if (ptr)
	{
		node->setValue(ptr->path);
	}
}

void serializeOut(SerializationNode *node, const sp<Sample> &ptr, const sp<Sample> &)
{
	if (ptr)
	{
		node->setValue(ptr->path);
	}
}

void serializeOut(SerializationNode *node, const std::vector<bool> &vector,
                  const std::vector<bool> &)
{
	node->setValueBoolVector(vector);
}

void serializeOut(SerializationNode *node, const VoxelMap &map, const VoxelMap &ref)
{
	serializeOut(node->addNode("size"), map.size, ref.size);
	serializeOut(node->addNode("slices"), map.slices, ref.slices);
}

void serializeOut(SerializationNode *node, const Colour &c, const Colour &ref)
{
	serializeOut(node->addNode("r"), c.r, ref.r);
	serializeOut(node->addNode("g"), c.g, ref.g);
	serializeOut(node->addNode("b"), c.b, ref.b);
	serializeOut(node->addNode("a"), c.a, ref.a);
}

void serializeOut(SerializationNode *node, const Xorshift128Plus<uint32_t> &t,
                  const Xorshift128Plus<uint32_t> &ref)
{
	if (!node)
		return;

	uint64_t s[2] = {0, 0};
	uint64_t sr[2] = {0, 0};
	t.getState(s);
	ref.getState(sr);
	serializeOut(node->addNode("s0"), s[0], sr[0]);
	serializeOut(node->addNode("s1"), s[1], sr[1]);
}

void serializeIn(const GameState *state, SerializationNode *node, sp<UnitAI> &ai)
{
	if (!node)
		return;

	UnitAI::Type type;
	serializeIn(state, node->getNode("type"), type);
	switch (type)
	{
		case UnitAI::Type::LowMorale:
		{
			auto sai = std::make_shared<UnitAILowMorale>();
			serializeIn(state, node, sai);
			ai = sai;
			break;
		}
		case UnitAI::Type::Default:
		{
			auto sai = std::make_shared<UnitAIDefault>();
			serializeIn(state, node, sai);
			ai = sai;
			break;
		}
		case UnitAI::Type::Behavior:
		{
			auto sai = std::make_shared<UnitAIBehavior>();
			serializeIn(state, node, sai);
			ai = sai;
			break;
		}
		case UnitAI::Type::Vanilla:
		{
			auto sai = std::make_shared<UnitAIVanilla>();
			serializeIn(state, node, sai);
			ai = sai;
			break;
		}
		case UnitAI::Type::Hardcore:
		{
			auto sai = std::make_shared<UnitAIHardcore>();
			serializeIn(state, node, sai);
			ai = sai;
			break;
		}
	}
}

void serializeIn(const GameState *state, SerializationNode *node, sp<TacticalAI> &ai)
{
	if (!node)
		return;

	TacticalAI::Type type;
	serializeIn(state, node->getNode("type"), type);
	switch (type)
	{
		case TacticalAI::Type::Vanilla:
		{
			auto sai = sp<TacticalAIVanilla>();
			serializeIn(state, node, sai);
			ai = sai;
			break;
		}
	}
}

void serializeOut(SerializationNode *node, const sp<UnitAI> &ptr, const sp<UnitAI> &ref)
{
	if (ptr)
	{
		switch (ptr->type)
		{
			case UnitAI::Type::LowMorale:
			{
				auto ptrCast = std::static_pointer_cast<UnitAILowMorale>(ptr);
				auto refCast = ref ? std::static_pointer_cast<UnitAILowMorale>(ref) : nullptr;
				serializeOut(node, ptrCast, refCast);
				break;
			}
			case UnitAI::Type::Default:
			{
				auto ptrCast = std::static_pointer_cast<UnitAIDefault>(ptr);
				auto refCast = ref ? std::static_pointer_cast<UnitAIDefault>(ref) : nullptr;
				serializeOut(node, ptrCast, refCast);
				break;
			}
			case UnitAI::Type::Behavior:
			{
				auto ptrCast = std::static_pointer_cast<UnitAIBehavior>(ptr);
				auto refCast = ref ? std::static_pointer_cast<UnitAIBehavior>(ref) : nullptr;
				serializeOut(node, ptrCast, refCast);
				break;
			}
			case UnitAI::Type::Vanilla:
			{
				auto ptrCast = std::static_pointer_cast<UnitAIVanilla>(ptr);
				auto refCast = ref ? std::static_pointer_cast<UnitAIVanilla>(ref) : nullptr;
				serializeOut(node, ptrCast, refCast);
				break;
			}
			case UnitAI::Type::Hardcore:
			{
				auto ptrCast = std::static_pointer_cast<UnitAIHardcore>(ptr);
				auto refCast = ref ? std::static_pointer_cast<UnitAIHardcore>(ref) : nullptr;
				serializeOut(node, ptrCast, refCast);
				break;
			}
		}
	}
}

bool operator==(const UnitAI &a, const UnitAI &b)
{
	if (a.type != b.type)
	{
		return false;
	}
	switch (a.type)
	{
		case UnitAI::Type::LowMorale:
		{
			const UnitAILowMorale &ca = static_cast<const UnitAILowMorale &>(a);
			const UnitAILowMorale &cb = static_cast<const UnitAILowMorale &>(b);
			return ca == cb;
		}
		case UnitAI::Type::Default:
		{
			const UnitAIDefault &ca = static_cast<const UnitAIDefault &>(a);
			const UnitAIDefault &cb = static_cast<const UnitAIDefault &>(b);
			return ca == cb;
		}
		case UnitAI::Type::Behavior:
		{
			const UnitAIBehavior &ca = static_cast<const UnitAIBehavior &>(a);
			const UnitAIBehavior &cb = static_cast<const UnitAIBehavior &>(b);
			return ca == cb;
		}
		case UnitAI::Type::Vanilla:
		{
			const UnitAIVanilla &ca = static_cast<const UnitAIVanilla &>(a);
			const UnitAIVanilla &cb = static_cast<const UnitAIVanilla &>(b);
			return ca == cb;
		}
		case UnitAI::Type::Hardcore:
		{
			const UnitAIHardcore &ca = static_cast<const UnitAIHardcore &>(a);
			const UnitAIHardcore &cb = static_cast<const UnitAIHardcore &>(b);
			return ca == cb;
		}
	}
	LogError("Unsupported comparison for UserAI type %d", (int)a.type);
	return false;
}
bool operator!=(const UnitAI &a, const UnitAI &b) { return !(a == b); }

void serializeOut(SerializationNode *node, const sp<TacticalAI> &ptr, const sp<TacticalAI> &ref)
{
	if (ptr)
	{
		switch (ptr->type)
		{
			case TacticalAI::Type::Vanilla:
			{
				auto ptrCast = std::static_pointer_cast<TacticalAIVanilla>(ptr);
				auto refCast = ref ? std::static_pointer_cast<TacticalAIVanilla>(ref) : nullptr;
				serializeOut(node, ptrCast, refCast);
				break;
			}
		}
	}
}

bool operator==(const TacticalAI &a, const TacticalAI &b)
{
	if (a.type != b.type)
	{
		return false;
	}
	switch (a.type)
	{
		case TacticalAI::Type::Vanilla:
		{
			const TacticalAIVanilla &ca = static_cast<const TacticalAIVanilla &>(a);
			const TacticalAIVanilla &cb = static_cast<const TacticalAIVanilla &>(b);
			return ca == cb;
		}
	}
	LogError("Unsupported comparison for Tactical type %d", (int)a.type);
	return false;
}
bool operator!=(const TacticalAI &a, const TacticalAI &b) { return !(a == b); }

bool GameState::saveGame(const UString &path, bool pack, bool pretty)
{
	auto archive = SerializationArchive::createArchive();
	if (serialize(archive.get()))
	{
		archive->write(path, pack, pretty);
		return true;
	}
	return false;
}

bool GameState::saveGameDelta(const UString &path, const GameState &reference, bool pack,
                              bool pretty)
{
	auto archive = SerializationArchive::createArchive();
	if (serialize(archive.get(), reference))
	{
		archive->write(path, pack, pretty);
		return true;
	}
	return false;
}

bool GameState::loadGame(const UString &path)
{

	auto archive = SerializationArchive::readArchive(path);
	if (!archive)
	{
		LogError("Failed to read \"%s\"", path);
		return false;
	}

	return deserialize(archive.get());
}

bool GameState::serialize(SerializationArchive *archive) const
{
	try
	{
		GameState defaultState;
		auto root = archive->newRoot("", "gamestate");
		root->addNode("serialization_version", GAMESTATE_SERIALIZATION_VERSION);
		serializeOut(root, *this, defaultState);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\"", e.what());
		return false;
	}
	return true;
}

bool GameState::serialize(SerializationArchive *archive, const GameState &reference) const
{
	try
	{
		auto root = archive->newRoot("", "gamestate");
		root->addNode("serialization_version", GAMESTATE_SERIALIZATION_VERSION);
		serializeOut(root, *this, reference);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\"", e.what());
		return false;
	}
	return true;
}

bool GameState::deserialize(SerializationArchive *archive)
{
	try
	{
		serializeIn(this, archive->getRoot("", "gamestate"), *this);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\"", e.what());
		return false;
	}
	return true;
}

static bool serialize(const BattleMapTileset &tileSet, SerializationArchive *archive)
{
	try
	{
		BattleMapTileset defaultTileset;
		serializeOut(archive->newRoot("", "tileset"), tileSet, defaultTileset);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\"", e.what());
		return false;
	}
	return true;
}

static bool deserialize(BattleMapTileset &tileSet, const GameState &state,
                        SerializationArchive *archive)
{
	try
	{
		serializeIn(&state, archive->getRoot("", "tileset"), tileSet);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\"", e.what());
		return false;
	}
	return true;
}

bool BattleMapTileset::saveTileset(const UString &path, bool pack, bool pretty)
{
	auto archive = SerializationArchive::createArchive();
	if (serialize(*this, archive.get()))
	{
		archive->write(path, pack, pretty);
		return true;
	}
	return false;
}

bool BattleMapTileset::loadTileset(GameState &state, const UString &path)
{

	auto archive = SerializationArchive::readArchive(path);
	if (!archive)
	{
		LogError("Failed to read \"%s\"", path);
		return false;
	}

	return deserialize(*this, state, archive.get());
}

static bool serialize(const BattleUnitImagePack &imagePack, SerializationArchive *archive)
{
	try
	{
		BattleUnitImagePack defaultImagePack;
		serializeOut(archive->newRoot("", "imagepack"), imagePack, defaultImagePack);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\"", e.what());
		return false;
	}
	return true;
}

static bool deserialize(BattleUnitImagePack &imagePack, const GameState &state,
                        SerializationArchive *archive)
{
	try
	{
		serializeIn(&state, archive->getRoot("", "imagepack"), imagePack);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\"", e.what());
		return false;
	}
	return true;
}

bool BattleUnitImagePack::saveImagePack(const UString &path, bool pack, bool pretty)
{
	auto archive = SerializationArchive::createArchive();
	if (serialize(*this, archive.get()))
	{
		archive->write(path, pack, pretty);
		return true;
	}
	return false;
}

bool BattleUnitImagePack::loadImagePack(GameState &state, const UString &path)
{

	auto file = fw().data->fs.open(path);
	if (!file)
	{
		LogError("Failed to open image pack \"%s\"", path);
		return false;
	}
	auto fullPath = file.systemPath();

	auto archive = SerializationArchive::readArchive(fullPath);
	if (!archive)
	{
		LogError("Failed to read \"%s\"", fullPath);
		return false;
	}

	return deserialize(*this, state, archive.get());
}

static bool serialize(const BattleUnitAnimationPack &animationPack, SerializationArchive *archive)
{
	try
	{
		BattleUnitAnimationPack defaultAnimationPack;
		serializeOut(archive->newRoot("", "animationpack"), animationPack, defaultAnimationPack);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\"", e.what());
		return false;
	}
	return true;
}

static bool deserialize(BattleUnitAnimationPack &animationPack, const GameState &state,
                        SerializationArchive *archive)
{
	try
	{
		serializeIn(&state, archive->getRoot("", "animationpack"), animationPack);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\"", e.what());
		return false;
	}
	return true;
}

bool BattleUnitAnimationPack::saveAnimationPack(const UString &path, bool pack, bool pretty)
{
	auto archive = SerializationArchive::createArchive();
	if (serialize(*this, archive.get()))
	{
		archive->write(path, pack, pretty);
		return true;
	}
	return false;
}

bool BattleUnitAnimationPack::loadAnimationPack(GameState &state, const UString &path)
{
	auto file = fw().data->fs.open(path);
	if (!file)
	{
		LogError("Failed to open animation pack \"%s\"", path);
	}
	const auto fullPath = file.systemPath();

	auto archive = SerializationArchive::readArchive(fullPath);
	if (!archive)
	{
		LogError("Failed to read \"%s\"", fullPath);
		return false;
	}

	return deserialize(*this, state, archive.get());
}

static bool serialize(const BattleMapSectorTiles &mapSector, SerializationArchive *archive)
{
	try
	{
		BattleMapSectorTiles defaultMapSector;
		serializeOut(archive->newRoot("", "mapsector"), mapSector, defaultMapSector);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\"", e.what());
		return false;
	}
	return true;
}

static bool deserialize(BattleMapSectorTiles &mapSector, const GameState &state,
                        SerializationArchive *archive)
{
	try
	{
		serializeIn(&state, archive->getRoot("", "mapsector"), mapSector);
	}
	catch (SerializationException &e)
	{
		LogError("Serialization failed: \"%s\"", e.what());
		return false;
	}
	return true;
}

bool BattleMapSectorTiles::saveSector(const UString &path, bool pack, bool pretty)
{
	auto archive = SerializationArchive::createArchive();
	if (serialize(*this, archive.get()))
	{
		archive->write(path, pack, pretty);
		return true;
	}
	return false;
}

bool BattleMapSectorTiles::loadSector(GameState &state, const UString &path)
{

	auto archive = SerializationArchive::readArchive(path);
	if (!archive)
	{
		LogError("Failed to read \"%s\"", path);
		return false;
	}

	return deserialize(*this, state, archive.get());
}

} // namespace OpenApoc

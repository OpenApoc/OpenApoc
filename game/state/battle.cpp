#include "game/state/city/city.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "game/state/battle.h"
#include "game/state/tileview/voxel.h"
#include "game/state/gamestate.h"
#include <functional>
#include <future>
#include <limits>
#include <unordered_map>

namespace OpenApoc
{
	// 01: Tile Types

	template <>
	sp<BattleGroundType> StateObject<BattleGroundType>::get(const GameState &state, const UString &id)
	{
		
		auto it = state.battle.ground_types.find(id);
		if (it != state.battle.ground_types.end())
			return it->second;

		LogError("No battle tile type matching ID \"%s\"", id.cStr());
		return nullptr;
	}

	template <> const UString &StateObject<BattleGroundType>::getPrefix()
	{
		static UString prefix = "BATTLEGROUND_";
		return prefix;
	}
	template <> const UString &StateObject<BattleGroundType>::getTypeName()
	{
		static UString name = "BattleGroundType";
		return name;
	}

	template <>
	sp<BattleLeftWallType> StateObject<BattleLeftWallType>::get(const GameState &state, const UString &id)
	{

		auto it = state.battle.left_wall_types.find(id);
		if (it != state.battle.left_wall_types.end())
			return it->second;

		LogError("No battle left wall type matching ID \"%s\"", id.cStr());
		return nullptr;
	}

	template <> const UString &StateObject<BattleLeftWallType>::getPrefix()
	{
		static UString prefix = "BATTLELEFTWALL_";
		return prefix;
	}
	template <> const UString &StateObject<BattleLeftWallType>::getTypeName()
	{
		static UString name = "BattleLeftWallType";
		return name;
	}

	template <>
	sp<BattleRightWallType> StateObject<BattleRightWallType>::get(const GameState &state, const UString &id)
	{

		auto it = state.battle.right_wall_types.find(id);
		if (it != state.battle.right_wall_types.end())
			return it->second;

		LogError("No battle right wall type matching ID \"%s\"", id.cStr());
		return nullptr;
	}

	template <> const UString &StateObject<BattleRightWallType>::getPrefix()
	{
		static UString prefix = "BATTLERIGHTWALL_";
		return prefix;
	}
	template <> const UString &StateObject<BattleRightWallType>::getTypeName()
	{
		static UString name = "BattleRightWallType";
		return name;
	}

	template <>
	sp<BattleSceneryType> StateObject<BattleSceneryType>::get(const GameState &state, const UString &id)
	{

		auto it = state.battle.scenery_types.find(id);
		if (it != state.battle.scenery_types.end())
			return it->second;

		LogError("No battle scenery type matching ID \"%s\"", id.cStr());
		return nullptr;
	}

	template <> const UString &StateObject<BattleSceneryType>::getPrefix()
	{
		static UString prefix = "BATTLESCENERY_";
		return prefix;
	}
	template <> const UString &StateObject<BattleSceneryType>::getTypeName()
	{
		static UString name = "BattleSceneryType";
		return name;
	}
	
	// 02: Battle

	// An ordered list of the types drawn in each layer
	// Within the same layer these are ordered by a calculated z based on the 'center' position
	static std::vector<std::set<BattleTileObject::Type>> layerMap = {
		// Draw ground first, then put stuff on top of that
		{ BattleTileObject::Type::Ground,BattleTileObject::Type::LeftWall, BattleTileObject::Type::RightWall, BattleTileObject::Type::Scenery },
		{ },
	};

	Battle::~Battle()
	{
		//For now, do nothing
	}
	
	void Battle::start()
	{
		for (auto &pair : initial_grounds)
		{
			auto s = mksp<BattleGround>();

			s->type = pair.second;
			s->initialPosition = pair.first;
			s->currentPosition = s->initialPosition;

			ground.insert(s);
		}
		for (auto &pair : initial_left_walls)
		{
			auto s = mksp<BattleLeftWall>();

			s->type = pair.second;
			s->initialPosition = pair.first;
			s->currentPosition = s->initialPosition;

			left_wall.insert(s);
		}
		for (auto &pair : initial_right_walls)
		{
			auto s = mksp<BattleRightWall>();

			s->type = pair.second;
			s->initialPosition = pair.first;
			s->currentPosition = s->initialPosition;

			right_wall.insert(s);
		}
		for (auto &pair : initial_scenery)
		{
			auto s = mksp<BattleScenery>();

			s->type = pair.second;
			s->initialPosition = pair.first;
			s->currentPosition = s->initialPosition;

			scenery.insert(s);
		}

		initMap();

	}

	void Battle::initMap()
	{
		if (this->map)
		{
			LogError("Called on battle with existing map");
			return;
		}
		this->map.reset(new BattleTileMap(this->size, layerMap));
		for (auto &s : this->ground)
		{
			this->map->addObjectToMap(s);
		}
		for (auto &s : this->left_wall)
		{
			this->map->addObjectToMap(s);
		}
		for (auto &s : this->right_wall)
		{
			this->map->addObjectToMap(s);
		}
		for (auto &s : this->scenery)
		{
			this->map->addObjectToMap(s);
		}
	}

	void Battle::update(GameState &state, unsigned int ticks)
	{
		Trace::start("City::update::ground->update");
		for (auto &s : this->ground)
		{
			s->update(state, ticks);
		}
		Trace::end("City::update::ground->update");
		Trace::start("City::update::left_wall->update");
		for (auto &s : this->left_wall)
		{
			s->update(state, ticks);
		}
		Trace::end("City::update::left_wall->update");
		Trace::start("City::update::right_wall->update");
		for (auto &s : this->right_wall)
		{
			s->update(state, ticks);
		}
		Trace::end("City::update::right_wall->update");
		Trace::start("City::update::scenery->update");
		for (auto &s : this->scenery)
		{
			s->update(state, ticks);
		}
		Trace::end("City::update::scenery->update");
	}

	// 03: BattleTile

	BattleTile::BattleTile(BattleTileMap &map, Vec3<int> position, int layerCount)
		: map(map), position(position), drawnObjects(layerCount)
	{
	}

	// 04: BattleTileMap

	BattleTileMap::BattleTileMap(Vec3<int> size, std::vector<std::set<BattleTileObject::Type>> layerMap)
		: layerMap(layerMap), size(size)
	{
		tiles.reserve(size.x * size.y * size.z);
		for (int z = 0; z < size.z; z++)
		{
			for (int y = 0; y < size.y; y++)
			{
				for (int x = 0; x < size.x; x++)
				{
					tiles.emplace_back(*this, Vec3<int>{x, y, z}, this->getLayerCount());
				}
			}
		}

		// Quick sanity check of the layer map:
		std::set<BattleTileObject::Type> seenTypes;
		for (auto &typesInLayer : layerMap)
		{
			for (auto &type : typesInLayer)
			{
				if (seenTypes.find(type) != seenTypes.end())
				{
					LogError("Type %d appears in multiple layers", static_cast<int>(type));
				}
				seenTypes.insert(type);
			}
		}
	}

	BattleTileMap::~BattleTileMap() = default;

	void BattleTileMap::addObjectToMap(sp<BattleGround> ground)
	{
		if (ground->tileObject)
		{
			LogError("Ground already has tile object");
		}
		// FIXME: mksp<> doesn't work for private (but accessible due to friend)
		// constructors?
		sp<BattleTileObjectGround> obj(new BattleTileObjectGround(*this, ground));
		obj->setPosition(ground->getPosition());
		ground->tileObject = obj;
	}

	void BattleTileMap::addObjectToMap(sp<BattleLeftWall> left_wall)
	{
		if (left_wall->tileObject)
		{
			LogError("Left wall already has tile object");
		}
		// FIXME: mksp<> doesn't work for private (but accessible due to friend)
		// constructors?
		sp<BattleTileObjectLeftWall> obj(new BattleTileObjectLeftWall(*this, left_wall));
		obj->setPosition(left_wall->getPosition());
		left_wall->tileObject = obj;
	}

	void BattleTileMap::addObjectToMap(sp<BattleRightWall> right_wall)
	{
		if (right_wall->tileObject)
		{
			LogError("Right wall already has tile object");
		}
		// FIXME: mksp<> doesn't work for private (but accessible due to friend)
		// constructors?
		sp<BattleTileObjectRightWall> obj(new BattleTileObjectRightWall(*this, right_wall));
		obj->setPosition(right_wall->getPosition());
		right_wall->tileObject = obj;
	}

	void BattleTileMap::addObjectToMap(sp<BattleScenery> scenery)
	{
		if (scenery->tileObject)
		{
			LogError("Scenery already has tile object");
		}
		// FIXME: mksp<> doesn't work for private (but accessible due to friend)
		// constructors?
		sp<BattleTileObjectScenery> obj(new BattleTileObjectScenery(*this, scenery));
		obj->setPosition(scenery->getPosition());
		scenery->tileObject = obj;
	}

	int BattleTileMap::getLayer(BattleTileObject::Type type) const
	{
		for (unsigned i = 0; i < this->layerMap.size(); i++)
		{
			if (this->layerMap[i].find(type) != this->layerMap[i].end())
			{
				return i;
			}
		}
		LogError("No layer matching object type %d", static_cast<int>(type));
		return 0;
	}

	int BattleTileMap::getLayerCount() const { return this->layerMap.size(); }

	bool BattleTileMap::tileIsValid(Vec3<int> tile) const
	{
		if (tile.z < 0 || tile.z >= this->size.z || tile.y < 0 || tile.y >= this->size.y ||
			tile.x < 0 || tile.x >= this->size.x)
			return false;
		return true;
	}

	sp<Image> BattleTileMap::dumpVoxelView(const Rect<int> viewRect, const TileTransform &transform) const
	{
		auto img = mksp<RGBImage>(viewRect.size());
		std::map<sp<BattleTileObject>, Colour> objectColours;
		std::default_random_engine colourRNG;
		// MSVC doesn't like uint8_t being the type for uniform_int_distribution?
		std::uniform_int_distribution<int> colourDist(0, 255);

		RGBImageLock lock(img);
		int h = viewRect.p1.y - viewRect.p0.y;
		int w = viewRect.p1.x - viewRect.p0.x;
		Vec2<float> offset = { viewRect.p0.x, viewRect.p0.y };

		LogWarning("ViewRect {%d,%d},{%d,%d}", viewRect.p0.x, viewRect.p0.y, viewRect.p1.x,
			viewRect.p1.y);

		LogWarning("Dumping voxels {%d,%d} voxels w/offset {%f,%f}", w, h, offset.x, offset.y);

		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				auto topPos = transform.screenToTileCoords(Vec2<float>{x, y} +offset, 9.99f);
				auto bottomPos = transform.screenToTileCoords(Vec2<float>{x, y} +offset, 0.0f);

				auto collision = this->findCollision(topPos, bottomPos);
				if (collision)
				{
					if (objectColours.find(collision.obj) == objectColours.end())
					{
						Colour c = { static_cast<uint8_t>(colourDist(colourRNG)),
							static_cast<uint8_t>(colourDist(colourRNG)),
							static_cast<uint8_t>(colourDist(colourRNG)), 255 };
						objectColours[collision.obj] = c;
					}
					lock.set({ x, y }, objectColours[collision.obj]);
				}
			}
		}

		return img;
	}

	// 05: BattleTileObject

	BattleTileObject::BattleTileObject(BattleTileMap &map, Type type, Vec3<float> bounds)
		: map(map), type(type), owningTile(nullptr), bounds(bounds), name("UNKNOWN_OBJECT")
	{
	}

	BattleTileObject::~BattleTileObject() = default;

	void BattleTileObject::removeFromMap()
	{
		auto thisPtr = shared_from_this();
		/* owner may be NULL as this can be used to set the initial position after creation */
		if (this->owningTile)
		{
			auto erased = this->owningTile->ownedObjects.erase(thisPtr);
			if (erased != 1)
			{
				LogError("Nothing erased?");
			}
			int layer = map.getLayer(this->type);
			this->owningTile->drawnObjects[layer].erase(
				std::remove(this->owningTile->drawnObjects[layer].begin(),
					this->owningTile->drawnObjects[layer].end(), thisPtr),
				this->owningTile->drawnObjects[layer].end());
			this->owningTile = nullptr;
		}
		for (auto *tile : this->intersectingTiles)
		{
			tile->intersectingObjects.erase(thisPtr);
		}
		this->intersectingTiles.clear();
	}

	namespace
	{
		class BattleTileObjectZComparer
		{
		public:
			bool operator()(const sp<BattleTileObject> &lhs, const sp<BattleTileObject> &rhs) const
			{
				float lhsZ = lhs->getPosition().x * 32.0f + lhs->getPosition().y * 32.0f +
					lhs->getPosition().z * 16.0f;
				float rhsZ = rhs->getPosition().x * 32.0f + rhs->getPosition().y * 32.0f +
					rhs->getPosition().z * 16.0f;
				// FIXME: Hack to force 'overlay' objects to be half-a-tile up in Z
				/*if (lhs->getType() == TileObject::Type::Doodad)
				{
					lhsZ += (32.0f + 32.0f + 16.0f) / 2.0f;
				}
				if (rhs->getType() == BattleTileObject::Type::Doodad)
				{
					rhsZ += (32.0f + 32.0f + 16.0f) / 2.0f;
				}*/
				return (lhsZ < rhsZ);
			}
		};
	} // anonymous namespace

	float BattleTileObject::getDistanceTo(sp<BattleTileObject> target)
	{
		return getDistanceTo(target->getPosition());
	}

	float BattleTileObject::getDistanceTo(Vec3<float> target)
	{
		return glm::length((target - this->getPosition()) * VELOCITY_SCALE);
	}

	void BattleTileObject::setPosition(Vec3<float> newPosition)
	{
		auto thisPtr = shared_from_this();
		if (!thisPtr)
		{
			LogError("This == null");
		}
		if (newPosition.x < 0 || newPosition.y < 0 || newPosition.z < 0 ||
			newPosition.x > map.size.x + 1 || newPosition.y > map.size.y + 1 ||
			newPosition.z > map.size.z + 1)
		{
			LogWarning("Trying to place object at {%f,%f,%f} in map of size {%d,%d,%d}", newPosition.x,
				newPosition.y, newPosition.z, map.size.x, map.size.y, map.size.z);
			newPosition.x = clamp(newPosition.x, 0.0f, (float)map.size.x + 1);
			newPosition.y = clamp(newPosition.y, 0.0f, (float)map.size.y + 1);
			newPosition.z = clamp(newPosition.z, 0.0f, (float)map.size.z + 1);
			LogWarning("Clamped object to {%f,%f,%f}", newPosition.x, newPosition.y, newPosition.z);
		}
		this->removeFromMap();

		this->owningTile = map.getTile(newPosition);
		if (!this->owningTile)
		{
			LogError("Failed to get tile for position {%f,%f,%f}", newPosition.x, newPosition.y,
				newPosition.z);
		}

		auto inserted = this->owningTile->ownedObjects.insert(thisPtr);
		if (!inserted.second)
		{
			LogError("Object already in owned object list?");
		}

		int layer = map.getLayer(this->type);

		this->owningTile->drawnObjects[layer].push_back(thisPtr);
		std::sort(this->owningTile->drawnObjects[layer].begin(),
			this->owningTile->drawnObjects[layer].end(), BattleTileObjectZComparer{});

		Vec3<int> minBounds = { floorf(newPosition.x - this->bounds.x / 2.0f),
			floorf(newPosition.y - this->bounds.y / 2.0f),
			floorf(newPosition.z - this->bounds.z / 2.0f) };
		Vec3<int> maxBounds = { ceilf(newPosition.x + this->bounds.x / 2.0f),
			ceilf(newPosition.y + this->bounds.y / 2.0f),
			ceilf(newPosition.z + this->bounds.z / 2.0f) };

		for (int x = minBounds.x; x < maxBounds.x; x++)
		{
			for (int y = minBounds.y; y < maxBounds.y; y++)
			{
				for (int z = minBounds.z; z < maxBounds.z; z++)
				{
					if (x < 0 || y < 0 || z < 0 || x > map.size.x || y > map.size.y || z > map.size.z)
					{
						// TODO: Decide if having bounds outside the map are really valid?
						continue;
					}
					BattleTile *intersectingTile = map.getTile(x, y, z);
					if (!intersectingTile)
					{
						LogError("Failed to get intersecting tile at {%d,%d,%d}", x, y, z);
						continue;
					}
					this->intersectingTiles.push_back(intersectingTile);
					intersectingTile->intersectingObjects.insert(thisPtr);
				}
			}
		}
		// Quick sanity check
		for (auto *t : this->intersectingTiles)
		{
			if (t->intersectingObjects.find(shared_from_this()) == t->intersectingObjects.end())
			{
				LogError("Intersecting objects inconsistent");
			}
		}
	}

	// 06: BattleXYZ classes

	BattleScenery::BattleScenery() : damaged(false), falling(false), destroyed(false) {}

	void BattleScenery::handleCollision(GameState &state, CollisionB &c)
	{
		// FIXME: Proper damage
		std::ignore = c;
		// If this tile has a damaged tile, replace it with that. If it's already damaged, destroy as
		// normal
		if (!this->tileObject)
		{
			// It's possible multiple projectiles hit the same tile in the same
			// tick, so if the object has already been destroyed just NOP this.
			// The projectile will still 'hit' this tile though.
			return;
		}
		if (this->falling)
		{
			// Already falling, just continue
			return;
		}
		/*if (!this->damaged && type->damagedTile)
		{
			this->damaged = true;
		}
		else*/
		{
			// Don't destroy bottom tiles, else everything will leak out
			if (this->initialPosition.z != 0)
			{
				this->tileObject->removeFromMap();
				this->tileObject.reset();
			}
		}
		for (auto &s : this->supports)
			s->collapse(state);
	}

	void BattleScenery::collapse(GameState &state)
	{
		// IF it's already falling or destroyed do nothing
		if (this->falling || !this->tileObject)
			return;
		this->falling = true;

		for (auto &s : this->supports)
			s->collapse(state);
	}

	void BattleScenery::update(GameState &state, unsigned int ticks)
	{
		if (!this->falling)
			return;
		if (!this->tileObject)
		{
			LogError("Falling BattleScenery with no object?");
		}

		auto currentPos = this->tileObject->getPosition();
		// FIXME: gravity acceleration?
		currentPos.z -= static_cast<float>(ticks) / 16.0f;
		this->tileObject->setPosition(currentPos);
		
		for (auto &obj : this->tileObject->getOwningTile()->ownedObjects)
		{
			switch (obj->getType())
			{
			case BattleTileObject::Type::Ground:
				// FIXME: do something?
				break;
			default:
				// Ignore other object types?
				break;
			}
		}
	}

	bool BattleScenery::isAlive() const
	{
		if (this->damaged || this->falling || this->destroyed)
			return false;
		return true;
	}

	BattleGround::BattleGround() : damaged(false), falling(false), destroyed(false) {}

	void BattleGround::handleCollision(GameState &state, CollisionB &c)
	{
		// FIXME: Proper damage
		std::ignore = c;
		// If this tile has a damaged tile, replace it with that. If it's already damaged, destroy as
		// normal
		if (!this->tileObject)
		{
			// It's possible multiple projectiles hit the same tile in the same
			// tick, so if the object has already been destroyed just NOP this.
			// The projectile will still 'hit' this tile though.
			return;
		}
		if (this->falling)
		{
			// Already falling, just continue
			return;
		}
		/*if (!this->damaged && type->damagedTile)
		{
		this->damaged = true;
		}
		else*/
		{
			// Don't destroy bottom tiles, else everything will leak out
			if (this->initialPosition.z != 0)
			{
				this->tileObject->removeFromMap();
				this->tileObject.reset();
			}
		}
		for (auto &s : this->supports)
			s->collapse(state);
	}

	void BattleGround::collapse(GameState &state)
	{
		// IF it's already falling or destroyed do nothing
		if (this->falling || !this->tileObject)
			return;
		this->falling = true;

		for (auto &s : this->supports)
			s->collapse(state);
	}

	void BattleGround::update(GameState &state, unsigned int ticks)
	{
		if (!this->falling)
			return;
		if (!this->tileObject)
		{
			LogError("Falling BattleGround with no object?");
		}

		auto currentPos = this->tileObject->getPosition();
		// FIXME: gravity acceleration?
		currentPos.z -= static_cast<float>(ticks) / 16.0f;
		this->tileObject->setPosition(currentPos);

		for (auto &obj : this->tileObject->getOwningTile()->ownedObjects)
		{
			switch (obj->getType())
			{
			case BattleTileObject::Type::Ground:
				// FIXME: do something?
				break;
			default:
				// Ignore other object types?
				break;
			}
		}
	}

	bool BattleGround::isAlive() const
	{
		if (this->damaged || this->falling || this->destroyed)
			return false;
		return true;
	}

	BattleLeftWall::BattleLeftWall() : damaged(false), falling(false), destroyed(false) {}

	void BattleLeftWall::handleCollision(GameState &state, CollisionB &c)
	{
		// FIXME: Proper damage
		std::ignore = c;
		// If this tile has a damaged tile, replace it with that. If it's already damaged, destroy as
		// normal
		if (!this->tileObject)
		{
			// It's possible multiple projectiles hit the same tile in the same
			// tick, so if the object has already been destroyed just NOP this.
			// The projectile will still 'hit' this tile though.
			return;
		}
		if (this->falling)
		{
			// Already falling, just continue
			return;
		}
		/*if (!this->damaged && type->damagedTile)
		{
		this->damaged = true;
		}
		else*/
		{
			// Don't destroy bottom tiles, else everything will leak out
			if (this->initialPosition.z != 0)
			{
				this->tileObject->removeFromMap();
				this->tileObject.reset();
			}
		}
		for (auto &s : this->supports)
			s->collapse(state);
	}

	void BattleLeftWall::collapse(GameState &state)
	{
		// IF it's already falling or destroyed do nothing
		if (this->falling || !this->tileObject)
			return;
		this->falling = true;

		for (auto &s : this->supports)
			s->collapse(state);
	}

	void BattleLeftWall::update(GameState &state, unsigned int ticks)
	{
		if (!this->falling)
			return;
		if (!this->tileObject)
		{
			LogError("Falling BattleLeftWall with no object?");
		}

		auto currentPos = this->tileObject->getPosition();
		// FIXME: gravity acceleration?
		currentPos.z -= static_cast<float>(ticks) / 16.0f;
		this->tileObject->setPosition(currentPos);

		for (auto &obj : this->tileObject->getOwningTile()->ownedObjects)
		{
			switch (obj->getType())
			{
			case BattleTileObject::Type::Ground:
				// FIXME: do something?
				break;
			default:
				// Ignore other object types?
				break;
			}
		}
	}

	bool BattleLeftWall::isAlive() const
	{
		if (this->damaged || this->falling || this->destroyed)
			return false;
		return true;
	}

	BattleRightWall::BattleRightWall() : damaged(false), falling(false), destroyed(false) {}

	void BattleRightWall::handleCollision(GameState &state, CollisionB &c)
	{
		// FIXME: Proper damage
		std::ignore = c;
		// If this tile has a damaged tile, replace it with that. If it's already damaged, destroy as
		// normal
		if (!this->tileObject)
		{
			// It's possible multiple projectiles hit the same tile in the same
			// tick, so if the object has already been destroyed just NOP this.
			// The projectile will still 'hit' this tile though.
			return;
		}
		if (this->falling)
		{
			// Already falling, just continue
			return;
		}
		/*if (!this->damaged && type->damagedTile)
		{
		this->damaged = true;
		}
		else*/
		{
			// Don't destroy bottom tiles, else everything will leak out
			if (this->initialPosition.z != 0)
			{
				this->tileObject->removeFromMap();
				this->tileObject.reset();
			}
		}
		for (auto &s : this->supports)
			s->collapse(state);
	}

	void BattleRightWall::collapse(GameState &state)
	{
		// IF it's already falling or destroyed do nothing
		if (this->falling || !this->tileObject)
			return;
		this->falling = true;

		for (auto &s : this->supports)
			s->collapse(state);
	}

	void BattleRightWall::update(GameState &state, unsigned int ticks)
	{
		if (!this->falling)
			return;
		if (!this->tileObject)
		{
			LogError("Falling BattleRightWall with no object?");
		}

		auto currentPos = this->tileObject->getPosition();
		// FIXME: gravity acceleration?
		currentPos.z -= static_cast<float>(ticks) / 16.0f;
		this->tileObject->setPosition(currentPos);

		for (auto &obj : this->tileObject->getOwningTile()->ownedObjects)
		{
			switch (obj->getType())
			{
			case BattleTileObject::Type::Ground:
				// FIXME: do something?
				break;
			default:
				// Ignore other object types?
				break;
			}
		}
	}

	bool BattleRightWall::isAlive() const
	{
		if (this->damaged || this->falling || this->destroyed)
			return false;
		return true;
	}

	// 07: BattleTileObjectXYZ classes

	void BattleTileObjectScenery::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
		TileViewMode mode)
	{
		std::ignore = transform;
		// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
		auto scenery = this->scenery.lock();
		if (!scenery)
		{
			LogError("Called with no owning scenery object");
			return;
		}
		// FIXME: If damaged use damaged tile sprites?
		auto &type = scenery->type;
		sp<Image> sprite;
		//sp<Image> overlaySprite;
		Vec2<float> transformedScreenPos = screenPosition;
		switch (mode)
		{
		case TileViewMode::Isometric:
			sprite = type->sprite;
			//overlaySprite = type->overlaySprite;
			transformedScreenPos -= type->imageOffset;
			break;
		case TileViewMode::Strategy:
			sprite = type->strategySprite;
			// All strategy sprites so far are 8x8 so offset by 4 to draw from the center
			// FIXME: Not true for large sprites (2x2 UFOs?)
			transformedScreenPos -= Vec2<float>{4, 4};
			break;
		default:
			LogError("Unsupported view mode");
		}
		if (sprite)
			r.draw(sprite, transformedScreenPos);
		// FIXME: Should be drawn at 'later' Z than scenery (IE on top of any vehicles on tile?)
		/*if (overlaySprite)
			r.draw(overlaySprite, transformedScreenPos);*/
	}

	BattleTileObjectScenery::~BattleTileObjectScenery() = default;

	BattleTileObjectScenery::BattleTileObjectScenery(BattleTileMap &map, sp<BattleScenery> scenery)
		: BattleTileObject(map, Type::Scenery, Vec3<float>{1, 1, 1}), scenery(scenery)
	{
	}

	sp<BattleScenery> BattleTileObjectScenery::getOwner()
	{
		auto s = this->scenery.lock();
		if (!s)
		{
			LogError("Owning scenery object disappeared");
		}
		return s;
	}

	sp<VoxelMap> BattleTileObjectScenery::getVoxelMap() { return this->getOwner()->type->voxelMap; }

	Vec3<float> BattleTileObjectScenery::getPosition() const
	{
		auto s = this->scenery.lock();
		if (!s)
		{
			LogError("Called with no owning scenery object");
			return{ 0, 0, 0 };
		}
		return s->getPosition();
	}
	
	void BattleTileObjectGround::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
		TileViewMode mode)
	{
		std::ignore = transform;
		// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
		auto ground = this->ground.lock();
		if (!ground)
		{
			LogError("Called with no owning ground object");
			return;
		}
		// FIXME: If damaged use damaged tile sprites?
		auto &type = ground->type;
		sp<Image> sprite;
		//sp<Image> overlaySprite;
		Vec2<float> transformedScreenPos = screenPosition;
		switch (mode)
		{
		case TileViewMode::Isometric:
			sprite = type->sprite;
			//overlaySprite = type->overlaySprite;
			transformedScreenPos -= type->imageOffset;
			break;
		case TileViewMode::Strategy:
			sprite = type->strategySprite;
			// All strategy sprites so far are 8x8 so offset by 4 to draw from the center
			// FIXME: Not true for large sprites (2x2 UFOs?)
			transformedScreenPos -= Vec2<float>{4, 4};
			break;
		default:
			LogError("Unsupported view mode");
		}
		if (sprite)
			r.draw(sprite, transformedScreenPos);
		// FIXME: Should be drawn at 'later' Z than ground (IE on top of any vehicles on tile?)
		/*if (overlaySprite)
		r.draw(overlaySprite, transformedScreenPos);*/
	}

	BattleTileObjectGround::~BattleTileObjectGround() = default;

	BattleTileObjectGround::BattleTileObjectGround(BattleTileMap &map, sp<BattleGround> ground)
		: BattleTileObject(map, Type::Ground, Vec3<float>{1, 1, 1}), ground(ground)
	{
	}

	sp<BattleGround> BattleTileObjectGround::getOwner()
	{
		auto s = this->ground.lock();
		if (!s)
		{
			LogError("Owning ground object disappeared");
		}
		return s;
	}

	sp<VoxelMap> BattleTileObjectGround::getVoxelMap() { return this->getOwner()->type->voxelMap; }

	Vec3<float> BattleTileObjectGround::getPosition() const
	{
		auto s = this->ground.lock();
		if (!s)
		{
			LogError("Called with no owning ground object");
			return{ 0, 0, 0 };
		}
		return s->getPosition();
	}


	void BattleTileObjectLeftWall::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
		TileViewMode mode)
	{
		std::ignore = transform;
		// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
		auto left_wall = this->left_wall.lock();
		if (!left_wall)
		{
			LogError("Called with no owning left_wall object");
			return;
		}
		// FIXME: If damaged use damaged tile sprites?
		auto &type = left_wall->type;
		sp<Image> sprite;
		//sp<Image> overlaySprite;
		Vec2<float> transformedScreenPos = screenPosition;
		switch (mode)
		{
		case TileViewMode::Isometric:
			sprite = type->sprite;
			//overlaySprite = type->overlaySprite;
			transformedScreenPos -= type->imageOffset;
			break;
		case TileViewMode::Strategy:
			sprite = type->strategySprite;
			// All strategy sprites so far are 8x8 so offset by 4 to draw from the center
			// FIXME: Not true for large sprites (2x2 UFOs?)
			transformedScreenPos -= Vec2<float>{4, 4};
			break;
		default:
			LogError("Unsupported view mode");
		}
		if (sprite)
			r.draw(sprite, transformedScreenPos);
		// FIXME: Should be drawn at 'later' Z than left_wall (IE on top of any vehicles on tile?)
		/*if (overlaySprite)
		r.draw(overlaySprite, transformedScreenPos);*/
	}

	BattleTileObjectLeftWall::~BattleTileObjectLeftWall() = default;

	BattleTileObjectLeftWall::BattleTileObjectLeftWall(BattleTileMap &map, sp<BattleLeftWall> left_wall)
		: BattleTileObject(map, Type::LeftWall, Vec3<float>{1, 1, 1}), left_wall(left_wall)
	{
	}

	sp<BattleLeftWall> BattleTileObjectLeftWall::getOwner()
	{
		auto s = this->left_wall.lock();
		if (!s)
		{
			LogError("Owning left_wall object disappeared");
		}
		return s;
	}

	sp<VoxelMap> BattleTileObjectLeftWall::getVoxelMap() { return this->getOwner()->type->voxelMap; }

	Vec3<float> BattleTileObjectLeftWall::getPosition() const
	{
		auto s = this->left_wall.lock();
		if (!s)
		{
			LogError("Called with no owning left_wall object");
			return{ 0, 0, 0 };
		}
		return s->getPosition();
	}



	void BattleTileObjectRightWall::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
		TileViewMode mode)
	{
		std::ignore = transform;
		// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
		auto right_wall = this->right_wall.lock();
		if (!right_wall)
		{
			LogError("Called with no owning right_wall object");
			return;
		}
		// FIXME: If damaged use damaged tile sprites?
		auto &type = right_wall->type;
		sp<Image> sprite;
		//sp<Image> overlaySprite;
		Vec2<float> transformedScreenPos = screenPosition;
		switch (mode)
		{
		case TileViewMode::Isometric:
			sprite = type->sprite;
			//overlaySprite = type->overlaySprite;
			transformedScreenPos -= type->imageOffset;
			break;
		case TileViewMode::Strategy:
			sprite = type->strategySprite;
			// All strategy sprites so far are 8x8 so offset by 4 to draw from the center
			// FIXME: Not true for large sprites (2x2 UFOs?)
			transformedScreenPos -= Vec2<float>{4, 4};
			break;
		default:
			LogError("Unsupported view mode");
		}
		if (sprite)
			r.draw(sprite, transformedScreenPos);
		// FIXME: Should be drawn at 'later' Z than right_wall (IE on top of any vehicles on tile?)
		/*if (overlaySprite)
		r.draw(overlaySprite, transformedScreenPos);*/
	}

	BattleTileObjectRightWall::~BattleTileObjectRightWall() = default;

	BattleTileObjectRightWall::BattleTileObjectRightWall(BattleTileMap &map, sp<BattleRightWall> right_wall)
		: BattleTileObject(map, Type::RightWall, Vec3<float>{1, 1, 1}), right_wall(right_wall)
	{
	}

	sp<BattleRightWall> BattleTileObjectRightWall::getOwner()
	{
		auto s = this->right_wall.lock();
		if (!s)
		{
			LogError("Owning right_wall object disappeared");
		}
		return s;
	}

	sp<VoxelMap> BattleTileObjectRightWall::getVoxelMap() { return this->getOwner()->type->voxelMap; }

	Vec3<float> BattleTileObjectRightWall::getPosition() const
	{
		auto s = this->right_wall.lock();
		if (!s)
		{
			LogError("Called with no owning right_wall object");
			return{ 0, 0, 0 };
		}
		return s->getPosition();
	}


} // namespace OpenApoc

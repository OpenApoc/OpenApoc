#pragma once
#include "library/voxel.h"
#include "framework/includes.h"
#include "game/state/stateobject.h"
#include "game/state/tileview/tile.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{
	#define BATTLE_TILE_X (48)
	#define BATTLE_TILE_Y (24)
	#define BATTLE_TILE_Z (40)

	#define BATTLE_STRAT_TILE_X 8
	#define BATTLE_STRAT_TILE_Y 8

	// 01: TileTypes

	class BattleGroundType : public StateObject<BattleGroundType>
	{
	public:
		sp<Image> sprite;
		sp<Image> strategySprite;
		sp<VoxelMap> voxelMap;
		Vec2<float> imageOffset;
	};

	class BattleLeftWallType : public StateObject<BattleLeftWallType>
	{
	public:
		sp<Image> sprite;
		sp<Image> strategySprite;
		sp<VoxelMap> voxelMap;
		Vec2<float> imageOffset;
	};

	class BattleRightWallType : public StateObject<BattleRightWallType>
	{
	public:
		sp<Image> sprite;
		sp<Image> strategySprite;
		sp<VoxelMap> voxelMap;
		Vec2<float> imageOffset;
	};

	class BattleSceneryType : public StateObject<BattleSceneryType>
	{
	public:
		sp<Image> sprite;
		sp<Image> strategySprite;
		sp<VoxelMap> voxelMap;
		Vec2<float> imageOffset;
	};

	// 02: Battle

	class GameState;
	class BattleTileMap;
	class BattleGround;
	class BattleScenery;
	class BattleLeftWall;
	class BattleRightWall;
	
	class Battle
	{
	public:
		Battle() = default;
		~Battle();

		void start();
		void initMap();

		Vec3<int> size;
		std::map<UString, sp<BattleGroundType>> ground_types;
		std::map<UString, sp<BattleLeftWallType>> left_wall_types;
		std::map<UString, sp<BattleRightWallType>> right_wall_types;
		std::map<UString, sp<BattleSceneryType>> scenery_types;

		std::map<Vec3<int>, StateRef<BattleGroundType>> initial_grounds;
		std::map<Vec3<int>, StateRef<BattleLeftWallType>> initial_left_walls;
		std::map<Vec3<int>, StateRef<BattleRightWallType>> initial_right_walls;
		std::map<Vec3<int>, StateRef<BattleSceneryType>> initial_scenery;
		
		std::set<sp<BattleGround>> ground;
		std::set<sp<BattleScenery>> scenery;
		std::set<sp<BattleLeftWall>> left_wall;
		std::set<sp<BattleRightWall>> right_wall;

		up<BattleTileMap> map;

		void update(GameState &state, unsigned int ticks);
	};

	// 03: BattleTile

	class Image;
	class BattleTileMap;
	class BattleTileObject;
	class CollisionB;
	class VoxelMap;
	class Renderer;
	class TileView;
	
	class BattleTile
	{
	public:
		BattleTileMap &map;
		Vec3<int> position;

		std::set<sp<BattleTileObject>> ownedObjects;
		std::set<sp<BattleTileObject>> intersectingObjects;

		// FIXME: This is effectively a z-sorted list of ownedObjects - can this be merged somehow?
		std::vector<std::vector<sp<BattleTileObject>>> drawnObjects;

		BattleTile(BattleTileMap &map, Vec3<int> position, int layerCount);
	};

	// 05: BattleTileObject

	class BattleTileObject : public std::enable_shared_from_this<BattleTileObject>
	{
	public:
		enum class Type
		{
			Ground, 
			LeftWall,
			RightWall,
			Scenery,
		};

		/* 'screenPosition' is where the center of the object should be drawn */
		virtual void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
			TileViewMode mode) = 0;
		const Type &getType() const { return this->type; }
		virtual Vec3<float> getPosition() const = 0;

		virtual float getDistanceTo(sp<BattleTileObject> target);
		virtual float getDistanceTo(Vec3<float> target);
		virtual void setPosition(Vec3<float> newPosition);
		virtual void removeFromMap();

		BattleTile *getOwningTile() const { return this->owningTile; }

		virtual sp<VoxelMap> getVoxelMap() { return nullptr; }
		virtual Vec3<float> getVoxelOffset() const { return bounds / 2.0f; }

		virtual const UString &getName() { return this->name; }

		virtual ~BattleTileObject();

		BattleTileMap &map;

	protected:
		friend class BattleTileMap;

		Type type;

		BattleTile *owningTile;
		std::vector<BattleTile *> intersectingTiles;

		BattleTileObject(BattleTileMap &map, Type type, Vec3<float> bounds);

		// The bounds is a cube centered around the 'position' used for stuff like collision detection
		Vec3<float> bounds;
		UString name;
	};

	// 04: BattleTileMap

	class BattleTileMap
	{
	private:
		std::vector<BattleTile> tiles;
		std::vector<std::set<BattleTileObject::Type>> layerMap;

	public:
		const BattleTile *getTile(int x, int y, int z) const
		{
			LogAssert(x >= 0);
			LogAssert(x < size.x);
			LogAssert(y >= 0);
			LogAssert(y < size.y);
			LogAssert(z >= 0);
			LogAssert(z < size.z);
			return &this->tiles[z * size.x * size.y + y * size.x + x];
		}
		BattleTile *getTile(int x, int y, int z)
		{
			LogAssert(x >= 0);
			LogAssert(x < size.x);
			LogAssert(y >= 0);
			LogAssert(y < size.y);
			LogAssert(z >= 0);
			LogAssert(z < size.z);
			return &this->tiles[z * size.x * size.y + y * size.x + x];
		}
		BattleTile *getTile(Vec3<int> pos) { return this->getTile(pos.x, pos.y, pos.z); }
		const BattleTile *getTile(Vec3<int> pos) const { return this->getTile(pos.x, pos.y, pos.z); }
		// Returns the tile this point is 'within'
		BattleTile *getTile(Vec3<float> pos)
		{
			return this->getTile(static_cast<int>(pos.x), static_cast<int>(pos.y),
				static_cast<int>(pos.z));
		}
		const BattleTile *getTile(Vec3<float> pos) const
		{
			return this->getTile(static_cast<int>(pos.x), static_cast<int>(pos.y),
				static_cast<int>(pos.z));
		}
		Vec3<int> size;

		BattleTileMap(Vec3<int> size, std::vector<std::set<BattleTileObject::Type>> layerMap);
		~BattleTileMap();

		CollisionB findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd) const;

		void addObjectToMap(sp<BattleGround>);
		void addObjectToMap(sp<BattleLeftWall>);
		void addObjectToMap(sp<BattleRightWall>);
		void addObjectToMap(sp<BattleScenery>);

		int getLayer(BattleTileObject::Type type) const;
		int getLayerCount() const;
		bool tileIsValid(Vec3<int> tile) const;

		sp<Image> dumpVoxelView(const Rect<int> viewRect, const TileTransform &transform) const;
	};

	// 06: BattleXYZ classes

	class BattleTileObjectScenery;
	class BattleTileObjectGround;
	class BattleTileObjectLeftWall;
	class BattleTileObjectRightWall;

	class BattleScenery : public std::enable_shared_from_this<BattleScenery>
	{

	public:
		StateRef<BattleSceneryType> type;

		Vec3<float> getPosition() const
		{
			// The "position" is the center, so offset by {0.5,0.5,0.5}
			Vec3<float> offsetPos = currentPosition;
			offsetPos += Vec3<float>{0.5, 0.5, 0.5};
			return offsetPos;
		}

		Vec3<int> initialPosition;
		Vec3<float> currentPosition;

		bool damaged;
		bool falling;
		bool destroyed;

		void handleCollision(GameState &state, CollisionB &c);

		void update(GameState &state, unsigned int ticks);
		void collapse(GameState &state);

		bool isAlive() const;

		sp<BattleTileObjectScenery> tileObject;
		std::set<sp<BattleScenery>> supports;
		std::set<sp<BattleScenery>> supportedBy;
		StateRef<Battle> battle;

		BattleScenery();
		~BattleScenery() = default;
	};

	class BattleGround : public std::enable_shared_from_this<BattleGround>
	{

	public:
		StateRef<BattleGroundType> type;

		Vec3<float> getPosition() const
		{
			// The "position" is the center, so offset by {0.5,0.5,0.5}
			Vec3<float> offsetPos = currentPosition;
			offsetPos += Vec3<float>{0.5, 0.5, 0.5};
			return offsetPos;
		}

		Vec3<int> initialPosition;
		Vec3<float> currentPosition;

		bool damaged;
		bool falling;
		bool destroyed;

		void handleCollision(GameState &state, CollisionB &c);

		void update(GameState &state, unsigned int ticks);
		void collapse(GameState &state);

		bool isAlive() const;

		sp<BattleTileObjectGround> tileObject;
		std::set<sp<BattleGround>> supports;
		std::set<sp<BattleGround>> supportedBy;
		StateRef<Battle> battle;

		BattleGround();
		~BattleGround() = default;
	};

	class BattleLeftWall : public std::enable_shared_from_this<BattleLeftWall>
	{

	public:
		StateRef<BattleLeftWallType> type;

		Vec3<float> getPosition() const
		{
			// The "position" is the center, so offset by {0.5,0.5,0.5}
			Vec3<float> offsetPos = currentPosition;
			offsetPos += Vec3<float>{0.5, 0.5, 0.5};
			return offsetPos;
		}

		Vec3<int> initialPosition;
		Vec3<float> currentPosition;

		bool damaged;
		bool falling;
		bool destroyed;

		void handleCollision(GameState &state, CollisionB &c);

		void update(GameState &state, unsigned int ticks);
		void collapse(GameState &state);

		bool isAlive() const;

		sp<BattleTileObjectLeftWall> tileObject;
		std::set<sp<BattleLeftWall>> supports;
		std::set<sp<BattleLeftWall>> supportedBy;
		StateRef<Battle> battle;

		BattleLeftWall();
		~BattleLeftWall() = default;
	};

	class BattleRightWall : public std::enable_shared_from_this<BattleRightWall>
	{

	public:
		StateRef<BattleRightWallType> type;

		Vec3<float> getPosition() const
		{
			// The "position" is the center, so offset by {0.5,0.5,0.5}
			Vec3<float> offsetPos = currentPosition;
			offsetPos += Vec3<float>{0.5, 0.5, 0.5};
			return offsetPos;
		}

		Vec3<int> initialPosition;
		Vec3<float> currentPosition;

		bool damaged;
		bool falling;
		bool destroyed;

		void handleCollision(GameState &state, CollisionB &c);

		void update(GameState &state, unsigned int ticks);
		void collapse(GameState &state);

		bool isAlive() const;

		sp<BattleTileObjectRightWall> tileObject;
		std::set<sp<BattleRightWall>> supports;
		std::set<sp<BattleRightWall>> supportedBy;
		StateRef<Battle> battle;

		BattleRightWall();
		~BattleRightWall() = default;
	};

	// 07: BattleTileObjectXYZ classes

	class BattleTileObjectScenery : public BattleTileObject
	{
	public:
		void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
			TileViewMode mode) override;
		~BattleTileObjectScenery() override;

		std::weak_ptr<BattleScenery> scenery;

		sp<BattleScenery> getOwner();

		sp<VoxelMap> getVoxelMap() override;
		Vec3<float> getPosition() const override;

	private:
		friend class BattleTileMap;
		BattleTileObjectScenery(BattleTileMap &map, sp<BattleScenery> scenery);
	};

	class BattleTileObjectGround : public BattleTileObject
	{
	public:
		void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
			TileViewMode mode) override;
		~BattleTileObjectGround() override;

		std::weak_ptr<BattleGround> ground;

		sp<BattleGround> getOwner();

		sp<VoxelMap> getVoxelMap() override;
		Vec3<float> getPosition() const override;

	private:
		friend class BattleTileMap;
		BattleTileObjectGround(BattleTileMap &map, sp<BattleGround> Ground);
	};

	class BattleTileObjectLeftWall : public BattleTileObject
	{
	public:
		void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
			TileViewMode mode) override;
		~BattleTileObjectLeftWall() override;

		std::weak_ptr<BattleLeftWall> left_wall;

		sp<BattleLeftWall> getOwner();

		sp<VoxelMap> getVoxelMap() override;
		Vec3<float> getPosition() const override;

	private:
		friend class BattleTileMap;
		BattleTileObjectLeftWall(BattleTileMap &map, sp<BattleLeftWall> LeftWall);
	};

	class BattleTileObjectRightWall : public BattleTileObject
	{
	public:
		void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
			TileViewMode mode) override;
		~BattleTileObjectRightWall() override;

		std::weak_ptr<BattleRightWall> right_wall;

		sp<BattleRightWall> getOwner();

		sp<VoxelMap> getVoxelMap() override;
		Vec3<float> getPosition() const override;

	private:
		friend class BattleTileMap;
		BattleTileObjectRightWall(BattleTileMap &map, sp<BattleRightWall> RightWall);
	};



}; // namespace OpenApoc

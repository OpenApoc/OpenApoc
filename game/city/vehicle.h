#pragma once

#include "framework/includes.h"
#include "game/tileview/tile.h"
#include "game/tileview/tile_visible.h"
#include "game/tileview/tile_collidable.h"

/* MSVC warns about inherited virtual functions that are implemented
 * in different superclasses though multiple inheritance, even
 * if one is a subclass of the other. So disable that as we rely
 * on inherited subclasses of TileObject overriding various functions */
#ifdef _MSC_VER
#pragma warning( disable : 4250 )
#endif // _MSC_VER


namespace OpenApoc {

class Image;
class VehicleFactory;
class VehicleDefinition;
class VehicleTileObject;
class Vehicle;
class Organisation;

class VehicleMission
{
public:
	Vehicle &vehicle;
	VehicleMission(Vehicle &vehicle);
	virtual Vec3<float> getNextDestination() = 0;
	virtual ~VehicleMission();
};

class VehicleMover
{
public:
	Vehicle &vehicle;
	VehicleMover(Vehicle &vehicle);
	virtual void update(unsigned int ticks) = 0;
	virtual ~VehicleMover();
};

class Vehicle : public std::enable_shared_from_this<Vehicle>
{
public:
	virtual ~Vehicle();
	Vehicle(const VehicleDefinition &def, Organisation &owner);

	const VehicleDefinition &def;
	Organisation &owner;

	enum class Type
	{
		Flying,
		Ground,
	};
	enum class Direction
	{
		N,
		NNE,
		NE,
		NEE,
		E,
		SEE,
		SE,
		SSE,
		S,
		SSW,
		SW,
		SWW,
		W,
		NWW,
		NW,
		NNW,
	};
	enum class Banking
	{
		Flat,
		Left,
		Right,
		Ascending,
		Decending,
	};

	std::shared_ptr<VehicleTileObject> tileObject;
	/* FIXME: Merge with tileObject? */
	Vec3<float> position;
	Vec3<float> direction;

	std::unique_ptr<VehicleMission> mission;
	std::unique_ptr<VehicleMover> mover;

	/* 'launch' the vehicle into the city */
	/* FIXME: Make this take initial mission/mover? */
	void launch(TileMap &map, Vec3<float> initialPosition);
};

class VehicleTileObject : public TileObjectDirectionalSprite, public TileObjectCollidable
{
private:
	Vehicle &vehicle;
public:
	VehicleTileObject(Vehicle &vehicle, TileMap &map, Vec3<float> position);
	virtual ~VehicleTileObject();
	virtual void update(unsigned int ticks);

	using TileObjectCollidable::setPosition;
	using TileObjectCollidable::getTileSizeInVoxels;
	using TileObjectCollidable::getBounds;
	using TileObjectCollidable::hasVoxelAt;
	using TileObjectCollidable::handleCollision;
	using TileObjectCollidable::removeFromAffectedTiles;
	using TileObjectCollidable::addToAffectedTiles;
	using TileObjectDirectionalSprite::getSprite;
};

}; //namespace OpenApoc

#pragma once

#include "framework/includes.h"
#include "game/tileview/tile.h"
#include "game/tileview/tile_visible.h"
#include "game/tileview/tile_collidable.h"
#include "game/rules/vehicledef.h"

#include <deque>

/* MSVC warns about inherited virtual functions that are implemented
 * in different superclasses though multiple inheritance, even
 * if one is a subclass of the other. So disable that as we rely
 * on inherited subclasses of TileObject overriding various functions */
#ifdef _MSC_VER
#pragma warning(disable : 4250)
#endif // _MSC_VER

namespace OpenApoc
{

class Image;
class VehicleFactory;
class VehicleDefinition;
class VehicleTileObject;
class Vehicle;
class Organisation;
class Weapon;
class VehicleMission;
class Building;

class VehicleMover
{
  public:
	Vehicle &vehicle;
	VehicleMover(Vehicle &vehicle);
	virtual void update(unsigned int ticks) = 0;
	virtual ~VehicleMover();
};

class Vehicle : public std::enable_shared_from_this<Vehicle>, public ActiveObject
{
  public:
	virtual ~Vehicle();
	Vehicle(const VehicleDefinition &def, sp<Organisation> owner);

	const VehicleDefinition &def;
	sp<Organisation> owner;

	std::weak_ptr<VehicleTileObject> tileObject;

	std::deque<std::unique_ptr<VehicleMission>> missions;
	std::unique_ptr<VehicleMover> mover;

	/* The building we are current landed in (May be nullptr if in the air) */
	Building *building;

	/* 'launch' the vehicle into the city */
	void launch(TileMap &map, Vec3<float> initialPosition);
	/* 'land' the vehicle in a building*/
	void land(TileMap &map, Building &b);

	std::vector<std::unique_ptr<Weapon>> weapons;

	virtual void update(unsigned int ticks) override;
};

class VehicleTileObject : public TileObjectDirectionalSprite, public TileObjectCollidable
{
  private:
	Vehicle &vehicle;

  public:
	VehicleTileObject(Vehicle &vehicle, TileMap &map, Vec3<float> position);
	virtual ~VehicleTileObject();
	Vec3<float> getDrawPosition() const override;

	virtual Rect<float> getSelectableBounds() const override;
	virtual void setSelected(bool selected) override;

	Vehicle &getVehicle() const { return this->vehicle; }

	using TileObjectCollidable::setPosition;
	using TileObjectCollidable::getTileSizeInVoxels;
	using TileObjectCollidable::getBounds;
	using TileObjectCollidable::hasVoxelAt;
	using TileObjectCollidable::handleCollision;
	using TileObjectCollidable::removeFromAffectedTiles;
	using TileObjectCollidable::addToAffectedTiles;
	using TileObjectDirectionalSprite::getSprite;
};

}; // namespace OpenApoc

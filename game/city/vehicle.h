#pragma once

#include "framework/includes.h"
#include "game/tileview/tile.h"
#include "game/rules/vehicledef.h"

#include <deque>

/* MSVC warns about inherited virtual functions that are implemented
 * in different superclasses though multiple inheritance, even
 * if one is a subclass of the other. So disable that as we rely
 * on inherited subclasses of TileObjectOld overriding various functions */
#ifdef _MSC_VER
#pragma warning(disable : 4250)
#endif // _MSC_VER

namespace OpenApoc
{

class Image;
class VehicleFactory;
class VehicleDefinition;
class TileObjectVehicle;
class TileObjectShadow;
class Vehicle;
class Organisation;
class Weapon;
class VehicleMission;
class Building;
class GameState;

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
	Vehicle(const VehicleDefinition &def, sp<Organisation> owner);

	const VehicleDefinition &def;
	sp<Organisation> owner;

	sp<TileObjectVehicle> tileObject;
	sp<TileObjectShadow> shadowObject;

	std::deque<std::unique_ptr<VehicleMission>> missions;
	std::unique_ptr<VehicleMover> mover;

	/* The building we are current landed in (May be nullptr if in the air) */
	std::weak_ptr<Building> building;

	/* 'launch' the vehicle into the city */
	void launch(TileMap &map, Vec3<float> initialPosition);
	/* 'land' the vehicle in a building*/
	void land(TileMap &map, sp<Building> b);

	std::vector<std::unique_ptr<Weapon>> weapons;
	Vec3<float> position;

	const Vec3<float> &getPosition() const { return this->position; }
	const Vec3<float> &getDirection() const;

	void setPosition(const Vec3<float> &pos);

	virtual void update(GameState &state, unsigned int ticks);
};

}; // namespace OpenApoc

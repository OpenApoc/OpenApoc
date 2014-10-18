#include "vehicle.h"
#include "game/resources/vehiclefactory.h"
#include <cfloat>

namespace OpenApoc {

Vehicle::Vehicle(VehicleDefinition &def)
	: def(def)
{

}

Vehicle::~Vehicle()
{

}

class VehicleSpinner : public VehicleMission
{
public:
	VehicleSpinner(Vehicle &vehicle)
		: VehicleMission(vehicle) {};
	virtual void update(unsigned ticks)
	{
		//TODO: Scale by 'ticks'
		FlyingVehicle &v = dynamic_cast<FlyingVehicle&>(*this->vehicle.tileObject);
		v.direction = glm::rotate(v.direction, 0.01f, Vec3<float>(0, 0, 1));
	}
};

VehicleMission::VehicleMission(Vehicle &v)
	: vehicle(v)
{

}

VehicleMission::~VehicleMission()
{

}

FlyingVehicle::FlyingVehicle(Vehicle &vehicle, Tile &owningTile)
	: TileObject(owningTile, Vec3<float>(owningTile.position), vehicle.def.size, true, true, std::shared_ptr<Image>(nullptr)), vehicle(vehicle), direction(0, 1, 0)
{
	assert(!vehicle.tileObject);
	this->mission.reset(new VehicleSpinner(vehicle));

}

FlyingVehicle::~FlyingVehicle()
{

}

void
FlyingVehicle::update(unsigned int ticks)
{
	if (this->mission)
		mission->update(ticks);
}

const std::vector<std::pair<Vec3<float>, Vehicle::Direction>> directions =
{
	{{ 0, 1, 0}, Vehicle::Direction::N},
	{{ 1, 1, 0}, Vehicle::Direction::NE},
	{{ 1, 0, 0}, Vehicle::Direction::E},
	{{ 1,-1, 0}, Vehicle::Direction::SE},
	{{ 0,-1, 0}, Vehicle::Direction::S},
	{{-1,-1, 0}, Vehicle::Direction::SW},
	{{-1, 0, 0}, Vehicle::Direction::W},
	{{-1, 1, 0}, Vehicle::Direction::NW},
};

static Vehicle::Direction findClosestDirection(Vec3<float> v)
{
	Vehicle::Direction d = Vehicle::Direction::N;
	float a = FLT_MAX;
	for (auto &p : directions)
	{
		float angle = fabs(glm::angle(glm::normalize(p.first), glm::normalize(v)));
		if (angle < a)
		{
			a = angle;
			d = p.second;
		}
	}
	return d;
}

Image&
FlyingVehicle::getSprite()
{
	//TODO: Banking selection logic
	Vehicle::Direction d = findClosestDirection(this->direction);
	return *this->vehicle.def.sprites[Vehicle::Banking::Flat][d];
}

void
FlyingVehicle::processCollision(TileObject &otherObject)
{
	//TODO: Vehicle collision
}

}; //namespace OpenApoc

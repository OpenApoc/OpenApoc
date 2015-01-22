#include "vehicle.h"
#include "game/resources/vehiclefactory.h"
#include <cfloat>
#include <random>

namespace OpenApoc {

Vehicle::Vehicle(VehicleDefinition &def)
	: def(def)
{

}

Vehicle::~Vehicle()
{

}

class VehicleRandomWalk : public VehicleMission
{
public:
	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution;
	VehicleRandomWalk(Vehicle &vehicle)
		: VehicleMission(vehicle), distribution(-1,1)
			{};
	virtual Vec3<float> getNextDestination()
	{
		FlyingVehicle &v = dynamic_cast<FlyingVehicle&>(*this->vehicle.tileObject);
		TileMap &map = v.owningTile->map;
		Vec3<int> nextPosition;
		int tries = 0;
		do {
			nextPosition = {v.position.x, v.position.y, v.position.z};
			Vec3<int> diff {distribution(generator), distribution(generator), distribution(generator)};
			nextPosition += diff;
			//FIXME HACK - abort after some attempts (e.g. if we're completely trapped)
			//and just phase through whatever obstruction is there
			tries++;
			//Keep looping until we find an empty tile within the map
		} while (nextPosition.x >= map.size.x || nextPosition.x < 0 ||
			nextPosition.y >= map.size.y || nextPosition.y < 0 ||
			nextPosition.z >= map.size.z || nextPosition.z < 0
			//FIXME: Proper routing/obstruction handling
			//(This below could cause an infinite loop if a vehicle gets 'trapped'
			|| (tries < 50 && !map.tiles[nextPosition.z][nextPosition.y][nextPosition.z].objects.empty()));
		return Vec3<float>{nextPosition.x, nextPosition.y, nextPosition.z};
	}
};

class FlyingVehicleMover : public VehicleMover
{
public:
	Vec3<float> goalPosition;
	float speed;
	FlyingVehicleMover(Vehicle &v)
		: VehicleMover(v)
	{
		goalPosition.x = -1.0f;
		speed = 0.1f;
	}
	virtual void update(unsigned int ticks)
	{
		FlyingVehicle &v = dynamic_cast<FlyingVehicle&>(*this->vehicle.tileObject);
		if (goalPosition.x == -1.0f)
			goalPosition = v.mission->getNextDestination();
		float distanceLeft = speed * ticks;
		while (distanceLeft > 0)
		{
			Vec3<float> vectorToGoal = goalPosition -
				v.position;
			float distanceToGoal = glm::length(vectorToGoal);
			if (distanceToGoal <= distanceLeft)
			{
				distanceLeft -= distanceToGoal;
				v.position = goalPosition;
				goalPosition = v.mission->getNextDestination();
			}
			else
			{
				v.direction = vectorToGoal;
				v.position += distanceLeft * glm::normalize(vectorToGoal);
				distanceLeft = -1;
			}
		}
		Vec3<int> currentTile{v.position.x, v.position.y, v.position.z};
		if (currentTile != v.owningTile->position)
		{
			for (auto o : v.owningTile->objects)
			{
				if (o.get() == &v)
				{
					auto &map = v.owningTile->map;
					v.owningTile->objects.remove(o);
					v.owningTile = &map.tiles[currentTile.z][currentTile.y][currentTile.x];
					v.owningTile->objects.push_back(o);
					break;
				}
			}
		}

		//FIXME: change owning tile?
		//I've possibly backed myself into a corner here, how I can 'change' the owning tile
		//when the update loop is current iterating over said list? Removing it causes the
		//parent iterator to become invalid (crashing), and adding it to a new tile
		//could cause it to be updated again when /that/ tile is processed.
	}
};

VehicleMission::VehicleMission(Vehicle &v)
	: vehicle(v)
{

}

VehicleMission::~VehicleMission()
{

}

VehicleMover::VehicleMover(Vehicle &v)
: vehicle(v)
{

}

VehicleMover::~VehicleMover()
{

}
FlyingVehicle::FlyingVehicle(Vehicle &vehicle, Tile *owningTile)
	: TileObject(owningTile, Vec3<float>(owningTile->position), vehicle.def.size, true, true, std::shared_ptr<Image>(nullptr)), vehicle(vehicle), direction(0, 1, 0)
{
	assert(!vehicle.tileObject);
	this->mission.reset(new VehicleRandomWalk(vehicle));
	this->mover.reset(new FlyingVehicleMover(vehicle));

}

FlyingVehicle::~FlyingVehicle()
{

}

void
FlyingVehicle::update(unsigned int ticks)
{
	if (this->mover)
		mover->update(ticks);
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
	//Reset Z (We don't care about ascending/decending)
	v.z = 0;
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

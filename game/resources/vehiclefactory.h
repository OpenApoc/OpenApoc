#pragma once

#include "framework/includes.h"
#include "game/city/vehicle.h"

namespace OpenApoc {

class Vehicle;
class Organisation;

class VehicleDefinition
{
public:
	UString name;
	Vehicle::Type type;
	std::map<Vehicle::Banking, std::map<Vehicle::Direction, std::shared_ptr<Image> > > sprites;
	//The same sprites but using vectors for directions
	std::vector<std::pair<Vec3<float>, std::shared_ptr<Image>>> directionalSprites;
	Vec3<float> size;
	std::shared_ptr<VoxelMap> voxelMap;
};

class VehicleFactory
{
private:

	Framework &fw;

	std::map<UString, VehicleDefinition> defs;
public:
	VehicleFactory(Framework &fw);
	~VehicleFactory();
	void ParseVehicleDefinition(tinyxml2::XMLElement *root);
	std::shared_ptr<Vehicle> create(const UString name, Organisation &owner);
};

};

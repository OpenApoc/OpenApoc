#pragma once

#include "framework/includes.h"
#include "game/city/vehicle.h"

namespace OpenApoc {

class Vehicle;

class VehicleDefinition
{
public:
	UString name;
	Vehicle::Type type;
	std::map<Vehicle::Banking, std::map<Vehicle::Direction, std::shared_ptr<Image> > > sprites;
	Vec3<float> size;
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
	std::shared_ptr<Vehicle> create(const UString name);
};

};

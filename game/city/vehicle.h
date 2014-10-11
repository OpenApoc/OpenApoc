#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Image;

class VehicleMission
{
public:
	virtual ~VehicleMission();
};

class Vehicle
{
public:
	virtual ~Vehicle();
	virtual Image& getImage() = delete;
	virtual Vec3<float> getCityPosition() = delete;
	virtual void update(float delta) = delete;
};

}; //namespace OpenApoc

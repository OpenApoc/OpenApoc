#pragma once

#include "game/state/stateobject.h"
#include <map>

namespace OpenApoc
{
class VehicleType;

class OrganisationRaid
{
  public:
	enum class Type
	{
		None = 0,
		Attack = 1,
		Raid = 2,
		Storm = 3,
		UnauthorizedVehicle = 4,
		Treaty = 5
	};

	int nextRaidTimer = 0;

	int max_attack_vehicles = 5;
	std::vector<StateRef<VehicleType>> attack_vehicle_types;
	std::map<Type, float> neutral_low_manpower;
	std::map<Type, float> neutral_normal;
	std::map<Type, float> neutral_high_manpower;

	std::map<Type, float> military_low_manpower;
	std::map<Type, float> military_normal;
	std::map<Type, float> military_high_manpower;
};

}; // namespace OpenApoc

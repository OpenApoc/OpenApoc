#pragma once

#include <map>

namespace OpenApoc
{

class OrganisationRaid
{
  public:
	enum class Type
	{
		None = 0,
		Attack = 1,
		Raid = 2,
		Storm = 3,
		IllegalFlyer = 4,
		Treaty = 5
	};

	std::map<Type, float> neutral_low_manpower;
	std::map<Type, float> neutral_normal;
	std::map<Type, float> neutral_high_manpower;

	std::map<Type, float> military_low_manpower;
	std::map<Type, float> military_normal;
	std::map<Type, float> military_high_manpower;
};

}; // namespace OpenApoc
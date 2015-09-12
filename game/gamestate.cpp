#include "game/gamestate.h"
#include "game/city/city.h"
#include "game/city/weapon.h"
#include "game/city/building.h"
#include "game/city/vehicle.h"
#include "game/organisation.h"

#include "framework/includes.h"
#include "framework/framework.h"

#include <random>

namespace OpenApoc
{

GameState::GameState(Framework &fw, Rules &rules)
    : showTileOrigin(false), showVehiclePath(false), showSelectableBounds(false)
{
	for (auto &orgdef : rules.getOrganisationDefs())
	{
		this->organisations.emplace_back(orgdef);
	}

	this->city.reset(new City(fw, *this));

	// Place some random testing vehicles
	std::uniform_int_distribution<int> bld_distribution(0, this->city->buildings.size() - 1);

	auto weaponIt = rules.getWeaponDefs().begin();

	for (int i = 0; i < 10; i++)
	{
		auto vehicleDefIt = fw.rules->getVehicleDefs().find("POLICE_HOVERCAR");
		if (vehicleDefIt == fw.rules->getVehicleDefs().end())
		{
			LogError("No POLICE_HOVERCAR vehicle def found?");
			return;
		}
		// Organisations[3] == megapol
		auto testVehicle = std::make_shared<Vehicle>(vehicleDefIt->second, this->organisations[3]);

		auto &weaponDef = weaponIt->second;
		LogInfo("Equipping with weapon \"%s\"", weaponDef.name.str().c_str());

		weaponIt++;
		if (weaponIt == rules.getWeaponDefs().end())
			weaponIt = rules.getWeaponDefs().begin();

		auto *testWeapon = new Weapon(weaponDef, testVehicle, weaponDef.ammoCapacity);
		testVehicle->weapons.emplace_back(testWeapon);

		this->city->vehicles.push_back(testVehicle);
		auto &b = this->city->buildings[bld_distribution(rng)];
		b.landed_vehicles.insert(testVehicle);
		testVehicle->building = &b;
		city->activeObjects.insert(std::dynamic_pointer_cast<ActiveObject>(testVehicle));
	}
	for (int i = 0; i < 10; i++)
	{
		auto vehicleDefIt = fw.rules->getVehicleDefs().find("PHOENIX_HOVERCAR");
		if (vehicleDefIt == fw.rules->getVehicleDefs().end())
		{
			LogError("No PHOENIX_HOVERCAR vehicle def found?");
			return;
		}
		// Organisations[3] == X-Com
		auto testVehicle = std::make_shared<Vehicle>(vehicleDefIt->second, this->organisations[0]);
		this->city->vehicles.push_back(testVehicle);

		auto &weaponDef = weaponIt->second;
		LogInfo("Equipping with weapon \"%s\"", weaponDef.name.str().c_str());

		weaponIt++;
		if (weaponIt == rules.getWeaponDefs().end())
			weaponIt = rules.getWeaponDefs().begin();

		auto *testWeapon = new Weapon(weaponDef, testVehicle, weaponDef.ammoCapacity);
		testVehicle->weapons.emplace_back(testWeapon);
		auto &b = this->city->buildings[bld_distribution(rng)];
		b.landed_vehicles.insert(testVehicle);
		testVehicle->building = &b;
		city->activeObjects.insert(std::dynamic_pointer_cast<ActiveObject>(testVehicle));
	}
}

}; // namespace OpenApoc

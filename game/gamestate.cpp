#include "game/gamestate.h"
#include "game/city/city.h"
#include "game/city/weapon.h"

#include "framework/includes.h"
#include "framework/framework.h"

#include <random>

namespace OpenApoc {

GameState::GameState(Framework &fw, Rules &rules)
	:showVehiclePath(false)
{
	for (auto &orgdef : rules.getOrganisationDefs())
	{
		this->organisations.emplace_back(orgdef);
	}

	this->city.reset(new City(fw, *this));

	//Place some random testing vehicles
	std::default_random_engine generator;
	//FIXME: Read size from city? (Only 'temporary' code anyway)
	std::uniform_int_distribution<int> xydistribution(0,99);
	// Spawn vehicles at the top to avoid creating them in small
	// inescapeable gaps in buildings
	std::uniform_int_distribution<int> zdistribution(8,9);

	auto weaponIt = rules.getWeaponDefs().begin();

	for (int i = 0; i < 50; i++)
	{
		int x = 0;
		int y = 0;
		int z = 0;
		do {
			x = xydistribution(generator);
			y = xydistribution(generator);
			z = zdistribution(generator);
		} while(!this->city->getTile(x,y,z)->ownedObjects.empty());

		auto vehicleDefIt = fw.rules->getVehicleDefs().find("POLICE_HOVERCAR");
		if (vehicleDefIt == fw.rules->getVehicleDefs().end())
		{
			LogError("No POLICE_HOVERCAR vehicle def found?");
			return;
		}
		//Organisations[3] == megapol
		auto testVehicle = std::make_shared<Vehicle>(vehicleDefIt->second, this->organisations[3]);
		
		auto &weaponDef = weaponIt->second;
		LogWarning("Equipping with weapon \"%s\"", weaponDef.name.str().c_str());

		weaponIt++;
		if (weaponIt == rules.getWeaponDefs().end())
			weaponIt = rules.getWeaponDefs().begin();
		
		auto *testWeapon = new Weapon(weaponDef, testVehicle, weaponDef.ammoCapacity);
		testVehicle->weapons.emplace_back(testWeapon);

		this->city->vehicles.push_back(testVehicle);
		testVehicle->launch(*this->city, Vec3<float>{x,y,z});
	}
	for (int i = 0; i < 50; i++)
	{
		int x = 0;
		int y = 0;
		int z = 0;
		do {
			x = xydistribution(generator);
			y = xydistribution(generator);
			z = zdistribution(generator);
		} while(!this->city->getTile(x,y,z)->ownedObjects.empty());

		auto vehicleDefIt = fw.rules->getVehicleDefs().find("PHOENIX_HOVERCAR");
		if (vehicleDefIt == fw.rules->getVehicleDefs().end())
		{
			LogError("No PHOENIX_HOVERCAR vehicle def found?");
			return;
		}
		//Organisations[3] == X-Com
		auto testVehicle = std::make_shared<Vehicle>(vehicleDefIt->second, this->organisations[0]);
		this->city->vehicles.push_back(testVehicle);

		
		auto &weaponDef = weaponIt->second;
		LogWarning("Equipping with weapon \"%s\"", weaponDef.name.str().c_str());

		weaponIt++;
		if (weaponIt == rules.getWeaponDefs().end())
			weaponIt = rules.getWeaponDefs().begin();
		
		auto *testWeapon = new Weapon(weaponDef, testVehicle, weaponDef.ammoCapacity);
		testVehicle->weapons.emplace_back(testWeapon);

		testVehicle->launch(*this->city, Vec3<float>{x,y,z});
	}
}


}; //namespace OpenApoc

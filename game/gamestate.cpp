#include "game/gamestate.h"
#include "game/city/city.h"
#include "game/city/weapon.h"
#include "game/city/building.h"
#include "game/city/vehicle.h"
#include "game/organisation.h"
#include "game/base/base.h"

#include "framework/includes.h"
#include "framework/framework.h"

#include <random>
#include <sstream>

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

	for (int i = 0; i < 50; i++)
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
		LogInfo("Equipping with weapon \"%s\"", weaponDef.name.c_str());

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
	for (int i = 0; i < 50; i++)
	{
		auto vehicleDefIt = fw.rules->getVehicleDefs().find("PHOENIX_HOVERCAR");
		if (vehicleDefIt == fw.rules->getVehicleDefs().end())
		{
			LogError("No PHOENIX_HOVERCAR vehicle def found?");
			return;
		}
		// Organisations[0] == X-Com
		auto testVehicle = std::make_shared<Vehicle>(vehicleDefIt->second, this->organisations[0]);
		this->city->vehicles.push_back(testVehicle);

		auto &weaponDef = weaponIt->second;
		LogInfo("Equipping with weapon \"%s\"", weaponDef.name.c_str());

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

	// Place a random testing base
	this->organisations[0].balance = 999999;
	auto &b = this->city->buildings[bld_distribution(rng)];
	b.owner.balance = 999999;
	this->bases.emplace_back(b);
	auto &base = this->bases.back();
	base.name = "Test Base";
	std::uniform_int_distribution<int> facilityPos(0, Base::SIZE - 1);
	for (auto &i : rules.getFacilityDefs())
	{
		base.buildFacility(i.second, {facilityPos(rng), facilityPos(rng)});
	}
}

// Just a handy shortcut since it's shown on every single screen
UString GameState::getPlayerBalance() const
{
	std::stringstream ss;
	ss << this->organisations[0].balance;
	return ss.str();
}

}; // namespace OpenApoc

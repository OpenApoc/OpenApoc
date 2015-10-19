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
    : player(nullptr), showTileOrigin(false), showVehiclePath(false), showSelectableBounds(false),
      rng(std::random_device{}())
{
	for (auto &org : rules.getOrganisations())
	{
		if (this->organisations.find(org.ID) != this->organisations.end())
		{
			LogError("Multiple organisations with ID \"%s\"", org.ID.c_str());
		}
		this->organisations[org.ID] = org;
		/* FIXME: Make 'player' organisation selectable? */
		if (org.ID == "ORG_X-COM")
		{
			this->player = &this->organisations[org.ID];
		}
	}

	if (!this->player)
	{
		LogError("No player organisation defined");
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
		auto testVehicle =
		    std::make_shared<Vehicle>(vehicleDefIt->second, this->getOrganisation("ORG_MEGAPOL"));

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
		auto testVehicle = std::make_shared<Vehicle>(vehicleDefIt->second, this->getPlayer());
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

	std::uniform_int_distribution<int> base_distribution(0, this->city->baseBuildings.size() - 1);
	auto base = this->city->baseBuildings[base_distribution(rng)]->base;
	this->playerBases.emplace_back(base);
	// base->building.owner = this->organisations[0];
	base->name = "Test Base";
	std::uniform_int_distribution<int> facilityPos(0, Base::SIZE - 1);
	for (auto &i : rules.getFacilityDefs())
	{
		if (!i.second.fixed)
			base->buildFacility(i.second, {facilityPos(rng), facilityPos(rng)});
	}
}

// Just a handy shortcut since it's shown on every single screen
UString GameState::getPlayerBalance() const
{
	std::ostringstream ss;
	ss << this->getPlayer().balance;
	return ss.str();
}

Organisation &GameState::getOrganisation(const UString &orgID)
{
	auto f = this->organisations.find(orgID);
	if (f == this->organisations.end())
	{
		LogError("No organisation matching ID \"%s\"", orgID.c_str());
	}
	return f->second;
}

Organisation &GameState::getPlayer() const { return *this->player; }

}; // namespace OpenApoc

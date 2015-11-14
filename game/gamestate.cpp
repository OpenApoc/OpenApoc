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
		this->organisations[org.ID] = std::make_shared<Organisation>(org);
		/* FIXME: Make 'player' organisation selectable? */
		if (org.ID == "ORG_X-COM")
		{
			this->player = this->organisations[org.ID];
		}
	}

	if (!this->player)
	{
		LogError("No player organisation defined");
	}

	this->city.reset(new City(fw, *this));

	// Place some random testing vehicles
	std::uniform_int_distribution<int> bld_distribution(0, this->city->buildings.size() - 1);

	// Loop through all vehicle types and weapons to get a decent spread for testing
	auto weaponIt = rules.getWeaponDefs().begin();
	auto vehicleDefIt = rules.getVehicleDefs().begin();

	for (int i = 0; i < 100; i++)
	{

		auto testVehicle = std::make_shared<Vehicle>(
		    vehicleDefIt->second, this->getOrganisation((vehicleDefIt->second.manufacturer)));

		auto &weaponDef = weaponIt->second;
		LogInfo("Equipping with weapon \"%s\"", weaponDef.name.c_str());

		weaponIt++;
		if (weaponIt == rules.getWeaponDefs().end())
			weaponIt = rules.getWeaponDefs().begin();
		vehicleDefIt++;
		if (vehicleDefIt == rules.getVehicleDefs().end())
			vehicleDefIt = rules.getVehicleDefs().begin();

		auto *testWeapon = new Weapon(weaponDef, testVehicle, weaponDef.ammoCapacity);
		testVehicle->weapons.emplace_back(testWeapon);

		this->city->vehicles.push_back(testVehicle);
		auto b = this->city->buildings[bld_distribution(rng)];
		b->landed_vehicles.insert(testVehicle);
		testVehicle->building = b;
	}

	if (this->city->baseBuildings.empty())
	{
		LogError("No valid base buildings");
	}
	std::uniform_int_distribution<int> base_distribution(0, this->city->baseBuildings.size() - 1);
	auto base = this->city->baseBuildings[base_distribution(rng)]->base;
	this->playerBases.emplace_back(base);
	base->name = "Test Base";
	base->bld.lock()->owner = this->getPlayer();
	base->startingBase(fw, rng);
}

// Just a handy shortcut since it's shown on every single screen
UString GameState::getPlayerBalance() const
{
	return Strings::FromInteger(this->getPlayer()->balance);
}

sp<Organisation> GameState::getOrganisation(const UString &orgID)
{
	auto f = this->organisations.find(orgID);
	if (f == this->organisations.end())
	{
		LogError("No organisation matching ID \"%s\"", orgID.c_str());
	}
	return f->second;
}

sp<Organisation> GameState::getPlayer() const { return this->player; }

}; // namespace OpenApoc

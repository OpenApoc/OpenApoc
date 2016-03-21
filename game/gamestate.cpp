#include "game/gamestate.h"
#include "game/city/city.h"
#include "game/city/vequipment.h"
#include "game/city/building.h"
#include "game/city/vehicle.h"
#include "game/city/vehiclemission.h"
#include "game/city/scenery.h"
#include "game/organisation.h"
#include "game/base/base.h"
#include "game/rules/vequipment.h"

#include "framework/includes.h"
#include "framework/framework.h"
#include "game/tileview/tileobject_vehicle.h"

#include <random>

namespace OpenApoc
{

GameState::GameState()
    : player(this), showTileOrigin(false), showVehiclePath(false), showSelectableBounds(false),
      time(0)
{
}

GameState::~GameState()
{
	for (auto &v : this->vehicles)
	{
		auto vehicle = v.second;
		if (vehicle->tileObject)
		{
			vehicle->tileObject->removeFromMap();
		}
		vehicle->tileObject = nullptr;
		// Detatch some back-pointers otherwise we get circular sp<> depedencies and leak
		// FIXME: This is not a 'good' way of doing this, maybe add a destroyVehicle() function? Or
		// make StateRefWeak<> or something?
		//
		vehicle->city = "";
		vehicle->homeBuilding = "";
		vehicle->currentlyLandedBuilding = "";
		vehicle->missions.clear();
		vehicle->equipment.clear();
	}
}

// Just a handy shortcut since it's shown on every single screen
UString GameState::getPlayerBalance() const
{
	return Strings::FromInteger(this->getPlayer()->balance);
}

StateRef<Organisation> GameState::getOrganisation(const UString &orgID)
{
	return StateRef<Organisation>(this, orgID);
}

StateRef<Organisation> GameState::getPlayer() const { return this->player; }

bool GameState::isValid()
{
	if (!this->rules.isValid())
		return false;

	return true;
}

void GameState::initState()
{
	for (auto &city : this->cities)
	{
		city.second->initMap();
		for (auto &v : this->vehicles)
		{
			auto vehicle = v.second;
			if (vehicle->city == city.second)
			{
				city.second->map->addObjectToMap(vehicle);
			}
		}
	}
}

void GameState::startGame()
{
	for (auto &pair : this->cities)
	{
		auto &city = pair.second;
		// Start the game with all buildings whole
		for (auto &tilePair : city->initial_tiles)
		{
			auto s = mksp<Scenery>();

			s->type = tilePair.second;
			s->initialPosition = tilePair.first;
			s->currentPosition = s->initialPosition;

			city->scenery.insert(s);
		}
	}

	// FIXME: How to create vehicles with unique name?
	int vehicle_count = 0;

	auto buildingIt = this->cities["CITYMAP_HUMAN"]->buildings.begin();

	// Create some random vehicles
	for (int i = 0; i < 10; i++)
	{
		for (auto vehicleType : this->vehicle_types)
		{
			auto &type = vehicleType.second;
			if (type->type != VehicleType::Type::Flying)
				continue;
			if (type->manufacturer == this->getPlayer())
				continue;

			auto v = mksp<Vehicle>();
			v->type = {this, vehicleType.first};
			v->name = type->name;
			v->city = {this, "CITYMAP_HUMAN"};
			v->currentlyLandedBuilding = {this, buildingIt->first};
			v->owner = type->manufacturer;
			v->health = type->health;
			auto vID = UString::format("%s%d", Vehicle::getPrefix().c_str(), vehicle_count++);

			buildingIt++;
			if (buildingIt == this->cities["CITYMAP_HUMAN"]->buildings.end())
				buildingIt = this->cities["CITYMAP_HUMAN"]->buildings.begin();

			// Vehicle::equipDefaultEquipment uses the state reference from itself, so make sure the
			// vehicle table has the entry before calling it
			this->vehicles[vID] = v;

			v->equipDefaultEquipment(*this);

			v->missions.emplace_back(VehicleMission::gotoLocation(*v, {0, 0, 0}));
		}
	}

	// Create the intial starting base
	// Randomly shuffle buildings until we find one with a base layout
	sp<City> humanCity = this->cities["CITYMAP_HUMAN"];
	int buildingCount = humanCity->buildings.size();

	std::vector<sp<Building>> buildingsWithBases;
	for (auto &b : humanCity->buildings)
	{
		if (b.second->base_layout)
			buildingsWithBases.push_back(b.second);
	}

	if (buildingsWithBases.empty())
	{
		LogError("City map has no buildings with valid base layouts");
	}

	std::uniform_int_distribution<int> bldDist(0, buildingsWithBases.size() - 1);

	auto bld = buildingsWithBases[bldDist(this->rng)];

	auto base = mksp<Base>(*this, StateRef<Building>{this, bld});
	base->startingBase(*this, this->rng);
	// FIXME: Make the base names increment (NEED TO BE UNIQUE!!)
	this->player_bases[Base::getPrefix() + "1"] = base;
	bld->owner = this->getPlayer();

	// Give the player one of each equi-able vehicle
	for (auto &it : this->vehicle_types)
	{
		auto &type = it.second;
		if (!type->equipment_screen)
			continue;
		auto v = mksp<Vehicle>();
		v->type = {this, type};
		v->name = type->name;
		v->city = {this, "CITYMAP_HUMAN"};
		v->currentlyLandedBuilding = {this, bld};
		v->homeBuilding = {this, bld};
		v->owner = this->getPlayer();
		v->health = type->health;
		auto vID = UString::format("%s%d", Vehicle::getPrefix().c_str(), vehicle_count++);
		this->vehicles[vID] = v;
		v->equipDefaultEquipment(*this);
	}
	// Give that base some inventory
	for (auto &pair : this->vehicle_equipment)
	{
		auto &equipmentID = pair.first;
		base->inventory[equipmentID] = 10;
	}
}

}; // namespace OpenApoc

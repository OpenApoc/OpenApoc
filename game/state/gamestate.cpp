#include "game/state/gamestate.h"
#include "framework/trace.h"
#include "game/state/base/base.h"
#include "game/state/base/facility.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/organisation.h"
#include "game/state/tileview/tileobject_vehicle.h"
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
		vehicle->mover = nullptr;
	}
	for (auto &b : this->player_bases)
	{
		for (auto &f : b.second->facilities)
		{
			if (f->lab)
				f->lab->current_project = "";
			f->lab = "";
		}
	}
	for (auto &org : this->organisations)
	{
		org.second->current_relations.clear();
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

void GameState::initState()
{
	for (auto &city : this->cities)
	{
		city.second->initMap();
		for (auto &v : this->vehicles)
		{
			auto vehicle = v.second;
			if (vehicle->city == city.second && !vehicle->currentlyLandedBuilding)
			{
				city.second->map->addObjectToMap(vehicle);
			}
		}
	}
	for (auto &v : this->vehicles)
	{
		if (!v.second->currentlyLandedBuilding)
		{
			v.second->setupMover();
		}
	}
	for (auto &c : this->cities)
	{
		auto &city = c.second;
		for (auto &s : city->scenery)
		{
			for (auto &b : city->buildings)
			{
				auto &building = b.second;
				Vec2<int> pos2d{s->initialPosition.x, s->initialPosition.y};
				if (building->bounds.within(pos2d))
				{
					s->building = {this, building};
				}
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

			v->currentlyLandedBuilding->landed_vehicles.insert({this, vID});

			v->equipDefaultEquipment(*this);
		}
	}

	// Create the intial starting base
	// Randomly shuffle buildings until we find one with a base layout
	sp<City> humanCity = this->cities["CITYMAP_HUMAN"];
	this->current_city = {this, humanCity};

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
	base->startingBase(*this);
	base->name = "Base " + Strings::FromInteger(this->player_bases.size() + 1);
	this->player_bases[Base::getPrefix() + Strings::FromInteger(this->player_bases.size() + 1)] =
	    base;
	bld->owner = this->getPlayer();

	// Give the player one of each equipable vehicle
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
		v->currentlyLandedBuilding->landed_vehicles.insert({this, vID});
		v->equipDefaultEquipment(*this);
	}
	// Give that base some inventory
	for (auto &pair : this->vehicle_equipment)
	{
		auto &equipmentID = pair.first;
		base->inventory[equipmentID] = 10;
	}

	for (auto &agentTypePair : this->initial_agents)
	{
		auto type = agentTypePair.first;
		auto count = agentTypePair.second;
		while (count > 0)
		{
			auto agent = this->agent_generator.createAgent(*this, type);
			agent->home_base = {this, base};
			agent->owner = this->getPlayer();
			count--;
		}
	}
}

bool GameState::canTurbo() const
{
	if (!this->current_city->projectiles.empty())
	{
		return false;
	}
	for (auto &v : this->vehicles)
	{
		if (v.second->city == this->current_city && v.second->tileObject != nullptr &&
		    v.second->owner->isRelatedTo(this->getPlayer()) == Organisation::Relation::Hostile)
		{
			return false;
		}
	}
	return true;
}

void GameState::update(unsigned int ticks)
{
	Trace::start("GameState::update::cities");
	for (auto &c : this->cities)
	{
		c.second->update(*this, ticks);
	}
	Trace::end("GameState::update::cities");
	Trace::start("GameState::update::vehicles");
	for (auto &v : this->vehicles)
	{
		v.second->update(*this, ticks);
	}
	Trace::end("GameState::update::vehicles");
	Trace::start("GameState::update::labs");
	for (auto &lab : this->research.labs)
	{
		Lab::update(ticks, {this, lab.second}, shared_from_this());
	}
	Trace::end("GameState::update::labs");
	this->time += ticks;
}

void GameState::update() { this->update(1); }

void GameState::updateTurbo()
{
	if (!this->canTurbo())
	{
		LogError("Called when canTurbo() is false");
	}
	unsigned ticksToUpdate = TURBO_TICKS;
	// Turbo always re-aligns to TURBO_TICKS (5 minutes)
	if (this->time % TURBO_TICKS)
	{
		ticksToUpdate -= ticksToUpdate % TURBO_TICKS;
	}
	this->update(ticksToUpdate);
}

}; // namespace OpenApoc

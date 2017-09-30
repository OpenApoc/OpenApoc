#include "game/state/gamestate.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "game/state/aequipment.h"
#include "game/state/base/base.h"
#include "game/state/base/facility.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlecommonsamplelist.h"
#include "game/state/battle/battlemap.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/battle/battleunitanimationpack.h"
#include "game/state/battle/battleunitimagepack.h"
#include "game/state/city/baselayout.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/citycommonimagelist.h"
#include "game/state/city/citycommonsamplelist.h"
#include "game/state/city/doodad.h"
#include "game/state/city/projectile.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gameevent.h"
#include "game/state/gametime.h"
#include "game/state/message.h"
#include "game/state/organisation.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/damage.h"
#include "game/state/rules/doodad_type.h"
#include "game/state/rules/ufo_growth.h"
#include "game/state/rules/ufo_incursion.h"
#include "game/state/rules/vammo_type.h"
#include "game/state/rules/vehicle_type.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_vehicle.h"
#include "game/state/ufopaedia.h"
#include "library/strings_format.h"
#include <random>

// ENABLE/DISABLE FAKE TRAFFIC
#define FAKE_TRAFFIC

namespace OpenApoc
{

GameState::GameState() : player(this) {}

GameState::~GameState()
{
	if (this->current_battle)
	{
		Battle::finishBattle(*this);
		Battle::exitBattle(*this);
	}
	for (auto &a : this->agents)
	{
		a.second->destroy();
	}
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
		vehicle->currentBuilding = "";
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
	return Strings::fromInteger(this->getPlayer()->balance);
}

StateRef<Organisation> GameState::getOrganisation(const UString &orgID)
{
	return StateRef<Organisation>(this, orgID);
}

const StateRef<Organisation> &GameState::getPlayer() const { return this->player; }
StateRef<Organisation> GameState::getPlayer() { return this->player; }
const StateRef<Organisation> &GameState::getAliens() const { return this->aliens; }
StateRef<Organisation> GameState::getAliens() { return this->aliens; }
const StateRef<Organisation> &GameState::getCivilian() const { return this->civilian; }
StateRef<Organisation> GameState::getCivilian() { return this->civilian; }

void GameState::initState()
{
	// FIXME: reseed rng when game starts

	if (current_battle)
		current_battle->initBattle(*this);

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
	for (auto &c : this->cities)
	{
		auto &city = c.second;
		city->initMap(*this);
		for (auto &v : this->vehicles)
		{
			auto vehicle = v.second;
			if (vehicle->city == city && !vehicle->currentBuilding)
			{
				city->map->addObjectToMap(vehicle);
			}
		}
		for (auto &p : c.second->projectiles)
		{
			if (p->trackedVehicle)
				p->trackedObject = p->trackedVehicle->tileObject;
		}
		if (city->portals.empty())
		{
			city->generatePortals(*this);
		}
	}
	for (auto &v : this->vehicles)
	{
		v.second->strategyImages = city_common_image_list->strategyImages;
		if (!v.second->currentBuilding)
		{
			v.second->setupMover();
		}
	}
	// Fill links for weapon's ammo
	for (auto &t : this->agent_equipment)
	{
		for (auto &w : t.second->weapon_types)
		{
			w->ammo_types.emplace_back(this, t.first);
		}
	}
	for (auto &a : this->agent_types)
	{
		a.second->gravLiftSfx = battle_common_sample_list->gravlift;
	}
	for (auto &a : this->agents)
	{
		a.second->leftHandItem = a.second->getFirstItemInSlot(EquipmentSlotType::LeftHand, false);
		a.second->rightHandItem = a.second->getFirstItemInSlot(EquipmentSlotType::RightHand, false);
	}
	// Run nessecary methods for different types
	research.updateTopicList();
}

void GameState::fillOrgStartingProperty()
{
	auto buildingIt = this->cities["CITYMAP_HUMAN"]->buildings.begin();

	// Create some random vehicles
	for (int i = 0; i < 5; i++)
	{
#ifndef FAKE_TRAFFIC
		break;
#endif
#ifdef PATHFINDING_DEBUG
		break;
#endif
		for (auto &vehicleType : this->vehicle_types)
		{
			auto &type = vehicleType.second;
			if (type->type != VehicleType::Type::Flying)
				continue;
			if (type->manufacturer == this->getPlayer())
				continue;

			auto v = cities["CITYMAP_HUMAN"]->placeVehicle(
			    *this, {this, vehicleType.first}, type->manufacturer, {this, buildingIt->first});
			v->homeBuilding = {this, buildingIt->first};

			buildingIt++;
			if (buildingIt == this->cities["CITYMAP_HUMAN"]->buildings.end())
				buildingIt = this->cities["CITYMAP_HUMAN"]->buildings.begin();

			// Vehicle::equipDefaultEquipment uses the state reference from itself, so make sure the
			// vehicle table has the entry before calling it
		}
	}
}

void GameState::startGame()
{
	agentEquipmentTemplates.resize(10);

	// Setup orgs
	for (auto &pair : this->organisations)
	{
		pair.second->ticksTakeOverAttemptAccumulated =
		    randBoundsExclusive(rng, (unsigned)0, TICKS_PER_TAKEOVER_ATTEMPT);
		// Initial relationship randomiser
		// Not for player or civilians
		if (pair.first == player.id || pair.first == civilian.id)
		{
			continue;
		}
		for (auto &entry : pair.second->current_relations)
		{
			// Not for civilians or perfect relationships
			if (entry.second == 100.0f || entry.first == civilian)
			{
				continue;
			}
			// First step: adjust based on difficulty
			// higher difficulty will produce a bigger sway
			if (difficulty > 0)
			{
				// Relationship vs player is adjusted by flat 0/5/0/-5/10
				if (entry.first == player)
				{
					entry.second += 10 - 5 * difficulty;
				}
				// Positive relationship is improved randomly
				else if (entry.second >= 0.0f)
				{
					entry.second += randBoundsInclusive(rng, 0, 3 * difficulty);
				}
				// Negative relationship with non-aliens is worsened randomly
				else if (entry.first != aliens)
				{
					entry.second -= randBoundsInclusive(rng, 0, 5 * difficulty);
				}
			}
			// Second step: random +- 10
			entry.second += randBoundsInclusive(rng, -10, 10);

			// Finally stay in bounds
			entry.second = clamp(entry.second, -100.0f, 100.0f);
		}
	}
	// Setup buildings
	for (auto &pair : this->cities)
	{
		for (auto &b : pair.second->buildings)
		{
			b.second->ticksDetectionAttemptAccumulated =
			    randBoundsExclusive(rng, (unsigned)0, TICKS_PER_DETECTION_ATTEMPT[difficulty]);
		}
	}
	// Setup scenery
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

			city->scenery.push_back(s);
		}
	}

	// Add aliens into random building
	int counter = 0;
	int giveUpCount = 100;
	auto buildingIt = this->cities["CITYMAP_HUMAN"]->buildings.begin();
	do
	{
		int buildID =
		    randBoundsExclusive(rng, 0, (int)this->cities["CITYMAP_HUMAN"]->buildings.size());
		buildingIt = this->cities["CITYMAP_HUMAN"]->buildings.begin();
		for (int i = 0; i < buildID; i++)
		{
			buildingIt++;
		}
		counter++;
	} while (buildingIt->second->owner->current_relations[player] < 0 || counter >= giveUpCount);

	for (auto &l : initial_aliens.at(difficulty))
	{
		buildingIt->second->current_crew[l.first] =
		    randBoundsExclusive(rng, l.second.x, l.second.y);
	}

	gameTime = GameTime::midday();

	newGame = true;
	firstDetection = true;
}

// Fills out initial player property
void GameState::fillPlayerStartingProperty()
{
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
	base->name = "Base " + Strings::fromInteger(this->player_bases.size() + 1);
	this->player_bases[Base::getPrefix() + Strings::fromInteger(this->player_bases.size() + 1)] =
	    base;
	bld->owner = this->getPlayer();
	bld->base = {this, base};
	this->current_base = {this, base};

	// Give the player one of each equipable vehicle
	for (auto &it : this->vehicle_types)
	{
		auto &type = it.second;
		if (!type->equipment_screen)
			continue;
		for (auto i = 0; i < 2; i++)
		{
			if (i > 0 && type->type == VehicleType::Type::Ground)
			{
				break;
			}

			auto v =
			    current_city->placeVehicle(*this, {this, type}, this->getPlayer(), {this, bld});
			v->homeBuilding = v->currentBuilding;
		}
	}
	// Give that base some vehicle inventory
	for (auto &pair : this->vehicle_equipment)
	{
		auto &equipmentID = pair.first;
		base->inventoryVehicleEquipment[equipmentID] = 10;
	}
	// Give base starting agent equipment
	for (auto &pair : this->initial_base_agent_equipment)
	{
		auto &equipmentID = pair.first;
		base->inventoryAgentEquipment[equipmentID] = pair.second;
	}
	// Give starting agents and their equipment
	for (auto &agentTypePair : this->initial_agents)
	{
		auto type = agentTypePair.first;
		auto count = agentTypePair.second;
		auto it = initial_agent_equipment.begin();
		while (count > 0)
		{
			auto agent = this->agent_generator.createAgent(*this, this->getPlayer(), type);
			agent->homeBuilding = base->building;
			agent->city = agent->homeBuilding->city;
			agent->enterBuilding(*this, agent->homeBuilding);
			count--;
			if (type == AgentType::Role::Soldier && it != initial_agent_equipment.end())
			{
				for (auto &t : *it)
				{
					if (t->type == AEquipmentType::Type::Armor)
					{
						EquipmentSlotType slotType = EquipmentSlotType::General;
						switch (t->body_part)
						{
							case BodyPart::Body:
								slotType = EquipmentSlotType::ArmorBody;
								break;
							case BodyPart::Legs:
								slotType = EquipmentSlotType::ArmorLegs;
								break;
							case BodyPart::Helmet:
								slotType = EquipmentSlotType::ArmorHelmet;
								break;
							case BodyPart::LeftArm:
								slotType = EquipmentSlotType::ArmorLeftHand;
								break;
							case BodyPart::RightArm:
								slotType = EquipmentSlotType::ArmorRightHand;
								break;
						}
						agent->addEquipmentByType(*this, {this, t->id}, slotType, false);
					}
					else if (t->type == AEquipmentType::Type::Ammo ||
					         t->type == AEquipmentType::Type::MediKit ||
					         t->type == AEquipmentType::Type::Grenade)
					{
						agent->addEquipmentByType(*this, {this, t->id}, EquipmentSlotType::General,
						                          false);
					}
					else
					{
						agent->addEquipmentByType(*this, {this, t->id}, false);
					}
				}
				it++;
			}
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
		    v.second->type->aggressiveness > 0 &&
		    v.second->owner->isRelatedTo(this->getPlayer()) == Organisation::Relation::Hostile &&
		    !v.second->isCrashed())
		{
			return false;
		}
	}
	return true;
}

void GameState::update(unsigned int ticks)
{
	if (this->current_battle)
	{
		// Save time to roll back to
		if (gameTimeBeforeBattle.getTicks() == 0)
			gameTimeBeforeBattle = GameTime(gameTime.getTicks());

		Trace::start("GameState::update::battles");
		this->current_battle->update(*this, ticks);
		Trace::end("GameState::update::battles");
		gameTime.addTicks(ticks);
	}
	else
	{
		// Roll back to time before battle
		if (gameTimeBeforeBattle.getTicks() != 0)
		{
			gameTime = GameTime(gameTimeBeforeBattle.getTicks());
			gameTimeBeforeBattle = GameTime(0);
		}

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
		Trace::start("GameState::update::agents");
		for (auto &a : this->agents)
		{
			a.second->update(*this, ticks);
		}
		Trace::end("GameState::update::agents");
		gameTime.addTicks(ticks);
		if (gameTime.fiveMinutesPassed())
		{
			this->updateEndOfFiveMinutes();
		}
		if (gameTime.hourPassed())
		{
			this->updateEndOfHour();
		}
		if (gameTime.dayPassed())
		{
			this->updateEndOfDay();
		}
		if (gameTime.weekPassed())
		{
			this->updateEndOfWeek();
		}
		gameTime.clearFlags();
	}
}

void GameState::updateEndOfFiveMinutes()
{
	// TakeOver calculation stops when org is taken over
	Trace::start("GameState::updateEndOfFiveMinutes::organisations");
	for (auto &o : this->organisations)
	{
		if (o.second->takenOver)
		{
			continue;
		}
		o.second->updateTakeOver(*this, TICKS_PER_MINUTE * 5);
		if (o.second->takenOver)
		{
			break;
		}
	}
	Trace::end("GameState::updateEndOfFiveMinutes::organisations");

	// Detection calculation stops when detection happens
	Trace::start("GameState::updateEndOfFiveMinutes::buildings");
	for (auto &b : current_city->buildings)
	{
		bool detected = b.second->ticksDetectionTimeOut > 0;
		b.second->updateDetection(*this, TICKS_PER_MINUTE * 5);
		if (b.second->ticksDetectionTimeOut > 0 && !detected)
		{
			break;
		}
	}
	Trace::end("GameState::updateEndOfFiveMinutes::buildings");
}

void GameState::updateEndOfHour()
{
	Trace::start("GameState::updateEndOfHour::labs");
	for (auto &lab : this->research.labs)
	{
		Lab::update(TICKS_PER_HOUR, {this, lab.second}, shared_from_this());
	}
	Trace::end("GameState::updateEndOfHour::labs");
	Trace::start("GameState::updateEndOfHour::cities");
	for (auto &c : this->cities)
	{
		c.second->hourlyLoop(*this);
	}
	Trace::end("GameState::updateEndOfHour::cities");
	Trace::start("GameState::updateEndOfHour::organisations");
	for (auto &o : this->organisations)
	{
		o.second->updateInfiltration(*this);
	}
	Trace::end("GameState::updateEndOfHour::organisations");
}

void GameState::updateEndOfDay()
{
	for (auto &b : this->player_bases)
	{
		for (auto &f : b.second->facilities)
		{
			if (f->buildTime > 0)
			{
				f->buildTime--;
				if (f->buildTime == 0)
				{
					fw().pushEvent(
					    new GameFacilityEvent(GameEventType::FacilityCompleted, b.second, f));
				}
			}
		}
	}

	Trace::start("GameState::updateEndOfDay::cities");
	for (auto &c : this->cities)
	{
		c.second->dailyLoop(*this);
	}
	Trace::end("GameState::updateEndOfDay::cities");

	// SPAWN ALIENS
	for (int i = 0; i < 5; i++)
	{
		StateRef<City> city = {this, "CITYMAP_HUMAN"};

		auto portal = city->portals.begin();
		std::uniform_int_distribution<int> portal_rng(0, city->portals.size() - 1);
		std::advance(portal, portal_rng(this->rng));

		auto bld_iter = city->buildings.begin();
		std::uniform_int_distribution<int> bld_rng(0, city->buildings.size() - 1);
		std::advance(bld_iter, bld_rng(this->rng));
		StateRef<Building> bld = {this, (*bld_iter).second};

		auto vehicleType = this->vehicle_types.find("VEHICLETYPE_ALIEN_ASSAULT_SHIP");
		if (vehicleType != this->vehicle_types.end())
		{
			auto &type = (*vehicleType).second;

			auto v = city->placeVehicle(*this, {this, (*vehicleType).first}, type->manufacturer,
			                            (*portal)->getPosition());
			v->missions.emplace_back(VehicleMission::infiltrateOrSubvertBuilding(*this, *v, bld));
			v->missions.front()->start(*this, *v);

			fw().pushEvent(new GameVehicleEvent(GameEventType::UfoSpotted, {this, v}));
		}
	}
}

void GameState::updateEndOfWeek()
{
	int week = this->gameTime.getWeek();
	auto growth = this->ufo_growth_lists.find(format("%s%d", UFOGrowth::getPrefix(), week));
	if (growth == this->ufo_growth_lists.end())
	{
		growth = this->ufo_growth_lists.find(format("%s%s", UFOGrowth::getPrefix(), "DEFAULT"));
	}

	if (growth != this->ufo_growth_lists.end())
	{
		StateRef<City> city = {this, "CITYMAP_ALIEN"};
		StateRef<Organisation> alienOrg = {this, "ORG_ALIEN"};
		std::uniform_int_distribution<int> xyPos(20, 120);

		for (auto &vehicleEntry : growth->second->vehicleTypeList)
		{
			auto vehicleType = this->vehicle_types.find(vehicleEntry.first);
			if (vehicleType != this->vehicle_types.end())
			{
				for (int i = 0; i < vehicleEntry.second; i++)
				{
					auto &type = (*vehicleType).second;

					auto v = city->placeVehicle(*this, {this, (*vehicleType).first}, alienOrg,
					                            {xyPos(rng), xyPos(rng), city->map->size.z - 1});
					v->missions.emplace_front(VehicleMission::patrol(*this, *v));
				}
			}
		}
	}
}

void GameState::updateTurbo()
{
	if (!this->canTurbo())
	{
		LogError("Called when canTurbo() is false");
	}
	unsigned ticksToUpdate = TURBO_TICKS;
	// Turbo always re-aligns to TURBO_TICKS (5 minutes)
	unsigned int align = this->gameTime.getTicks() % TURBO_TICKS;
	if (align != 0)
	{
		ticksToUpdate -= align;
	}
	this->update(ticksToUpdate);
}

void GameState::logEvent(GameEvent *ev)
{
	if (messages.size() == MAX_MESSAGES)
	{
		messages.pop_front();
	}
	Vec3<int> location = EventMessage::NO_LOCATION;
	if (GameVehicleEvent *gve = dynamic_cast<GameVehicleEvent *>(ev))
	{
		location = gve->vehicle->position;
	}
	else if (GameBuildingEvent *gve = dynamic_cast<GameBuildingEvent *>(ev))
	{
		location = {gve->building->bounds.p0.x, gve->building->bounds.p0.y, 0};
	}
	else if (GameAgentEvent *gae = dynamic_cast<GameAgentEvent *>(ev))
	{
		if (gae->agent->unit)
		{
			location = gae->agent->unit->position;
		}
		// FIXME: Introduce agent position when in city
	}
	else if (GameBaseEvent *gbe = dynamic_cast<GameBaseEvent *>(ev))
	{
		location =
		    Vec3<int>(gbe->base->building->bounds.p0.x + gbe->base->building->bounds.p1.x,
		              gbe->base->building->bounds.p0.y + gbe->base->building->bounds.p1.y, 0) /
		    2;
	}
	// TODO: Other event types
	messages.emplace_back(EventMessage{gameTime, ev->message(), location});
}

uint64_t getNextObjectID(GameState &state, const UString &objectPrefix)
{
	std::lock_guard<std::mutex> l(state.objectIdCountLock);
	return state.objectIdCount[objectPrefix]++;
}

}; // namespace OpenApoc

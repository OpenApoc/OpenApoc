#include "game/state/gamestate.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/modinfo.h"
#include "framework/options.h"
#include "framework/sound.h"
#include "framework/trace.h"
#include "game/state/battle/battle.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/facility.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/gameevent.h"
#include "game/state/gametime.h"
#include "game/state/message.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlecommonsamplelist.h"
#include "game/state/rules/battle/battlemap.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/rules/battle/battleunitanimationpack.h"
#include "game/state/rules/battle/battleunitimagepack.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/rules/city/baselayout.h"
#include "game/state/rules/city/citycommonimagelist.h"
#include "game/state/rules/city/citycommonsamplelist.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/rules/city/ufogrowth.h"
#include "game/state/rules/city/ufoincursion.h"
#include "game/state/rules/city/ufomissionpreference.h"
#include "game/state/rules/city/ufopaedia.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/rules/doodadtype.h"
#include "game/state/shared/aequipment.h"
#include "game/state/shared/doodad.h"
#include "game/state/shared/organisation.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_vehicle.h"
#include "library/strings_format.h"
#include <random>

namespace OpenApoc
{

GameState::GameState() : player(this) { luaGameState.init(*this); }

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
		vehicle->removeFromMap(*this);
		// Detach some back-pointers otherwise we get circular sp<> dependencies and leak
		// FIXME: This is not a 'good' way of doing this, maybe add a destroyVehicle() function? Or
		// make StateRefWeak<> or something?
		//
		vehicle->city.clear();
		vehicle->homeBuilding.clear();
		vehicle->currentBuilding.clear();
		vehicle->missions.clear();
		vehicle->equipment.clear();
		vehicle->mover = nullptr;
	}
	for (auto &b : this->player_bases)
	{
		for (auto &f : b.second->facilities)
		{
			if (f->lab)
			{
				f->lab->assigned_agents.clear();
				f->lab->current_project.clear();
				f->lab.clear();
			}
		}
		b.second->building.clear();
	}
	for (auto &org : this->organisations)
	{
		org.second->current_relations.clear();
	}
	for (auto &city : this->cities)
	{
		for (auto &building : city.second->buildings)
		{
			auto &bld = building.second;
			bld->city.clear();
			bld->function.clear();
			bld->owner.clear();
			bld->base_layout.clear();
			bld->base.clear();
			bld->battle_map.clear();
			bld->preset_crew.clear();
			bld->current_crew.clear();
			bld->currentVehicles.clear();
			bld->currentAgents.clear();
			bld->researchUnlock.clear();
			bld->accessTopic.clear();
		}
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
const StateRef<Organisation> &GameState::getGovernment() const { return this->government; }
StateRef<Organisation> GameState::getGovernment() { return this->government; }
const StateRef<Organisation> &GameState::getCivilian() const { return this->civilian; }
StateRef<Organisation> GameState::getCivilian() { return this->civilian; }

void GameState::initState()
{
	if (current_battle)
	{
		current_battle->initBattle(*this);
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
					if (s->isAlive() && !s->type->commonProperty)
					{
						s->building->buildingParts.insert(s->initialPosition);
					}
					break;
				}
			}
		}
	}
	for (auto &c : this->cities)
	{
		auto &city = c.second;
		city->initMap(*this);
		if (newGame)
		{
			// if (c.first == "CITYMAP_HUMAN")
			{
				city->fillRoadSegmentMap(*this);
				city->initialSceneryLinkUp();
			}
		}
		// Add vehicles to map
		for (auto &v : this->vehicles)
		{
			auto vehicle = v.second;
			if (vehicle->city == city && !vehicle->currentBuilding && !vehicle->betweenDimensions)
			{

				city->map->addObjectToMap(*this, vehicle);
			}
			vehicle->strategyImages = city_common_image_list->strategyImages;
			vehicle->setupMover();
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
	// Fill links for weapon's ammo
	for (auto &t : this->agent_equipment)
	{
		for (auto &w : t.second->weapon_types)
		{
			w->ammo_types.emplace(this, t.first);
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
	// Run necessary methods for different types
	research.updateTopicList();
	// Apply mods (Stub until we actually have mods)
	applyMods();
	// Validate
	validate();

	skipTurboCalculations = config().getBool("OpenApoc.NewFeature.SkipTurboMovement");
}

void GameState::applyMods() { luaGameState.callHook("applyMods", 0, 0); }

void GameState::setCurrentCity(StateRef<City> city)
{
	current_city = city;
	for (auto &u : current_city->researchUnlock)
	{
		u->forceComplete();
	}
}

void GameState::validate()
{
	LogWarning("Validating GameState");
	validateResearch();
	validateScenery();
	LogWarning("Validated GameState");
}

void GameState::validateResearch()
{
	for (auto &t : research.topics)
	{
		if (t.second->type == ResearchTopic::Type::Engineering)
		{
			if (t.second->itemId.length() == 0)
			{
				LogError("EMPTY REFERENCE resulting item for %s", t.first);
			}
			else
			{
				bool fail = false;
				switch (t.second->item_type)
				{
					case ResearchTopic::ItemType::VehicleEquipment:
						if (vehicle_equipment.find(t.second->itemId) == vehicle_equipment.end())
						{
							fail = true;
						}
						break;
					case ResearchTopic::ItemType::VehicleEquipmentAmmo:
						if (vehicle_ammo.find(t.second->itemId) == vehicle_ammo.end())
						{
							fail = true;
						}
						break;
					case ResearchTopic::ItemType::AgentEquipment:
						if (agent_equipment.find(t.second->itemId) == agent_equipment.end())
						{
							fail = true;
						}
						break;
					case ResearchTopic::ItemType::Craft:
						if (vehicle_types.find(t.second->itemId) == vehicle_types.end())
						{
							fail = true;
						}
						break;
				}
				if (fail)
				{
					LogError("%s DOES NOT EXIST: referenced as manufactured by %s",
					         t.second->itemId, t.first);
				}
			}
		}
		for (auto &rd : t.second->dependencies.research)
		{
			for (auto &topic : rd.topics)
			{
				if (topic.id.length() == 0)
				{
					LogError("EMPTY REFERENCE required topic for %s", t.first);
				}
				else if (research.topics.find(topic.id) == research.topics.end())
				{
					LogError("%s DOES NOT EXIST: referenced as required topic for %s", topic.id,
					         t.first);
				}
			}
		}
		for (auto &entry : t.second->dependencies.items.agentItemsRequired)
		{
			if (entry.first.id.length() == 0)
			{
				LogError("EMPTY REFERENCE required item for %s", t.first);
			}
			else if (agent_equipment.find(entry.first.id) == agent_equipment.end())
			{
				LogError("%s DOES NOT EXIST: referenced as required item for %s", entry.first.id,
				         t.first);
			}
		}
		for (auto &entry : t.second->dependencies.items.agentItemsConsumed)
		{
			if (entry.first.id.length() == 0)
			{
				LogError("EMPTY REFERENCE consumed item for %s", t.first);
			}
			else if (agent_equipment.find(entry.first.id) == agent_equipment.end())
			{
				LogError("%s DOES NOT EXIST: referenced as consumed item for %s", entry.first.id,
				         t.first);
			}
		}
		for (auto &entry : t.second->dependencies.items.vehicleItemsRequired)
		{
			if (entry.first.id.length() == 0)
			{
				LogError("EMPTY REFERENCE required item for %s", t.first);
			}
			else if (vehicle_equipment.find(entry.first.id) == vehicle_equipment.end())
			{
				LogError("%s DOES NOT EXIST: referenced as required item for %s", entry.first.id,
				         t.first);
			}
		}
		for (auto &entry : t.second->dependencies.items.vehicleItemsConsumed)
		{
			if (entry.first.id.length() == 0)
			{
				LogError("EMPTY REFERENCE consumed item for %s", t.first);
			}
			else if (vehicle_equipment.find(entry.first.id) == vehicle_equipment.end())
			{
				LogError("%s DOES NOT EXIST: referenced as consumed item for %s", entry.first.id,
				         t.first);
			}
		}
		for (auto &entry : t.second->dependencies.items.agentItemsConsumed)
		{
			if (t.second->dependencies.items.agentItemsRequired.find(entry.first) ==
			    t.second->dependencies.items.agentItemsRequired.end())
			{
				LogError("Consumed agent item %s not in required list for topic %s", entry.first.id,
				         t.first);
			}
			else if (t.second->dependencies.items.agentItemsRequired.at(entry.first) < entry.second)
			{
				LogError("Consumed agent items %s has bigger count than required for topic %s",
				         entry.first.id, t.first);
			}
		}
		for (auto &entry : t.second->dependencies.items.vehicleItemsConsumed)
		{
			if (t.second->dependencies.items.vehicleItemsRequired.find(entry.first) ==
			    t.second->dependencies.items.vehicleItemsRequired.end())
			{
				LogError("Consumed vehicle item %s not in required list for topic %s",
				         entry.first.id, t.first);
			}
			else if (t.second->dependencies.items.vehicleItemsRequired.at(entry.first) <
			         entry.second)
			{
				LogError("Consumed vehicle item %s has bigger count than required for topic %s",
				         entry.first.id, t.first);
			}
		}
	}
}

void GameState::validateScenery()
{
	for (auto &c : cities)
	{
		for (auto &sc : c.second->tile_types)
		{
			auto thisSc = StateRef<SceneryTileType>{this, sc.first};
			std::set<StateRef<SceneryTileType>> seenTypes;
			while (thisSc->damagedTile)
			{
				seenTypes.insert(thisSc);
				bool roadAlive = false;
				bool roadDead = false;
				bool newRoad = false;
				if (thisSc->tile_type != SceneryTileType::TileType::Road &&
				    thisSc->damagedTile->tile_type == SceneryTileType::TileType::Road)
				{
					newRoad = true;
				}
				else
				{
					for (int i = 0; i < 4; i++)
					{
						if (thisSc->connection[i] &&
						    thisSc->connection[i] == thisSc->damagedTile->connection[i])
						{
							roadAlive = true;
						}
						if (thisSc->connection[i] &&
						    thisSc->connection[i] != thisSc->damagedTile->connection[i])
						{
							roadDead = true;
						}
						if (!thisSc->connection[i] &&
						    thisSc->connection[i] != thisSc->damagedTile->connection[i])
						{
							newRoad = true;
						}
					}
				}
				if (newRoad || (roadAlive && roadDead))
				{
					LogError("ROAD MUTATION: In %s when damaged from %s to %s roads go [%d%d%d%d] "
					         "to [%d%d%d%d]",
					         sc.first, thisSc.id, thisSc->damagedTile.id,
					         (int)thisSc->connection[0], (int)thisSc->connection[1],
					         (int)thisSc->connection[2], (int)thisSc->connection[3],
					         (int)thisSc->damagedTile->connection[0],
					         (int)thisSc->damagedTile->connection[1],
					         (int)thisSc->damagedTile->connection[2],
					         (int)thisSc->damagedTile->connection[3]);
				}
				if (seenTypes.find(thisSc->damagedTile) != seenTypes.end())
				{
					break;
				}
				thisSc = thisSc->damagedTile;
			}
		}
	}
}

void GameState::validateAgentEquipment()
{
	for (auto &ae : agent_equipment)
	{
		if (ae.second->type == AEquipmentType::Type::Ammo)
		{
			if (ae.second->max_ammo == 0)
			{
				LogError(
				    "%s ZERO MAX AMMO: equipment of type ammo must always have non-zero max ammo",
				    ae.first);
			}
			if (ae.second->max_ammo != 1 && ae.second->bioStorage)
			{
				LogError("%s BIO AMMO CLIP: equipment stored in alien containment must never have "
				         "max ammo other than 1",
				         ae.first);
			}
		}
	}
}

void GameState::fillOrgStartingProperty()
{
	auto buildingIt = this->cities["CITYMAP_HUMAN"]->buildings.begin();

	for (auto &o : this->organisations)
	{
		o.second->updateVehicleAgentPark(*this);
		o.second->updateHirableAgents(*this);
		for (auto &m : o.second->missions[{this, "CITYMAP_HUMAN"}])
		{
			m.next +=
			    gameTime.getTicks() +
			    randBoundsInclusive(rng, (uint64_t)0,
			                        m.pattern.maxIntervalRepeat - m.pattern.minIntervalRepeat) -
			    m.pattern.minIntervalRepeat / 2;
		}
	}

	luaGameState.callHook("newGamePostInit", 0, 0);
}

void GameState::startGame()
{
	luaGameState.callHook("newGame", 0, 0);

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
				// Relationship vs player is adjusted by flat 0/5/0/-5/-10
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

			// Set player reverse relationships
			if (entry.first == getPlayer())
			{
				getPlayer()->current_relations[{this, pair.first}] = entry.second;
			}
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

	updateEndOfWeek();

	newGame = true;
	firstDetection = true;
	nextInvasion = gameTime.getTicks() + 10 * TICKS_PER_HOUR +
	               randBoundsInclusive(rng, 0, (int)(2 * TICKS_PER_HOUR));
}

// Fills out initial player property
void GameState::fillPlayerStartingProperty()
{
	// Create the initial starting base
	// Randomly shuffle buildings until we find one with a base layout
	sp<City> humanCity = this->cities["CITYMAP_HUMAN"];
	setCurrentCity({this, humanCity});

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
	/*for (auto &it : this->vehicle_types)
	{
	    auto &type = it.second;
	    if (!type->equipment_screen)
	        continue;
	    auto v = current_city->placeVehicle(*this, {this, type}, this->getPlayer(), {this, bld});
	    v->homeBuilding = v->currentBuilding;
	}*/
	for (auto &pair : this->initial_vehicles)
	{
		auto v = current_city->createVehicle(*this, pair.first, this->getPlayer(), {this, bld});
		v->homeBuilding = v->currentBuilding;
		for (auto &eq : pair.second)
		{
			auto device = v->addEquipment(*this, eq);
			device->ammo = eq->max_ammo;
		}
	}
	// Give the player initial vehicle equipment
	for (auto &pair : this->initial_vehicle_equipment)
	{
		base->inventoryVehicleEquipment[pair.first.id] = pair.second;
	}
	// Give the player initial vehicle ammo
	for (auto &pair : this->initial_vehicle_ammo)
	{
		base->inventoryVehicleAmmo[pair.first.id] = pair.second;
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
			if (agent->type->canTrain)
			{
				agent->trainingAssignment = agent->initial_stats.psi_energy > 30
				                                ? TrainingAssignment::Psi
				                                : TrainingAssignment::Physical;
			}
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

	// Start player centered on base
	auto bldBounds = bld->bounds;

	Vec2<int> buildingCenter = (bldBounds.p0 + bldBounds.p1) / 2;
	bld->city->cityViewScreenCenter = {buildingCenter.x, buildingCenter.y, 1.0f};
}

void GameState::invasion()
{
	auto invadedCity = StateRef<City>{this, "CITYMAP_HUMAN"};
	if (current_city != invadedCity)
	{
		nextInvasion += TICKS_PER_MINUTE;
		return;
	}
	nextInvasion = gameTime.getTicks() + 24 * TICKS_PER_HOUR +
	               randBoundsInclusive(rng, 0, (int)(72 * TICKS_PER_HOUR));

	invadedCity->generatePortals(*this);

	auto invadingCity = StateRef<City>{this, "CITYMAP_ALIEN"};
	auto invadingOrg = StateRef<Organisation>{this, "ORG_ALIEN"};

	// Set a list of possible participants
	std::map<UString, int> vehicleLimits;
	std::map<UString, std::list<sp<Vehicle>>> invaders;
	for (auto &v : vehicles)
	{
		if (v.second->owner == invadingOrg && v.second->city == invadingCity)
		{
			vehicleLimits[v.second->type.id]++;
			invaders[v.second->type.id].push_back(v.second);
		}
	}
	// Select a random mission type
	int week = this->gameTime.getWeek();
	auto preference =
	    this->ufo_mission_preference.find(format("%s%d", UFOMissionPreference::getPrefix(), week));
	if (preference == this->ufo_mission_preference.end())
	{
		preference = this->ufo_mission_preference.find(
		    format("%s%s", UFOMissionPreference::getPrefix(), "DEFAULT"));
	}
	auto missionType = pickRandom(rng, preference->second->missionList);
	// Compile list of missions rated by priority
	std::map<int, sp<UFOIncursion>> incursions;
	for (auto &e : ufo_incursions)
	{
		if (e.second->primaryMission == missionType)
		{
			incursions[e.second->priority] = e.second;
		}
	}
	// Find first incursion by type that fits
	sp<UFOIncursion> currentIncursion;
	for (auto &inc : incursions)
	{
		auto limits = vehicleLimits;
		for (auto &v : inc.second->primaryList)
		{
			limits[v.first] -= v.second;
		}
		for (auto &v : inc.second->attackList)
		{
			limits[v.first] -= v.second;
		}
		for (auto &v : inc.second->escortList)
		{
			limits[v.first] -= v.second;
		}
		bool enoughVehicles = true;
		for (auto &v : limits)
		{
			if (v.second < 0)
			{
				enoughVehicles = false;
				break;
			}
		}
		if (enoughVehicles)
		{
			currentIncursion = inc.second;
			break;
		}
	}
	if (!currentIncursion)
	{
		return;
	}

	std::set<StateRef<Vehicle>> escorted;
	for (auto &v : currentIncursion->primaryList)
	{
		for (int i = 0; i < v.second; i++)
		{
			auto invader = invaders[v.first].front();
			invaders[v.first].pop_front();

			invader->enterDimensionGate(*this);
			invader->equipDefaultEquipment(*this);
			invader->city = invadedCity;
			invader->setMission(*this, VehicleMission::arriveFromDimensionGate(*this, *invader));
			switch (missionType)
			{
				case UFOIncursion::PrimaryMission::Attack:
					invader->addMission(*this, VehicleMission::attackBuilding(*this, *invader),
					                    true);
					break;
				case UFOIncursion::PrimaryMission::Infiltration:
					invader->addMission(
					    *this, VehicleMission::infiltrateOrSubvertBuilding(*this, *invader, false),
					    true);
					break;
				case UFOIncursion::PrimaryMission::Subversion:
					invader->addMission(
					    *this, VehicleMission::infiltrateOrSubvertBuilding(*this, *invader, true),
					    true);
					break;
				case UFOIncursion::PrimaryMission::Overspawn:
					LogWarning("Implement Overspawn, just attacking for now");
					// FIXME: Implement Overspawn, just attacking for now
					invader->addMission(*this, VehicleMission::attackBuilding(*this, *invader),
					                    true);
					break;
			}
			escorted.emplace(this, invader);
		}
	}
	for (auto &v : currentIncursion->escortList)
	{
		for (int i = 0; i < v.second; i++)
		{
			auto invader = invaders[v.first].front();
			invaders[v.first].pop_front();

			invader->enterDimensionGate(*this);
			invader->city = invadedCity;
			invader->setMission(*this, VehicleMission::arriveFromDimensionGate(*this, *invader));
			// This creates a copy of escorted list in randomised order
			auto escortedCopy = escorted;
			std::list<StateRef<Vehicle>> escortedRandomized;
			while (!escortedCopy.empty())
			{
				auto item = pickRandom(rng, escortedCopy);
				escortedCopy.erase(item);
				escortedRandomized.push_back(item);
			}
			invader->addMission(
			    *this, VehicleMission::followVehicle(*this, *invader, escortedRandomized), true);
		}
	}
	for (auto &v : currentIncursion->attackList)
	{
		for (int i = 0; i < v.second; i++)
		{
			auto invader = invaders[v.first].front();
			invaders[v.first].pop_front();

			invader->enterDimensionGate(*this);
			invader->city = invadedCity;
			invader->setMission(*this, VehicleMission::arriveFromDimensionGate(*this, *invader));
			invader->addMission(*this, VehicleMission::attackBuilding(*this, *invader), true);
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
		if (!v.second->isDead() && v.second->city == this->current_city &&
		    v.second->tileObject != nullptr)
		{
			if (v.second->type->aggressiveness > 0 &&
			    v.second->owner->isRelatedTo(this->getPlayer()) ==
			        Organisation::Relation::Hostile &&
			    !v.second->crashed)
			{
				return false;
			}
			for (auto &m : v.second->missions)
			{
				if (m->type == VehicleMission::MissionType::AttackBuilding ||
				    m->type == VehicleMission::MissionType::AttackVehicle)
				{
					return false;
				}
			}
		}
	}
	return true;
}

/**
 * Immediately remove all dead objects.
 */
void OpenApoc::GameState::cleanUpDeathNote()
{
	// Any additional death notes should processed here.
	if (!vehiclesDeathNote.empty())
	{
		for (auto &name : this->vehiclesDeathNote)
		{
			vehicles.erase(name);
		}
		vehiclesDeathNote.clear();
	}
}

void GameState::update(unsigned int ticks)
{
	if (this->current_battle)
	{
		Trace::start("GameState::update::battles");
		this->current_battle->update(*this, ticks);
		Trace::end("GameState::update::battles");
		gameTime.addTicks(ticks);
	}
	else
	{
		// Roll back to time before battle and stuff
		if (gameTimeBeforeBattle.getTicks() != 0)
		{
			updateAfterBattle();
		}

		Trace::start("GameState::update::cities");
		current_city->update(*this, ticks);
		Trace::end("GameState::update::cities");

		Trace::start("GameState::update::organisations");
		for (auto &o : this->organisations)
		{
			o.second->updateMissions(*this);
		}
		Trace::end("GameState::update::organisations");

		Trace::start("GameState::update::vehicles");
		for (auto &v : this->vehicles)
		{
			if (v.second->city == current_city)
			{
				v.second->update(*this, ticks);
			}
		}
		cleanUpDeathNote();
		Trace::end("GameState::update::vehicles");

		Trace::start("GameState::update::agents");
		for (auto &a : this->agents)
		{
			if (a.second->city == current_city)
			{
				a.second->update(*this, ticks);
			}
		}
		Trace::end("GameState::update::agents");

		gameTime.addTicks(ticks);

		if (gameTime.getTicks() > nextInvasion)
		{
			invasion();
		}
		if (gameTime.secondPassed())
		{
			this->updateEndOfSecond();
		}
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

void GameState::updateEndOfSecond()
{
	Trace::start("GameState::updateEachSecond::buildings");
	for (auto &b : current_city->buildings)
	{
		b.second->updateCargo(*this);
		if (!b.second->base || b.second->owner != getPlayer())
		{
			continue;
		}
		auto base = b.second->base;
		for (auto v : b.second->currentVehicles)
		{
			for (auto &e : v->equipment)
			{
				// We only can reload VehicleWeapon and VehicleEngine(?)
				if (e->type->type != EquipmentSlotType::VehicleWeapon &&
				    e->type->type != EquipmentSlotType::VehicleEngine) //  e->type->max_ammo == 0
				{
					continue;
				}
				// Only show events for vehicles owned by current player
				if (v->owner->name == getPlayer()->name)
				{
					if (e->reload(*this, base))
					{
						switch (e->type->type)
						{
							case EquipmentSlotType::VehicleEngine:
								fw().pushEvent(
								    new GameVehicleEvent(GameEventType::VehicleRefuelled, v));
								break;
							case EquipmentSlotType::VehicleWeapon:
								fw().pushEvent(
								    new GameVehicleEvent(GameEventType::VehicleRearmed, v));
								break;
							default:
								LogInfo("Implement the remaining messages for vehicle rearmed / "
								        "reloaded / refueled / whatever");
								break;
						}
					}
				}
			}
		}
	}
	Trace::end("GameState::updateEachSecond::buildings");
	Trace::start("GameState::updateEachSecond::vehicles");
	for (auto &v : vehicles)
	{
		if (v.second->city == current_city)
		{
			v.second->updateEachSecond(*this);
		}
	}
	Trace::end("GameState::updateEachSecond::vehicles");
	Trace::start("GameState::updateEachSecond::agents");
	for (auto &a : this->agents)
	{
		if (a.second->city == current_city)
		{
			a.second->updateEachSecond(*this);
		}
	}
	Trace::end("GameState::updateEachSecond::agents");
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
	for (auto &b : current_city->buildings)
	{
		b.second->updateCargo(*this);
	}
	Trace::end("GameState::updateEndOfFiveMinutes::buildings");
}

void GameState::updateEndOfHour()
{
	Trace::start("GameState::updateEndOfHour::agents");
	for (auto &a : this->agents)
	{
		a.second->updateHourly(*this);
	}
	Trace::end("GameState::updateEndOfHour::agents");
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
	Trace::start("GameState::updateEndOfDay::bases");
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
	Trace::end("GameState::updateEndOfDay::bases");
	Trace::start("GameState::updateEndOfDay::organisations");
	for (auto &o : this->organisations)
	{
		o.second->updateVehicleAgentPark(*this);
		o.second->updateHirableAgents(*this);
	}
	Trace::end("GameState::updateEndOfDay::organisations");
	Trace::start("GameState::updateEndOfDay::agents");
	for (auto &a : this->agents)
	{
		a.second->updateDaily(*this);
	}
	Trace::end("GameState::updateEndOfDay::agents");
	Trace::start("GameState::updateEndOfDay::cities");
	for (auto &c : this->cities)
	{
		c.second->dailyLoop(*this);
	}
	Trace::end("GameState::updateEndOfDay::cities");
}

void GameState::updateEndOfWeek() { luaGameState.callHook("updateEndOfWeek", 0, 0); }

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
	this->updateAfterTurbo();
}

void GameState::updateAfterTurbo()
{
	Trace::start("GameState::updateAfterTurbo::vehicles");
	for (auto &v : this->vehicles)
	{
		if (v.second->city != current_city)
		{
			continue;
		}
		if (v.second->type->aggressiveness > 0)
		{
			continue;
		}
		v.second->update(*this, randBoundsExclusive(rng, (unsigned)0, 20 * TICKS_PER_SECOND));
	}
	Trace::end("GameState::updateAfterTurbo::vehicles");
}

void GameState::updateBeforeBattle()
{
	// Save time to roll back to
	gameTimeBeforeBattle = GameTime(gameTime.getTicks());
	// Some useless event just to know if something was reported
	eventFromBattle = GameEventType::None;
}

void GameState::updateAfterBattle()
{
	gameTime = GameTime(gameTimeBeforeBattle.getTicks());
	gameTimeBeforeBattle = GameTime(0);

	switch (eventFromBattle)
	{
		case GameEventType::MissionCompletedBuildingAlien:
		{
			fw().pushEvent(new GameEvent(eventFromBattle));
			break;
		}
		case GameEventType::MissionCompletedBuildingNormal:
		{
			fw().pushEvent(new GameBuildingEvent(eventFromBattle, {this, missionLocationBattle}));
			break;
		}
		case GameEventType::MissionCompletedBase:
		{
			fw().pushEvent(new GameBaseEvent(
			    eventFromBattle, StateRef<Building>(this, missionLocationBattle)->base));
			break;
		}
		case GameEventType::BaseDestroyed:
		{
			auto building = StateRef<Building>{this, missionLocationBattle};
			fw().pushEvent(new GameSomethingDiedEvent(eventFromBattle, eventFromBattleText,
			                                          "bySomeone", building->crewQuarters));
			break;
		}
		case GameEventType::MissionCompletedBuildingRaid:
		{
			fw().pushEvent(new GameBuildingEvent(eventFromBattle, {this, missionLocationBattle}));
			break;
		}
		case GameEventType::MissionCompletedVehicle:
		{
			fw().pushEvent(new GameEvent(eventFromBattle));
			break;
		}
		default:
			break;
	}
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
		location = {gve->building->bounds.p0.x, gve->building->bounds.p0.y, 1};
	}
	else if (GameAgentEvent *gae = dynamic_cast<GameAgentEvent *>(ev))
	{
		if (gae->agent->unit)
		{
			location = gae->agent->unit->position;
		}
		else
		{
			location = gae->agent->position;
		}
	}
	else if (GameBaseEvent *gbe = dynamic_cast<GameBaseEvent *>(ev))
	{
		location =
		    Vec3<int>(gbe->base->building->bounds.p0.x + gbe->base->building->bounds.p1.x,
		              gbe->base->building->bounds.p0.y + gbe->base->building->bounds.p1.y, 1) /
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

int GameScore::getTotal()
{
	return tacticalMissions + researchCompleted + alienIncidents + craftShotDownUFO +
	       craftShotDownXCom + incursions + cityDamage;
}

void GameState::loadMods()
{
	auto mods = Options::modList.get().split(":");
	for (const auto &modString : mods)
	{
		LogWarning("loading mod \"%s\"", modString);
		auto modPath = Options::modPath.get() + "/" + modString;
		auto modInfo = ModInfo::getInfo(modPath);
		if (!modInfo)
		{
			LogError("Failed to load ModInfo for mod \"%s\"", modString);
			continue;
		}
		LogWarning("Loaded modinfo for mod ID \"%s\"", modInfo->getID());
		if (modInfo->getStatePath() != "")
		{
			auto modStatePath = modPath + "/" + modInfo->getStatePath();
			LogWarning("Loading mod gamestate \"%s\"", modStatePath);

			if (!this->loadGame(modStatePath))
			{
				LogError("Failed to load mod ID \"%s\"", modInfo->getID());
			}
		}

		const auto &modLoadScript = modInfo->getModLoadScript();

		if (!modLoadScript.empty())
		{
			LogInfo("Executing modLoad script \"%s\" for mod \"%s\"", modLoadScript,
			        modInfo->getID());
			this->luaGameState.runScript(modLoadScript);
		}
	}
}

bool GameState::appendGameState(const UString &gamestatePath)
{
	LogInfo("Appending gamestate \"%s\"", gamestatePath);
	auto file = fw().data->fs.open(gamestatePath);
	if (!file)
	{
		LogWarning("Failed to open gamestate file \"%s\"", gamestatePath);
		return false;
	}
	return this->loadGame(file.systemPath());
}

}; // namespace OpenApoc

#include "game/ui/skirmish/skirmish.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/battle/battle.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/facility.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlemap.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/ui/battle/battlebriefing.h"
#include "game/ui/general/aequipscreen.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/skirmish/mapselector.h"
#include "game/ui/skirmish/selectforces.h"
namespace OpenApoc
{

namespace
{

std::shared_future<void> loadBattleBuilding(bool hotseat, sp<Building> building, GameState *state,
                                            StateRef<Base> playerBase, bool raid, bool customAliens,
                                            std::map<StateRef<AgentType>, int> aliens,
                                            bool customGuards, int guards, bool customCivilians,
                                            int civilians, int score)
{
	std::map<StateRef<AgentType>, int> aliensLocal;

	auto loadTask = fw().threadPoolEnqueue([hotseat, building, state, raid, aliensLocal, guards,
	                                        civilians, aliens, customAliens, customGuards,
	                                        customCivilians, playerBase, score]() -> void {
		std::list<StateRef<Agent>> agents;
		if (playerBase->building == building)
		{
			// No agents for base defense! Auto-chosen
		}
		else
		{
			for (auto &a : state->agents)
			{
				if (a.second->type->role == AgentType::Role::Soldier &&
				    a.second->homeBuilding == playerBase->building)
				{
					a.second->enterBuilding(*state, {state, building});
					agents.emplace_back(state, a.second);
				}
			}
		}
		StateRef<Organisation> org = raid ? building->owner : state->getAliens();
		StateRef<Building> bld = {state, building};
		StateRef<Vehicle> veh = {};

		const std::map<StateRef<AgentType>, int> *aliensRef = customAliens ? &aliens : nullptr;
		const int *guardsRef = customGuards ? &guards : nullptr;
		const int *civiliansRef = customCivilians ? &civilians : nullptr;

		Battle::beginBattle(*state, hotseat, org, agents, aliensRef, guardsRef, civiliansRef, veh,
		                    bld);
		// Skirmish settings
		state->current_battle->skirmish = true;
		state->current_battle->scoreBeforeSkirmish = state->totalScore.tacticalMissions;
		state->totalScore.tacticalMissions = score;
		for (auto &o : state->organisations)
		{
			if (o.first == state->getPlayer().id)
			{
				continue;
			}
			state->current_battle->relationshipsBeforeSkirmish[{state, o.first}] =
			    o.second->getRelationTo(state->getPlayer());
		}
	});
	return loadTask;
}

std::shared_future<void> loadBattleVehicle(bool hotseat, sp<VehicleType> vehicle, GameState *state,
                                           StateRef<Base> playerBase, bool customAliens,
                                           std::map<StateRef<AgentType>, int> aliens, int score)
{

	auto loadTask = fw().threadPoolEnqueue(
	    [hotseat, vehicle, state, customAliens, aliens, playerBase, score]() -> void {
		    std::list<StateRef<Agent>> agents;
		    for (auto &a : state->agents)
			    if (a.second->type->role == AgentType::Role::Soldier &&
			        a.second->homeBuilding == playerBase->building)
				    agents.emplace_back(state, a.second);

		    StateRef<Organisation> org = {state, UString("ORG_ALIEN")};
		    auto v = mksp<Vehicle>();
		    auto vID = Vehicle::generateObjectID(*state);
		    v->type = {state, vehicle};
		    v->name = format("%s %d", v->type->name, ++v->type->numCreated);

		    state->vehicles[vID] = v;
		    StateRef<Vehicle> ufo = {state, vID};
		    ufo->owner = state->getAliens();
		    ufo->city = playerBase->building->city;

		    auto playerVeh = state->current_city->placeVehicle(
		        *state, StateRef<VehicleType>{state, "VEHICLETYPE_BIOTRANS"}, state->getPlayer(),
		        playerBase->building);
		    playerVeh->homeBuilding = playerBase->building;
		    StateRef<Vehicle> playerVehRef = {state, playerVeh};
		    playerVehRef->leaveBuilding(*state, {20, 20, 11});

		    for (auto &agent : agents)
		    {
			    agent->enterVehicle(*state, playerVehRef);
		    }
		    const std::map<StateRef<AgentType>, int> *aliensRef = customAliens ? &aliens : nullptr;
		    Battle::beginBattle(*state, hotseat, org, agents, aliensRef, playerVehRef, ufo);
		    // Skirmish settings
		    state->current_battle->skirmish = true;
		    state->current_battle->scoreBeforeSkirmish = state->totalScore.tacticalMissions;
		    state->totalScore.tacticalMissions = score;
		    for (auto &o : state->organisations)
		    {
			    if (o.first == state->getPlayer().id)
			    {
				    continue;
			    }
			    state->current_battle->relationshipsBeforeSkirmish[{state, o.first}] =
			        o.second->getRelationTo(state->getPlayer());
		    }
	    });

	return loadTask;
}
} // namespace

Skirmish::Skirmish(sp<GameState> state) : Stage(), menuform(ui().getForm("skirmish")), state(*state)
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	updateLocationLabel();
	menuform->findControlTyped<ScrollBar>("NUM_HUMANS_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_HUMANS")
		        ->setText(format(
		            "%d", menuform->findControlTyped<ScrollBar>("NUM_HUMANS_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("NUM_HYBRIDS_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_HYBRIDS")
		        ->setText(format(
		            "%d", menuform->findControlTyped<ScrollBar>("NUM_HYBRIDS_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("NUM_ANDROIDS_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("NUM_ANDROIDS")
		        ->setText(format(
		            "%d",
		            menuform->findControlTyped<ScrollBar>("NUM_ANDROIDS_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("DAYS_PHYSICAL_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("DAYS_PHYSICAL")
		        ->setText(format(
		            "%d",
		            menuform->findControlTyped<ScrollBar>("DAYS_PHYSICAL_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("DAYS_PSI_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("DAYS_PSI")
		        ->setText(format(
		            "%d", menuform->findControlTyped<ScrollBar>("DAYS_PSI_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("PLAYER_TECH_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("PLAYER_TECH")
		        ->setText(
		            menuform->findControlTyped<ScrollBar>("PLAYER_TECH_SLIDER")->getValue() == 0
		                ? "NO"
		                : format("%d", menuform->findControlTyped<ScrollBar>("PLAYER_TECH_SLIDER")
		                                   ->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("ALIEN_SCORE_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("ALIEN_SCORE")
		        ->setText(format(
		            "%dK",
		            menuform->findControlTyped<ScrollBar>("ALIEN_SCORE_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("ORG_SCORE_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    menuform->findControlTyped<Label>("ORG_SCORE")
		        ->setText(format(
		            "%d", menuform->findControlTyped<ScrollBar>("ORG_SCORE_SLIDER")->getValue()));
	    });
	menuform->findControlTyped<ScrollBar>("ARMOR_SLIDER")
	    ->addCallback(FormEventType::ScrollBarChange, [this](Event *) {
		    UString armor = "";
		    switch (menuform->findControlTyped<ScrollBar>("ARMOR_SLIDER")->getValue())
		    {
			    case 0:
				    armor = "NONE";
				    break;
			    case 1:
				    armor = "MEGAPOL";
				    break;
			    case 2:
				    armor = "MEGAPOL+MB";
				    break;
			    case 3:
				    armor = "MARSEC";
				    break;
			    case 4:
				    armor = "X-COM+MB";
				    break;
			    case 5:
				    armor = "X-COM";
				    break;
			    default:
				    break;
		    }
		    menuform->findControlTyped<Label>("ARMOR")->setText(armor);
	    });

	menuform->findControlTyped<ScrollBar>("NUM_HUMANS_SLIDER")->setValue(8);
	menuform->findControlTyped<ScrollBar>("NUM_HYBRIDS_SLIDER")->setValue(2);
	menuform->findControlTyped<ScrollBar>("NUM_ANDROIDS_SLIDER")->setValue(2);

	menuform->findControlTyped<ScrollBar>("DAYS_PHYSICAL_SLIDER")->setValue(7);
	menuform->findControlTyped<ScrollBar>("DAYS_PSI_SLIDER")->setValue(7);
	menuform->findControlTyped<ScrollBar>("PLAYER_TECH_SLIDER")->setValue(1);

	menuform->findControlTyped<ScrollBar>("ALIEN_SCORE_SLIDER")->setValue(12);
	menuform->findControlTyped<ScrollBar>("ORG_SCORE_SLIDER")->setValue(4);
	menuform->findControlTyped<ScrollBar>("ARMOR_SLIDER")->setValue(4);
}

Skirmish::~Skirmish() = default;

UString Skirmish::getLocationText()
{
	return menuform->findControlTyped<Label>("LOCATION")->getText();
}

void Skirmish::setLocation(StateRef<Building> building)
{
	clearLocation();
	locBuilding = building;
	updateLocationLabel();
}

void Skirmish::setLocation(StateRef<VehicleType> veh)
{
	clearLocation();
	locVehicle = veh;
	updateLocationLabel();
}

void Skirmish::setLocation(StateRef<Base> base)
{
	clearLocation();
	locBase = base;
	updateLocationLabel();
}

void Skirmish::goToBattle(bool customAliens, std::map<StateRef<AgentType>, int> aliens,
                          bool customGuards, int guards, bool customCivilians, int civilians)
{
	auto score = menuform->findControlTyped<ScrollBar>("ALIEN_SCORE_SLIDER")->getValue() * 1000;

	// Create a temporary base
	auto sourceBase = locBase ? locBase : StateRef<Base>(&state, "BASE_1");
	auto city = sourceBase->building->city;

	auto newBuilding = mksp<Building>();
	city->buildings["BUILDING_SKIRMISH"] = newBuilding;

	auto newBase = mksp<Base>();
	state.player_bases["BASE_SKIRMISH"] = newBase;

	StateRef<Building> playerBuilding = {&state, "BUILDING_SKIRMISH"};
	StateRef<Base> playerBase = {&state, "BASE_SKIRMISH"};

	playerBuilding->owner = state.getPlayer();
	playerBuilding->base = playerBase;
	playerBuilding->city = city;
	playerBuilding->carEntranceLocation = {0, 0, 0};
	playerBuilding->landingPadLocations.emplace(0, 0, 0);

	playerBase->building = playerBuilding;
	playerBase->corridors = sourceBase->corridors;
	for (auto &f : sourceBase->facilities)
	{
		playerBase->buildFacility(state, f->type, f->pos, false);
	}
	for (auto &f : playerBase->facilities)
	{
		f->buildTime = 0;
	}

	LogWarning("Adding new agents to base %s", playerBase.id);
	int countHumans = menuform->findControlTyped<ScrollBar>("NUM_HUMANS_SLIDER")->getValue();
	int countHybrids = menuform->findControlTyped<ScrollBar>("NUM_HYBRIDS_SLIDER")->getValue();
	int countAndroids = menuform->findControlTyped<ScrollBar>("NUM_ANDROIDS_SLIDER")->getValue();
	int playerTech = menuform->findControlTyped<ScrollBar>("PLAYER_TECH_SLIDER")->getValue();
	unsigned int physTicks =
	    menuform->findControlTyped<ScrollBar>("DAYS_PHYSICAL_SLIDER")->getValue() * TICKS_PER_DAY;
	unsigned int psiTicks =
	    menuform->findControlTyped<ScrollBar>("DAYS_PSI_SLIDER")->getValue() * TICKS_PER_DAY;
	StateRef<AgentType> human = {&state, "AGENTTYPE_X-COM_AGENT_HUMAN"};
	StateRef<AgentType> hybrid = {&state, "AGENTTYPE_X-COM_AGENT_HYBRID"};
	StateRef<AgentType> android = {&state, "AGENTTYPE_X-COM_AGENT_ANDROID"};
	auto player = state.getPlayer();
	std::list<StateRef<Agent>> agents;
	for (int i = 0; i < countHumans; i++)
	{
		if (agents.size() >= MAX_UNITS_PER_SIDE)
		{
			break;
		}
		agents.emplace_back(state.agent_generator.createAgent(state, player, human));
	}
	for (int i = 0; i < countHybrids; i++)
	{
		if (agents.size() >= MAX_UNITS_PER_SIDE)
		{
			break;
		}
		agents.emplace_back(state.agent_generator.createAgent(state, player, hybrid));
	}
	for (int i = 0; i < countAndroids; i++)
	{
		if (agents.size() >= MAX_UNITS_PER_SIDE)
		{
			break;
		}
		agents.emplace_back(state.agent_generator.createAgent(state, player, android));
	}
	for (auto &agent : agents)
	{
		auto initialEquipment =
		    playerTech == 0
		        ? std::list<const AEquipmentType *>()
		        : EquipmentSet::getByLevel(state, playerTech)->generateEquipmentList(state);

		int initialArmorType = menuform->findControlTyped<ScrollBar>("ARMOR_SLIDER")->getValue();
		switch (initialArmorType)
		{
			case 1:
				// armor = "MEGAPOL";
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_HELMET"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_BODY_ARMOR"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_LEFT_ARM_ARMOR"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_RIGHT_ARM_ARMOR"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_LEG_ARMOR"].get());
				break;
			case 2:
				// armor = "MEGAPOL+MB";
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_HELMET"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_BODY_UNIT"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_LEFT_ARM_ARMOR"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_RIGHT_ARM_ARMOR"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_LEG_ARMOR"].get());
				break;
			case 3:
				// armor = "MARSEC";
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_HEAD_UNIT"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_BODY_UNIT"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_LEFT_ARM_UNIT"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_RIGHT_ARM_UNIT"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_LEG_UNITS"].get());
				break;
			case 4:
				// armor = "X-COM+MB";
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_HEAD_SHIELD"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_BODY_UNIT"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_LEFT_ARM_SHIELD"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_RIGHT_ARM_SHIELD"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_LEG_SHIELDS"].get());
				break;
			case 5:
				// armor = "X-COM";
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_HEAD_SHIELD"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_BODY_SHIELD"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_LEFT_ARM_SHIELD"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_RIGHT_ARM_SHIELD"].get());
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_LEG_SHIELDS"].get());
				break;
			default:
				break;
		}

		// Add initial equipment
		for (auto &t : initialEquipment)
		{
			if (!t)
			{
				continue;
			}
			agent->addEquipmentByType(state, {&state, t->id}, true);
		}

		agent->trainPhysical(state, physTicks);
		agent->trainPsi(state, psiTicks);
		agent->homeBuilding = playerBase->building;
		agent->enterBuilding(state, agent->homeBuilding);
	}
	if (locBase && customCivilians)
	{
		for (int i = 0; i < civilians; i++)
		{
			auto a = state.agent_generator.createAgent(state, player, AgentType::Role::Engineer);
			a->homeBuilding = playerBase->building;
			a->enterBuilding(state, a->homeBuilding);
			agents.emplace_back(a);
		}
	}

	LogWarning("Resetting base inventory");
	playerBase->inventoryAgentEquipment.clear();
	for (auto &t : state.agent_equipment)
	{
		// Ignore unfinished items
		if (t.second->type == AEquipmentType::Type::AlienDetector ||
		    t.second->type == AEquipmentType::Type::DimensionForceField ||
		    t.second->type == AEquipmentType::Type::MindShield ||
		    t.second->type == AEquipmentType::Type::MultiTracker ||
		    t.second->type == AEquipmentType::Type::StructureProbe ||
		    t.second->type == AEquipmentType::Type::VortexAnalyzer)
		{
			// continue;
		}
		// Ignore alien builtin weapons
		if (t.second->store_space == 5 && t.second->manufacturer == state.getAliens())
		{
			continue;
		}
		// Ignore special attacks
		if (t.second->store_space == 0)
		{
			continue;
		}
		// Ignore bio
		if (t.second->bioStorage)
		{
			continue;
		}
		// Manual exclusion
		if (t.first == "AEQUIPMENTTYPE_FORCEWEB" || t.first == "AEQUIPMENTTYPE_ENERGY_POD" ||
		    t.first == "AEQUIPMENTTYPE_DIMENSION_DESTABILISER" ||
		    t.first == "AEQUIPMENTTYPE_ELERIUM" || t.first == "AEQUIPMENTTYPE_PSICLONE" ||
		    t.first == "AEQUIPMENTTYPE_TRACKER_GUN" ||
		    t.first == "AEQUIPMENTTYPE_TRACKER_GUN_CLIP" || t.first == "AEQUIPMENTTYPE_PSI-GRENADE")
		{
			continue;
		}

		if (t.second->type == AEquipmentType::Type::Ammo)
		{
			playerBase->inventoryAgentEquipment[t.first] = 100 * t.second->max_ammo;
		}
		else
		{
			playerBase->inventoryAgentEquipment[t.first] = 100;
		}
	}

	bool hotseat = menuform->findControlTyped<CheckBox>("HOTSEAT")->isChecked();

	loadBattle = [this, score, hotseat, playerBase, customAliens, aliens, customGuards, guards,
	              customCivilians, civilians]() {
		if (locBuilding)
		{
			bool raid = menuform->findControlTyped<CheckBox>("ALTERNATIVE_ATTACK")->isChecked();
			if (raid)
			{
				locBuilding->owner->tech_level =
				    menuform->findControlTyped<ScrollBar>("ORG_SCORE_SLIDER")->getValue();
			}
			battleInBuilding(hotseat, playerBase, locBuilding, raid, customAliens, aliens,
			                 customGuards, guards, customCivilians, civilians, score);
		}
		else if (locVehicle)
		{
			battleInVehicle(hotseat, playerBase, locVehicle, customAliens, aliens, score);
		}
		else if (locBase)
		{
			battleInBase(hotseat, playerBase, customAliens, aliens, score);
		}
	};

	sp<Agent> firstAgent;
	for (auto &a : state.agents)
	{
		if (a.second->homeBuilding.id == "BUILDING_SKIRMISH")
		{
			firstAgent = a.second;
			break;
		}
	}

	fw().stageQueueCommand(
	    {StageCmd::Command::PUSH, mksp<AEquipScreen>(state.shared_from_this(), firstAgent)});
}

void Skirmish::customizeForces(bool force)
{
	// No map selected
	if (!locBuilding && !locVehicle && !locBase)
	{
		return;
	}
	std::map<StateRef<AgentType>, int> *aliens = nullptr;
	std::map<StateRef<AgentType>, int> aliensNone;
	if (locVehicle)
	{
		aliens = &locVehicle->crew_downed;
	}
	/*
	else if (locBuilding && locBuilding->hasAliens())
	{
	    aliens = locBuilding->preset_crew.empty() ? &locBuilding->current_crew
	                                              : &locBuilding->preset_crew;
	}
	*/
	else if (locBuilding && !locBuilding->preset_crew.empty())
	{
		aliens = &locBuilding->preset_crew;
	}
	else if (force)
	{
		aliens = &aliensNone;
	}
	int guards = -1;
	if (locBuilding)
	{
		if (locBuilding->owner != state.getAliens())
		{
			guards = 0;
			if (menuform->findControlTyped<CheckBox>("ALTERNATIVE_ATTACK")->isChecked())
			{
				guards = locBuilding->owner->getGuardCount(state);
			}
		}
	}
	int civilians = -1;
	if (locBase)
	{
		civilians = 5;
	}
	if (locBuilding)
	{
		civilians = state.getCivilian()->getGuardCount(state);
	}

	fw().stageQueueCommand(
	    {StageCmd::Command::PUSH, mksp<SelectForces>(state.shared_from_this(), *this, aliens,
	                                                 guards == 0 ? nullptr : &guards,
	                                                 civilians == 0 ? nullptr : &civilians)});
}

void Skirmish::clearLocation()
{
	locBuilding.clear();
	locVehicle.clear();
	locBase.clear();
}

void Skirmish::updateLocationLabel()
{
	UString text = "[No map selected]";
	if (locBuilding)
	{
		text = format("[%s Building] %s [%s]",
		              locBuilding->owner == state.getAliens() ? "Alien" : "Human",
		              locBuilding->name, locBuilding->battle_map.id);
	}
	else if (locVehicle)
	{
		text = format("[UFO] %s [%s]", locVehicle->name, locVehicle->battle_map.id);
	}
	else if (locBase)
	{
		text = format("[Base] %s", locBase->name);
	}
	menuform->findControlTyped<Label>("LOCATION")->setText(format("LOCATION: %s", text));
}

void Skirmish::battleInBuilding(bool hotseat, StateRef<Base> playerBase,
                                StateRef<Building> building, bool raid, bool customAliens,
                                std::map<StateRef<AgentType>, int> aliens, bool customGuards,
                                int guards, bool customCivilians, int civilians, int score)
{
	fw().stageQueueCommand(
	    {StageCmd::Command::REPLACEALL,
	     mksp<BattleBriefing>(state.shared_from_this(), building->owner,
	                          Building::getId(state, building), true, raid,
	                          loadBattleBuilding(hotseat, building, &state, playerBase, raid,
	                                             customAliens, aliens, customGuards, guards,
	                                             customCivilians, civilians, score))});
}

void Skirmish::battleInBase(bool hotseat, StateRef<Base> base, bool customAliens,
                            std::map<StateRef<AgentType>, int> aliens, int score)
{
	fw().stageQueueCommand(
	    {StageCmd::Command::REPLACEALL,
	     mksp<BattleBriefing>(
	         state.shared_from_this(), state.getAliens(), base->building.id, true, true,
	         loadBattleBuilding(hotseat, base->building, &state, base, false, customAliens, aliens,
	                            false, 0, false, 0, score))});
}

void Skirmish::battleInVehicle(bool hotseat, StateRef<Base> playerBase,
                               StateRef<VehicleType> vehicle, bool customAliens,
                               std::map<StateRef<AgentType>, int> aliens, int score)
{
	fw().stageQueueCommand(
	    {StageCmd::Command::REPLACEALL,
	     mksp<BattleBriefing>(state.shared_from_this(), state.getAliens(),
	                          VehicleType::getId(state, vehicle), false, false,
	                          loadBattleVehicle(hotseat, vehicle, &state, playerBase, customAliens,
	                                            aliens, score))});
}

void Skirmish::begin()
{
	fw().stageQueueCommand(
	    {StageCmd::Command::PUSH, mksp<MapSelector>(state.shared_from_this(), *this)});
}

void Skirmish::pause() {}

void Skirmish::resume()
{
	if (loadBattle)
	{
		loadBattle();
	}
}

void Skirmish::finish() {}

void Skirmish::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
		if (e->keyboard().KeyCode == SDLK_RETURN || e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_OK")->click();
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_OK")
		{
			if (!locBase && !locBuilding && !locVehicle)
			{
				fw().stageQueueCommand(
				    {StageCmd::Command::PUSH, mksp<MapSelector>(state.shared_from_this(), *this)});
				return;
			}

			bool customize =
			    menuform->findControlTyped<CheckBox>("CUSTOMISE_FORCES")->isChecked() || locBase ||
			    (!menuform->findControlTyped<CheckBox>("ALTERNATIVE_ATTACK")->isChecked() &&
			     locBuilding && locBuilding->owner != state.aliens);
			if (customize)
			{
				customizeForces(true);
			}
			else
			{
				goToBattle();
			}
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_SELECTMAP")
		{
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH, mksp<MapSelector>(state.shared_from_this(), *this)});
			return;
		}
	}
}

void Skirmish::update() { menuform->update(); }

void Skirmish::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool Skirmish::isTransition() { return false; }
} // namespace OpenApoc

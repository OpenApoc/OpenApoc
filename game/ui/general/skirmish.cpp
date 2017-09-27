#include "game/ui/general/skirmish.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/scrollbar.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/base/base.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlemap.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/vehicle_type.h"
#include "game/ui/battle/battlebriefing.h"
#include "game/ui/general/mapselector.h"
#include "game/ui/general/messagebox.h"
#include "game/ui/general/selectforces.h"
namespace OpenApoc
{

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
		        ->setText(menuform->findControlTyped<ScrollBar>("PLAYER_TECH_SLIDER")->getValue() ==
		                          0
		                      ? "NO"
		                      : format("%d",
		                               menuform->findControlTyped<ScrollBar>("PLAYER_TECH_SLIDER")
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

void Skirmish::goToBattle(std::map<StateRef<AgentType>, int> *aliens, int *guards, int *civilians)
{
	state.score = menuform->findControlTyped<ScrollBar>("ALIEN_SCORE_SLIDER")->getValue() * 1000;

	auto playerBase = locBase ? locBase : StateRef<Base>(&state, "BASE_1");

	LogWarning("Erasing agents from base %s", playerBase.id);
	std::set<UString> agentsToRemove;
	for (auto &a : state.agents)
	{
		if (a.second->type->role == AgentType::Role::Soldier &&
		    a.second->homeBuilding->base == playerBase)
		{
			agentsToRemove.insert(a.first);
			a.second->destroy();
		}
	}
	for (auto &a : agentsToRemove)
	{
		state.agents.erase(a);
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
		        ? std::list<sp<AEquipmentType>>()
		        : EquipmentSet::getByLevel(state, playerTech)->generateEquipmentList(state);

		int initialArmorType = menuform->findControlTyped<ScrollBar>("ARMOR_SLIDER")->getValue();
		switch (initialArmorType)
		{
			case 1:
				// armor = "MEGAPOL";
				initialEquipment.push_back(state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_HELMET"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_BODY_ARMOR"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_LEFT_ARM_ARMOR"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_RIGHT_ARM_ARMOR"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_LEG_ARMOR"]);
				break;
			case 2:
				// armor = "MEGAPOL+MB";
				initialEquipment.push_back(state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_HELMET"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_BODY_UNIT"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_LEFT_ARM_ARMOR"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_RIGHT_ARM_ARMOR"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MEGAPOL_LEG_ARMOR"]);
				break;
			case 3:
				// armor = "MARSEC";
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_HEAD_UNIT"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_BODY_UNIT"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_LEFT_ARM_UNIT"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_RIGHT_ARM_UNIT"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_LEG_UNITS"]);
				break;
			case 4:
				// armor = "X-COM+MB";
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_HEAD_SHIELD"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_MARSEC_BODY_UNIT"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_LEFT_ARM_SHIELD"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_RIGHT_ARM_SHIELD"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_LEG_SHIELDS"]);
				break;
			case 5:
				// armor = "X-COM";
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_HEAD_SHIELD"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_BODY_SHIELD"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_LEFT_ARM_SHIELD"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_RIGHT_ARM_SHIELD"]);
				initialEquipment.push_back(
				    state.agent_equipment["AEQUIPMENTTYPE_X-COM_LEG_SHIELDS"]);
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

	if (locBuilding)
	{
		bool raid = menuform->findControlTyped<CheckBox>("ALTERNATIVE_ATTACK")->isChecked();
		if (raid)
		{
			locBuilding->owner->tech_level =
			    menuform->findControlTyped<ScrollBar>("ORG_SCORE_SLIDER")->getValue();
		}
		battleInBuilding(hotseat, playerBase, locBuilding, raid, aliens, guards, civilians);
	}
	else if (locVehicle)
	{
		battleInVehicle(hotseat, playerBase, locVehicle, aliens);
	}
	else if (locBase)
	{
		battleInBase(hotseat, locBase, aliens);
	}
	// No map selected
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
	else if (locBuilding && locBuilding->hasAliens())
	{
		aliens = locBuilding->preset_crew.empty() ? &locBuilding->current_crew
		                                          : &locBuilding->preset_crew;
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

std::shared_future<void> loadBattleBuilding(bool hotseat, sp<Building> building, GameState *state,
                                            StateRef<Base> playerBase, bool raid,
                                            std::map<StateRef<AgentType>, int> *aliens = nullptr,
                                            int *guards = nullptr, int *civilians = nullptr)
{
	std::map<StateRef<AgentType>, int> aliensLocal;
	bool aliensPresent = false;
	if (aliens)
	{
		aliensLocal = *aliens;
		aliensPresent = true;
	}
	int guardsLocal = 0;
	bool guardsPresent = false;
	if (guards)
	{
		guardsLocal = *guards;
		guardsPresent = true;
	}
	int civiliansLocal = 0;
	bool civiliansPresent = false;
	if (civilians)
	{
		civiliansLocal = *civilians;
		civiliansPresent = true;
	}
	auto loadTask = fw().threadPoolEnqueue([hotseat, building, state, raid, aliensLocal,
	                                        guardsLocal, civiliansLocal, aliensPresent,
	                                        guardsPresent, civiliansPresent, playerBase]() -> void {
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
					agents.emplace_back(state, a.second);
				}
			}
		}
		StateRef<Organisation> org = raid ? building->owner : state->getAliens();
		StateRef<Building> bld = {state, building};
		StateRef<Vehicle> veh = {};

		const std::map<StateRef<AgentType>, int> *aliens = aliensPresent ? &aliensLocal : nullptr;
		const int *guards = guardsPresent ? &guardsLocal : nullptr;
		const int *civilians = civiliansPresent ? &civiliansLocal : nullptr;

		Battle::beginBattle(*state, hotseat, org, agents, aliens, guards, civilians, veh, bld);
	});

	return loadTask;
}

std::shared_future<void> loadBattleVehicle(bool hotseat, sp<VehicleType> vehicle, GameState *state,
                                           StateRef<Base> playerBase,
                                           std::map<StateRef<AgentType>, int> *aliens = nullptr)
{

	auto loadTask = fw().threadPoolEnqueue([hotseat, vehicle, state, aliens, playerBase]() -> void {
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
		StateRef<Vehicle> veh = {};

		Battle::beginBattle(*state, hotseat, org, agents, aliens, veh, ufo);
	});

	return loadTask;
}

void Skirmish::battleInBuilding(bool hotseat, StateRef<Base> playerBase,
                                StateRef<Building> building, bool raid,
                                std::map<StateRef<AgentType>, int> *aliens, int *guards,
                                int *civilians)
{
	fw().stageQueueCommand(
	    {StageCmd::Command::REPLACEALL,
	     mksp<BattleBriefing>(state.shared_from_this(), building->owner,
	                          Building::getId(state, building), true, raid,
	                          loadBattleBuilding(hotseat, building, &state, playerBase, raid,
	                                             aliens, guards, civilians))});
}

void Skirmish::battleInBase(bool hotseat, StateRef<Base> base,
                            std::map<StateRef<AgentType>, int> *aliens)
{
	fw().stageQueueCommand(
	    {StageCmd::Command::REPLACEALL,
	     mksp<BattleBriefing>(
	         state.shared_from_this(), state.getAliens(), base->building.id, true, true,
	         loadBattleBuilding(hotseat, base->building, &state, base, false, aliens))});
}

void Skirmish::battleInVehicle(bool hotseat, StateRef<Base> playerBase,
                               StateRef<VehicleType> vehicle,
                               std::map<StateRef<AgentType>, int> *aliens)
{
	fw().stageQueueCommand(
	    {StageCmd::Command::REPLACEALL,
	     mksp<BattleBriefing>(state.shared_from_this(), state.getAliens(),
	                          VehicleType::getId(state, vehicle), false, false,
	                          loadBattleVehicle(hotseat, vehicle, &state, playerBase, aliens))});
}

void Skirmish::begin()
{
	fw().stageQueueCommand(
	    {StageCmd::Command::PUSH, mksp<MapSelector>(state.shared_from_this(), *this)});
}

void Skirmish::pause() {}

void Skirmish::resume() {}

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
		if (e->keyboard().KeyCode == SDLK_RETURN)
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
			     locBuilding && !locBuilding->hasAliens());
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
}

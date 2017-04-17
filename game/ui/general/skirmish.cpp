#include "game/ui/general/skirmish.h"
#include "forms/form.h"
#include "forms/checkbox.h"
#include "game/ui/general/mapselector.h"
#include "game/ui/general/selectforces.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/rules/vehicle_type.h"
#include "game/ui/battle/battlebriefing.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/building.h"
#include "game/state/base/base.h"
#include "game/state/gamestate.h"
#include "forms/scrollbar.h"
#include "game/state/battle/battlemap.h"
#include "game/state/battle/battle.h"
#include "forms/label.h"
namespace OpenApoc
{
	
Skirmish::Skirmish(sp<GameState> state)
: Stage(), state(*state), menuform(ui().getForm("skirmish"))
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	updateLocationLabel();
	menuform->findControlTyped<ScrollBar>("NUM_HUMANS_SLIDER")
		->addCallback(FormEventType::ScrollBarChange, [this](Event *e) {
		menuform->findControlTyped<Label>("NUM_HUMANS")->setText(format("%d", menuform->findControlTyped<ScrollBar>("NUM_HUMANS_SLIDER")->getValue()));
	});
	menuform->findControlTyped<ScrollBar>("NUM_HYBRIDS_SLIDER")
		->addCallback(FormEventType::ScrollBarChange, [this](Event *e) {
		menuform->findControlTyped<Label>("NUM_HYBRIDS")->setText(format("%d", menuform->findControlTyped<ScrollBar>("NUM_HYBRIDS_SLIDER")->getValue()));
	});
	menuform->findControlTyped<ScrollBar>("NUM_ANDROIDS_SLIDER")
		->addCallback(FormEventType::ScrollBarChange, [this](Event *e) {
		menuform->findControlTyped<Label>("NUM_ANDROIDS")->setText(format("%d", menuform->findControlTyped<ScrollBar>("NUM_ANDROIDS_SLIDER")->getValue()));
	});
	menuform->findControlTyped<ScrollBar>("DAYS_PHYSICAL_SLIDER")
		->addCallback(FormEventType::ScrollBarChange, [this](Event *e) {
		menuform->findControlTyped<Label>("DAYS_PHYSICAL")->setText(format("%d", menuform->findControlTyped<ScrollBar>("DAYS_PHYSICAL_SLIDER")->getValue()));
	});
	menuform->findControlTyped<ScrollBar>("DAYS_PSI_SLIDER")
		->addCallback(FormEventType::ScrollBarChange, [this](Event *e) {
		menuform->findControlTyped<Label>("DAYS_PSI")->setText(format("%d", menuform->findControlTyped<ScrollBar>("DAYS_PSI_SLIDER")->getValue()));
	});
	menuform->findControlTyped<ScrollBar>("PLAYER_TECH_SLIDER")
		->addCallback(FormEventType::ScrollBarChange, [this](Event *e) {
		menuform->findControlTyped<Label>("PLAYER_TECH")->setText(menuform->findControlTyped<ScrollBar>("PLAYER_TECH_SLIDER")->getValue() == 0 ? "NO" : format("%d", menuform->findControlTyped<ScrollBar>("PLAYER_TECH_SLIDER")->getValue()));
	});
	menuform->findControlTyped<ScrollBar>("ALIEN_SCORE_SLIDER")
		->addCallback(FormEventType::ScrollBarChange, [this](Event *e) {
		menuform->findControlTyped<Label>("ALIEN_SCORE")->setText(format("%dK", menuform->findControlTyped<ScrollBar>("ALIEN_SCORE_SLIDER")->getValue()));
	});
	menuform->findControlTyped<ScrollBar>("ORG_SCORE_SLIDER")
		->addCallback(FormEventType::ScrollBarChange, [this](Event *e) {
		menuform->findControlTyped<Label>("ORG_SCORE")->setText(format("%d", menuform->findControlTyped<ScrollBar>("ORG_SCORE_SLIDER")->getValue()));
	});
	menuform->findControlTyped<ScrollBar>("ARMOR_SLIDER")
		->addCallback(FormEventType::ScrollBarChange, [this](Event *e) {
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

	menuform->findControlTyped<ScrollBar>("DAYS_PHYSICAL_SLIDER")->setValue(14);
	menuform->findControlTyped<ScrollBar>("DAYS_PSI_SLIDER")->setValue(14);
	menuform->findControlTyped<ScrollBar>("PLAYER_TECH_SLIDER")->setValue(1);//Make it update!
	menuform->findControlTyped<ScrollBar>("PLAYER_TECH_SLIDER")->setValue(0);

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

void Skirmish::goToBattle()
{
	if (locBuilding)
	{
		bool raid = menuform->findControlTyped<CheckBox>("ALTERNATIVE_ATTACK")->isChecked();
		battleInBuilding(locBuilding, raid);
	}
	else if (locVehicle)
	{
		battleInVehicle(locVehicle);
	}
	else if (locBase)
	{
		battleInBase(locBase);
	}
	// No map selected
}

void Skirmish::customizeForces()
{
	// No map selected
	if (!locBuilding && !locVehicle && !locBase)
	{
		return;
	}
	fw().stageQueueCommand({ StageCmd::Command::PUSH, mksp<SelectForces>(state.shared_from_this(), *this) });
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
		text = format("[%s Building] %s [%s]", locBuilding->owner == state.getAliens() ? "Alien" : "Human", locBuilding->name, locBuilding->battle_map.id);
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

std::future<void> loadBattleBuilding(sp<Building> building, GameState * state, bool raid)
{

	auto loadTask = fw().threadPoolEnqueue([building, state, raid]() -> void {
		std::list<StateRef<Agent>> agents;
		for (auto &a : state->agents)
			if (a.second->type->role == AgentType::Role::Soldier &&
				a.second->owner == state->getPlayer())
				agents.emplace_back(state, a.second);

		StateRef<Organisation> org = raid ? building->owner : state->getAliens();
		StateRef<Building> bld = { state, building };
		StateRef<Vehicle> veh = {};

		Battle::beginBattle(*state, org, agents, veh, bld);
	});

	return loadTask;
}

std::future<void> loadBattleVehicle(sp<VehicleType> vehicle, GameState * state)
{

	auto loadTask = fw().threadPoolEnqueue([vehicle, state]() -> void {
		std::list<StateRef<Agent>> agents;
		for (auto &a : state->agents)
			if (a.second->type->role == AgentType::Role::Soldier &&
				a.second->owner == state->getPlayer())
				agents.emplace_back(state, a.second);

		StateRef<Organisation> org = { state, UString("ORG_ALIEN") };
		auto v = mksp<Vehicle>();

		auto vID = Vehicle::generateObjectID(*state);

		v->type = { state, vehicle };
		v->name = format("%s %d", v->type->name, ++v->type->numCreated);

		state->vehicles[vID] = v;
		StateRef<Vehicle> ufo = { state, vID };
		StateRef<Vehicle> veh = {};

		Battle::beginBattle(*state, org, agents, veh, ufo);
	});

	return loadTask;
}



void Skirmish::battleInBuilding(StateRef<Building> building, bool raid)
{
	fw().stageQueueCommand(
	{ StageCmd::Command::PUSH,
		mksp<BattleBriefing>(state.shared_from_this(), building->owner, Building::getId(state, building),
			true, true, loadBattleBuilding(building, &state, true)) });
}

void Skirmish::battleInBase(StateRef<Base> base)
{
}

void Skirmish::battleInVehicle(StateRef<VehicleType> vehicle)
{
	fw().stageQueueCommand(
	{ StageCmd::Command::PUSH,
		mksp<BattleBriefing>(state.shared_from_this(), state.getAliens(),
			VehicleType::getId(state, vehicle), false, false,
			loadBattleVehicle(vehicle, &state)) });
}

void Skirmish::begin(){
	fw().stageQueueCommand({ StageCmd::Command::PUSH, mksp<MapSelector>(state.shared_from_this(), *this) });
}

void Skirmish::pause(){}

void Skirmish::resume(){}

void Skirmish::finish(){}

void Skirmish::eventOccurred(Event * e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			fw().stageQueueCommand({ StageCmd::Command::POP });
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_OK")
		{
			bool customize = menuform->findControlTyped<CheckBox>("CUSTOMISE_FORCES")->isChecked() 
				|| (menuform->findControlTyped<CheckBox>("ALTERNATIVE_ATTACK")->isChecked() && locBuilding && locBuilding->owner != state.getAliens());
			if (customize)
			{
				customizeForces();
			}
			else
			{
				goToBattle();
			}
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_SELECTMAP")
		{
			fw().stageQueueCommand({ StageCmd::Command::PUSH, mksp<MapSelector>(state.shared_from_this(), *this) });
			return;
		}
	}

}

void Skirmish::update()
{
	menuform->update();
}

void Skirmish::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool Skirmish::isTransition()
{
	return false;
}

}
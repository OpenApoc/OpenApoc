#include "game/ui/general/mapselector.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/battle/battlemap.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/ui/battle/battlebriefing.h"
#include "game/ui/battle/battleview.h"
#include "game/ui/city/cityview.h"

namespace OpenApoc
{

MapSelector::MapSelector(sp<GameState> state)
    : Stage(), menuform(ui().getForm("FORM_MAPSELECTOR")), state(state)
{
	auto listbox = menuform->findControlTyped<ListBox>("LISTBOX_MAPS");
	std::set<sp<BattleMap>> seen_maps;
	for (auto &v : state->vehicle_types)
	{
		if (!v.second->battle_map || seen_maps.find(v.second->battle_map) != seen_maps.end())
			continue;
		seen_maps.insert(v.second->battle_map);
		listbox->addItem(createMapRowVehicle(v.second, state));
	}
	for (auto &c : state->cities)
	{
		for (auto &b : c.second->buildings)
		{
			if (seen_maps.find(b.second->battle_map) != seen_maps.end())
				continue;
			seen_maps.insert(b.second->battle_map);
			listbox->addItem(createMapRowBuilding(b.second, state));
		}
	}
}

MapSelector::~MapSelector() = default;

std::future<void> loadBattleBuilding(sp<Building> building, sp<GameState> state)
{

	auto loadTask = fw().threadPoolEnqueue([building, state]() -> void {
		std::list<StateRef<Agent>> agents;
		for (auto &a : state->agents)
			if (a.second->type->role == AgentType::Role::Soldier &&
			    a.second->owner == state->getPlayer())
				agents.emplace_back(state.get(), a.second);

		StateRef<Organisation> org = building->owner;
		StateRef<Building> bld = {state.get(), building};
		StateRef<Vehicle> veh = {};

		Battle::beginBattle(*state.get(), org, agents, veh, bld);
	});

	return loadTask;
}

sp<Control> MapSelector::createMapRowBuilding(sp<Building> building, sp<GameState> state)
{
	auto control = mksp<Control>();

	const int HEIGHT = 21;

	auto date = control->createChild<Label>(UString(""), ui().getFont("SMALFONT"));
	date->Location = {0, 0};
	date->Size = {100, HEIGHT};
	date->TextVAlign = VerticalAlignment::Centre;

	auto time = control->createChild<Label>(UString(""), ui().getFont("SMALFONT"));
	time->Location = date->Location + Vec2<int>{date->Size.x, 0};
	time->Size = {60, HEIGHT};
	time->TextVAlign = VerticalAlignment::Centre;

	auto text = control->createChild<Label>(building->name, ui().getFont("SMALFONT"));
	text->Location = time->Location + Vec2<int>{time->Size.x, 0};
	text->Size = {328, HEIGHT};
	text->TextVAlign = VerticalAlignment::Centre;

	{
		auto btnImage = fw().data->loadImage(
		    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:57:ui/menuopt.pal");
		auto btnLocation = control->createChild<GraphicButton>(btnImage, btnImage);
		btnLocation->Location = text->Location + Vec2<int>{text->Size.x, 0};
		btnLocation->Size = {22, HEIGHT};
		btnLocation->addCallback(FormEventType::ButtonClick, [building, state](Event *) {
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH,
			     mksp<BattleBriefing>(state, loadBattleBuilding(building, state))});
		});
	}

	return control;
}

std::future<void> loadBattleVehicle(sp<VehicleType> vehicle, sp<GameState> state)
{

	auto loadTask = fw().threadPoolEnqueue([vehicle, state]() -> void {
		std::list<StateRef<Agent>> agents;
		for (auto &a : state->agents)
			if (a.second->type->role == AgentType::Role::Soldier &&
			    a.second->owner == state->getPlayer())
				agents.emplace_back(state.get(), a.second);

		StateRef<Organisation> org = {state.get(), UString("ORG_ALIEN")};
		auto v = mksp<Vehicle>();

		v->type = {state.get(), vehicle};
		v->name = UString::format("%s %d", v->type->name, ++v->type->numCreated);

		state->vehicles[v->name] = v;
		StateRef<Vehicle> ufo = {state.get(), v->name};
		StateRef<Vehicle> veh = {};

		Battle::beginBattle(*state.get(), org, agents, veh, ufo);
	});

	return loadTask;
}

sp<Control> MapSelector::createMapRowVehicle(sp<VehicleType> vehicle, sp<GameState> state)
{
	auto control = mksp<Control>();

	const int HEIGHT = 21;

	auto date = control->createChild<Label>(UString(""), ui().getFont("SMALFONT"));
	date->Location = {0, 0};
	date->Size = {100, HEIGHT};
	date->TextVAlign = VerticalAlignment::Centre;

	auto time = control->createChild<Label>(UString(""), ui().getFont("SMALFONT"));
	time->Location = date->Location + Vec2<int>{date->Size.x, 0};
	time->Size = {60, HEIGHT};
	time->TextVAlign = VerticalAlignment::Centre;

	auto text = control->createChild<Label>(vehicle->name, ui().getFont("SMALFONT"));
	text->Location = time->Location + Vec2<int>{time->Size.x, 0};
	text->Size = {328, HEIGHT};
	text->TextVAlign = VerticalAlignment::Centre;

	{
		auto btnImage = fw().data->loadImage(
		    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:57:ui/menuopt.pal");
		auto btnLocation = control->createChild<GraphicButton>(btnImage, btnImage);
		btnLocation->Location = text->Location + Vec2<int>{text->Size.x, 0};
		btnLocation->Size = {22, HEIGHT};
		btnLocation->addCallback(FormEventType::ButtonClick, [vehicle, state](Event *) {
			fw().stageQueueCommand(
			    {StageCmd::Command::PUSH,
			     mksp<BattleBriefing>(state, loadBattleVehicle(vehicle, state))});
		});
	}

	return control;
}

void MapSelector::begin()
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
}

void MapSelector::pause() {}

void MapSelector::resume() {}

void MapSelector::finish() {}

void MapSelector::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_OK")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}
}

void MapSelector::update() { menuform->update(); }

void MapSelector::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool MapSelector::isTransition() { return false; }

}; // namespace OpenApoc

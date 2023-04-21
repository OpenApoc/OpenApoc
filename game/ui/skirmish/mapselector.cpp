#include "game/ui/skirmish/mapselector.h"
#include "forms/form.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemap.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/ui/battle/battlebriefing.h"
#include "game/ui/skirmish/skirmish.h"
#include "game/ui/tileview/battleview.h"
#include "game/ui/tileview/cityview.h"
#include "library/strings_format.h"

namespace OpenApoc
{

MapSelector::MapSelector(sp<GameState> state, Skirmish &skirmish)
    : Stage(), menuform(ui().getForm("mapselector")), skirmish(skirmish)
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());

	auto listbox = menuform->findControlTyped<ListBox>("LISTBOX_MAPS");
	std::set<sp<BattleMap>> seen_maps;
	for (auto &b : state->player_bases)
	{
		listbox->addItem(createMapRowBase({state.get(), b.first}, state));
	}
	for (auto &v : state->vehicle_types)
	{
		if (!v.second->battle_map || seen_maps.find(v.second->battle_map) != seen_maps.end())
			continue;
		seen_maps.insert(v.second->battle_map);
		listbox->addItem(createMapRowVehicle({state.get(), v.first}, state));
	}
	for (auto &c : state->cities)
	{
		for (auto &b : c.second->buildings)
		{
			listbox->addItem(createMapRowBuilding({state.get(), b.first}, state));
		}
	}
}

MapSelector::~MapSelector() = default;

sp<Control> MapSelector::createMapRowBuilding(StateRef<Building> building, sp<GameState> state)
{
	auto control = mksp<Control>();

	const int HEIGHT = 21;

	auto text = control->createChild<Label>(
	    format("[%s Building] %s [%s]", building->owner == state->getAliens() ? "Alien" : "Human",
	           building->name, building->battle_map.id),
	    ui().getFont("smalfont"));
	text->Location = {0, 0};
	text->Size = {488, HEIGHT};
	text->TextVAlign = VerticalAlignment::Centre;

	{
		auto btnImage = fw().data->loadImage(
		    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:57:ui/menuopt.pal");
		auto btnLocation = control->createChild<GraphicButton>(btnImage, btnImage);
		btnLocation->Location = text->Location + Vec2<int>{text->Size.x, 0};
		btnLocation->Size = {22, HEIGHT};
		btnLocation->addCallback(FormEventType::ButtonClick,
		                         [building, state, this](Event *)
		                         {
			                         skirmish.setLocation(building);
			                         fw().stageQueueCommand({StageCmd::Command::POP});
		                         });
	}

	control->Size.y = HEIGHT;
	return control;
}

sp<Control> MapSelector::createMapRowVehicle(StateRef<VehicleType> vehicle, sp<GameState> state)
{
	auto control = mksp<Control>();

	const int HEIGHT = 21;

	auto text = control->createChild<Label>(
	    format("[UFO] %s [%s]", vehicle->name, vehicle->battle_map.id), ui().getFont("smalfont"));
	text->Location = {0, 0};
	text->Size = {488, HEIGHT};
	text->TextVAlign = VerticalAlignment::Centre;

	{
		auto btnImage = fw().data->loadImage(
		    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:57:ui/menuopt.pal");
		auto btnLocation = control->createChild<GraphicButton>(btnImage, btnImage);
		btnLocation->Location = text->Location + Vec2<int>{text->Size.x, 0};
		btnLocation->Size = {22, HEIGHT};
		btnLocation->addCallback(FormEventType::ButtonClick,
		                         [vehicle, state, this](Event *)
		                         {
			                         skirmish.setLocation(vehicle);
			                         fw().stageQueueCommand({StageCmd::Command::POP});
		                         });
	}

	control->Size.y = HEIGHT;
	return control;
}

sp<Control> MapSelector::createMapRowBase(StateRef<Base> base, sp<GameState> state)
{
	auto control = mksp<Control>();

	const int HEIGHT = 21;

	auto text =
	    control->createChild<Label>(format("[Base] %s", base->name), ui().getFont("smalfont"));
	text->Location = {0, 0};
	text->Size = {488, HEIGHT};
	text->TextVAlign = VerticalAlignment::Centre;

	{
		auto btnImage = fw().data->loadImage(
		    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:57:ui/menuopt.pal");
		auto btnLocation = control->createChild<GraphicButton>(btnImage, btnImage);
		btnLocation->Location = text->Location + Vec2<int>{text->Size.x, 0};
		btnLocation->Size = {22, HEIGHT};
		btnLocation->addCallback(FormEventType::ButtonClick,
		                         [base, state, this](Event *)
		                         {
			                         skirmish.setLocation(base);
			                         fw().stageQueueCommand({StageCmd::Command::POP});
		                         });
	}

	control->Size.y = HEIGHT;
	return control;
}

void MapSelector::begin() {}

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

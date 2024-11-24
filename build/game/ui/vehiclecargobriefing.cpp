#include "game/ui/vehiclecargobriefing.h"
#include "forms/form.h"
#include "forms/ui.h"
#include <SDL_keycode.h>
#include <framework/event.h>
#include <framework/framework.h>
#include <forms/listbox.h>
#include <forms/scrollbar.h>

namespace OpenApoc
{
// VEHICLE_CARGO_BRIEFING

Vehiclecargobriefing::Vehiclecargobriefing(sp<Vehicle> vehicle)
    : Stage(), menuform(ui().getForm("city/vehicle_cargo")), state(state)
{
	LogWarning("CargoClass..");

	//auto listbox = menuform->findControlTyped<ListBox>("LISTBOX_CARGO");
	//for (auto cargo : vehicle->missions)
	//{
	//	
	//}
	this->update();
	//listbox->scroller->scrollMax();
}

Vehiclecargobriefing::Vehiclecargobriefing(sp<GameState> state,
                                           sp<Vehicle> vehicle)
    : Stage(), menuform(ui().getForm("city/vehicle_cargo.form")), state(state)
{
	// auto listbox = menuform->findControlTyped<ListBox>("LISTBOX_MESSAGES");
	// for (EventMessage message : state->messages)
	//{
	//	listbox->addItem(createMessageRow(message, state, cityView));
	// }
	// this->update();
	// listbox->scroller->scrollMax();
}

Vehiclecargobriefing::~Vehiclecargobriefing() = default;

// sp<Control> Vehiclecargobriefing::createCargoRow(sp<GameState> state, CityView &cityView) {}

// Stage control
void Vehiclecargobriefing::begin() 
{

}
void Vehiclecargobriefing::pause() {}
void Vehiclecargobriefing::resume() {}
void Vehiclecargobriefing::finish() {}
void Vehiclecargobriefing::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_MOUSE_DOWN)
	{
		LogWarning("Briefing");
		fw().stageQueueCommand({StageCmd::Command::POP});
		return;

		//if (e->keyboard().KeyCode == SDLK_i)
		//{
		//	menuform->findControl("BUTTON_OK")->click();
		//	return;
		//}
	}
	return;
}
void Vehiclecargobriefing::update() { menuform->update(); }
void Vehiclecargobriefing::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}
bool Vehiclecargobriefing::isTransition() { return false; }

}; // namespace OpenApoc
#include "game/ui/city/buildingscreen.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/city/building.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

BuildingScreen::BuildingScreen(sp<GameState> state, sp<Building> building)
    : Stage(), menuform(ui().getForm("FORM_BUILDING_SCREEN")), state(state), building(building)
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	menuform->findControlTyped<Label>("TEXT_BUILDING_NAME")->setText(tr(building->name));
	menuform->findControlTyped<Label>("TEXT_OWNER_NAME")->setText(tr(building->owner->name));
	menuform->findControlTyped<Label>("TEXT_BUILDING_FUNCTION")->setText(tr(building->function));
}

BuildingScreen::~BuildingScreen() = default;

void BuildingScreen::begin() {}

void BuildingScreen::pause() {}

void BuildingScreen::resume() {}

void BuildingScreen::finish() {}

void BuildingScreen::eventOccurred(Event *e)
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
		if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			fw().stageQueueCommand({StageCmd::Command::POP});
			return;
		}
	}
}

void BuildingScreen::update() { menuform->update(); }

void BuildingScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	fw().renderer->drawFilledRect({0, 0}, fw().displayGetSize(), Colour{0, 0, 0, 128});
	menuform->render();
}

bool BuildingScreen::isTransition() { return false; }

}; // namespace OpenApoc

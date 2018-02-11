#include "game/ui/city/scorescreen.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

ScoreScreen::ScoreScreen(sp<GameState> state)
    : Stage(), menuform(ui().getForm("city/score")), state(state)
{
}

ScoreScreen::~ScoreScreen() = default;

void ScoreScreen::begin()
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
	menuform->findControlTyped<Label>("TEXT_DATE")->setText(state->gameTime.getLongDateString());
	menuform->findControlTyped<Label>("TEXT_WEEK")->setText(state->gameTime.getWeekString());
}

void ScoreScreen::pause() {}

void ScoreScreen::resume() {}

void ScoreScreen::finish() {}

void ScoreScreen::eventOccurred(Event *e)
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

void ScoreScreen::update() { menuform->update(); }

void ScoreScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool ScoreScreen::isTransition() { return false; }

}; // namespace OpenApoc

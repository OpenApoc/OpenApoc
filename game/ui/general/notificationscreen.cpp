#include "game/ui/general/notificationscreen.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/ui/city/cityview.h"

namespace OpenApoc
{

NotificationScreen::NotificationScreen(sp<GameState> state, CityView &cityView,
                                       const UString &message)
    : Stage(), menuform(ui().getForm("FORM_NOTIFICATION_SCREEN")), state(state)
{
	menuform->findControlTyped<Label>("TEXT_NOTIFICATION")->setText(message);

	menuform->findControl("BUTTON_RESUME")
	    ->addCallback(FormEventType::ButtonClick,
	                  [this](Event *) { this->stageCmd.cmd = StageCmd::Command::POP; });
	menuform->findControl("BUTTON_PAUSE")
	    ->addCallback(FormEventType::ButtonClick, [this, &cityView](Event *) {
		    cityView.zoomLastEvent();
		    cityView.setUpdateSpeed(UpdateSpeed::Pause);
		    this->stageCmd.cmd = StageCmd::Command::POP;
		});
}

NotificationScreen::~NotificationScreen() = default;

void NotificationScreen::begin() {}

void NotificationScreen::pause() {}

void NotificationScreen::resume() {}

void NotificationScreen::finish() {}

void NotificationScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}
}

void NotificationScreen::update(StageCmd *const cmd)
{
	menuform->update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void NotificationScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	fw().renderer->drawFilledRect({0, 0}, fw().displayGetSize(), Colour{0, 0, 0, 128});
	menuform->render();
}

bool NotificationScreen::isTransition() { return false; }

}; // namespace OpenApoc

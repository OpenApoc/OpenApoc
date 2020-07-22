#include "game/ui/general/notificationscreen.h"
#include "forms/checkbox.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/ui/tileview/battleview.h"
#include "game/ui/tileview/cityview.h"

namespace OpenApoc
{

NotificationScreen::NotificationScreen(sp<GameState> state, CityView &cityView,
                                       const UString &message, GameEventType eventType)
    : Stage(), menuform(ui().getForm("notification")), eventType(eventType), state(state)
{
	menuform->findControlTyped<Label>("TEXT_NOTIFICATION")->setText(message);

	menuform->findControl("BUTTON_PAUSE")
	    ->addCallback(FormEventType::ButtonClick, [&cityView](Event *) {
		    cityView.zoomLastEvent();
		    cityView.setUpdateSpeed(CityUpdateSpeed::Pause);
		    fw().stageQueueCommand({StageCmd::Command::POP});
	    });
}

NotificationScreen::NotificationScreen(sp<GameState> state, BattleView &battleView,
                                       const UString &message, GameEventType eventType)
    : Stage(), menuform(ui().getForm("notification")), eventType(eventType), state(state)
{
	menuform->findControlTyped<Label>("TEXT_NOTIFICATION")->setText(message);

	menuform->findControl("BUTTON_PAUSE")
	    ->addCallback(FormEventType::ButtonClick, [&battleView](Event *) {
		    battleView.zoomLastEvent();
		    battleView.setUpdateSpeed(BattleUpdateSpeed::Pause);
		    fw().stageQueueCommand({StageCmd::Command::POP});
	    });
}

NotificationScreen::~NotificationScreen() = default;

void NotificationScreen::begin()
{
	menuform->findControl("BUTTON_RESUME")->addCallback(FormEventType::ButtonClick, [](Event *) {
		fw().stageQueueCommand({StageCmd::Command::POP});
	});

	menuform->findControlTyped<CheckBox>("CHECKBOX_ALWAYS_PAUSE")->setChecked(true);
}

void NotificationScreen::pause() {}

void NotificationScreen::resume()
{
	if (!config().getBool(GameEvent::optionsMap.at(eventType)))
	{
		fw().stageQueueCommand({StageCmd::Command::POP});
		menuform->findControlTyped<CheckBox>("CHECKBOX_ALWAYS_PAUSE")->setChecked(false);
	}
}

void NotificationScreen::finish()
{
	if (GameEvent::optionsMap.find(eventType) != GameEvent::optionsMap.end())
	{
		config().set(GameEvent::optionsMap.at(eventType),
		             menuform->findControlTyped<CheckBox>("CHECKBOX_ALWAYS_PAUSE")->isChecked());
	}
}

void NotificationScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			menuform->findControl("BUTTON_RESUME")->click();
			return;
		}
		if (e->keyboard().KeyCode == SDLK_SPACE)
		{
			menuform->findControl("CHECKBOX_ALWAYS_PAUSE")->click();
			return;
		}
		if (e->keyboard().KeyCode == SDLK_RETURN || e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_PAUSE")->click();
			return;
		}
	}
}

void NotificationScreen::update() { menuform->update(); }

void NotificationScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool NotificationScreen::isTransition() { return false; }

}; // namespace OpenApoc

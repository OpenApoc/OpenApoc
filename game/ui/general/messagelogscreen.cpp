#include "game/ui/general/messagelogscreen.h"
#include "forms/form.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/listbox.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "game/state/message.h"
#include "game/ui/tileview/battleview.h"
#include "game/ui/tileview/cityview.h"

namespace OpenApoc
{

MessageLogScreen::MessageLogScreen(sp<GameState> state, CityView &cityView)
    : Stage(), menuform(ui().getForm("messagelog")), state(state)
{
	auto listbox = menuform->findControlTyped<ListBox>("LISTBOX_MESSAGES");
	for (EventMessage message : state->messages)
	{
		listbox->addItem(createMessageRow(message, state, cityView));
	}
}

MessageLogScreen::MessageLogScreen(sp<GameState> state, BattleView &battleView)
    : Stage(), menuform(ui().getForm("messagelog")), state(state)
{
	auto listbox = menuform->findControlTyped<ListBox>("LISTBOX_MESSAGES");
	for (EventMessage message : state->messages)
	{
		listbox->addItem(createMessageRow(message, state, battleView));
	}
}

MessageLogScreen::~MessageLogScreen() = default;

sp<Control> MessageLogScreen::createMessageRow(EventMessage message, sp<GameState> state,
                                               CityView &cityView)
{
	return createMessageRow(message, state, [message, state, &cityView](Event *) {
		cityView.setScreenCenterTile(message.location);
		fw().stageQueueCommand({StageCmd::Command::POP});
	});
}

sp<Control> MessageLogScreen::createMessageRow(EventMessage message, sp<GameState> state,
                                               BattleView &battleView)
{
	return createMessageRow(message, state, [message, state, &battleView](Event *) {
		battleView.setScreenCenterTile(message.location);
		fw().stageQueueCommand({StageCmd::Command::POP});
	});
}

sp<Control> MessageLogScreen::createMessageRow(EventMessage message,
                                               sp<GameState> state [[maybe_unused]],
                                               std::function<void(FormsEvent *e)> callback)
{
	auto control = mksp<Control>();

	const int HEIGHT = 21;

	auto date =
	    control->createChild<Label>(message.time.getShortDateString(), ui().getFont("smalfont"));
	date->Location = {0, 0};
	date->Size = {100, HEIGHT};
	date->TextVAlign = VerticalAlignment::Centre;

	auto time =
	    control->createChild<Label>(message.time.getLongTimeString(), ui().getFont("smalfont"));
	time->Location = date->Location + Vec2<int>{date->Size.x, 0};
	time->Size = {60, HEIGHT};
	time->TextVAlign = VerticalAlignment::Centre;

	auto text = control->createChild<Label>(message.text, ui().getFont("smalfont"));
	text->Location = time->Location + Vec2<int>{time->Size.x, 0};
	text->Size = {328, HEIGHT};
	text->TextVAlign = VerticalAlignment::Centre;

	if (message.location != EventMessage::NO_LOCATION)
	{
		auto btnImage = fw().data->loadImage(
		    "PCK:xcom3/ufodata/newbut.pck:xcom3/ufodata/newbut.tab:57:ui/menuopt.pal");
		auto btnLocation = control->createChild<GraphicButton>(btnImage, btnImage);
		btnLocation->Location = text->Location + Vec2<int>{text->Size.x, 0};
		btnLocation->Size = {22, HEIGHT};
		btnLocation->addCallback(FormEventType::ButtonClick, callback);
	}

	return control;
}

void MessageLogScreen::begin()
{
	menuform->findControlTyped<Label>("TEXT_FUNDS")->setText(state->getPlayerBalance());
}

void MessageLogScreen::pause() {}

void MessageLogScreen::resume() {}

void MessageLogScreen::finish() {}

void MessageLogScreen::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_OK")->click();
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

void MessageLogScreen::update() { menuform->update(); }

void MessageLogScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool MessageLogScreen::isTransition() { return false; }

}; // namespace OpenApoc

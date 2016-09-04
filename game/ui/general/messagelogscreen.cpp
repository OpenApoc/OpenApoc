#include "game/ui/general/messagelogscreen.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/ui/city/cityview.h"

namespace OpenApoc
{

MessageLogScreen::MessageLogScreen(sp<GameState> state, CityView &cityView)
    : Stage(), menuform(ui().getForm("FORM_MESSAGE_LOG_SCREEN")), state(state)
{
	auto listbox = menuform->findControlTyped<ListBox>("LISTBOX_MESSAGES");
	for (EventMessage message : state->messages)
	{
		listbox->addItem(createMessageRow(message, state, cityView));
	}
}

MessageLogScreen::~MessageLogScreen() = default;

sp<Control> MessageLogScreen::createMessageRow(EventMessage message, sp<GameState> state,
                                               CityView &cityView)
{
	auto control = mksp<Control>();

	const int HEIGHT = 21;

	auto date =
	    control->createChild<Label>(message.time.getShortDateString(), ui().getFont("SMALFONT"));
	date->Location = {0, 0};
	date->Size = {100, HEIGHT};
	date->TextVAlign = VerticalAlignment::Centre;

	auto time = control->createChild<Label>(message.time.getTimeString(), ui().getFont("SMALFONT"));
	time->Location = date->Location + Vec2<int>{date->Size.x, 0};
	time->Size = {60, HEIGHT};
	time->TextVAlign = VerticalAlignment::Centre;

	auto text = control->createChild<Label>(message.text, ui().getFont("SMALFONT"));
	text->Location = time->Location + Vec2<int>{time->Size.x, 0};
	text->Size = {328, HEIGHT};
	text->TextVAlign = VerticalAlignment::Centre;

	if (message.getMapLocation(*state) != EventMessage::NO_LOCATION)
	{
		auto btnImage = fw().data->loadImage(
		    "PCK:XCOM3/UFODATA/NEWBUT.PCK:XCOM3/UFODATA/NEWBUT.TAB:57:UI/menuopt.pal");
		auto btnLocation = control->createChild<GraphicButton>(btnImage, btnImage);
		btnLocation->Location = text->Location + Vec2<int>{text->Size.x, 0};
		btnLocation->Size = {22, HEIGHT};
		btnLocation->addCallback(FormEventType::ButtonClick, [message, state, &cityView](Event *) {
			cityView.setScreenCenterTile(message.getMapLocation(*state));
			fw().stageQueueCommand({StageCmd::Command::POP});
		});
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

void MessageLogScreen::update() { menuform->update(); }

void MessageLogScreen::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	fw().renderer->drawFilledRect({0, 0}, fw().displayGetSize(), Colour{0, 0, 0, 128});
	menuform->render();
}

bool MessageLogScreen::isTransition() { return false; }

}; // namespace OpenApoc

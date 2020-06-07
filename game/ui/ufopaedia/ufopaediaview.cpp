#include "game/ui/ufopaedia/ufopaediaview.h"
#include "forms/form.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "game/state/gamestate.h"
#include "game/ui/ufopaedia/ufopaediacategoryview.h"
#include "library/sp.h"

namespace OpenApoc
{

UfopaediaView::UfopaediaView(sp<GameState> state)
    : Stage(), menuform(ui().getForm("ufopaediatitle")), state(state)
{
}

UfopaediaView::~UfopaediaView() = default;

void UfopaediaView::begin() {}

void UfopaediaView::pause() {}

void UfopaediaView::resume() {}

void UfopaediaView::finish() {}

void UfopaediaView::eventOccurred(Event *e)
{
	menuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE || e->keyboard().KeyCode == SDLK_RETURN ||
		    e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			menuform->findControl("BUTTON_QUIT")->click();
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

		if (e->forms().RaisedBy->Name.substr(0, 7) == "BUTTON_")
		{
			for (auto &cat : state->ufopaedia)
			{
				auto catName = cat.first;
				UString butName = "BUTTON_" + catName;
				if (butName == e->forms().RaisedBy->Name)
				{
					fw().stageQueueCommand(
					    {StageCmd::Command::PUSH, mksp<UfopaediaCategoryView>(state, cat.second)});
					LogInfo("Clicked category \"%s\"", catName);
					return;
				}
			}
		}
	}
}

void UfopaediaView::update() { menuform->update(); }

void UfopaediaView::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	menuform->render();
}

bool UfopaediaView::isTransition() { return false; }

}; // namespace OpenApoc

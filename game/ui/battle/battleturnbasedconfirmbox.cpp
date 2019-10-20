#include "game/ui/battle/battleturnbasedconfirmbox.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/textbutton.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/renderer.h"
#include "library/strings_format.h"

namespace OpenApoc
{
BattleTurnBasedConfirmBox::BattleTurnBasedConfirmBox(const UString &text,
                                                     std::function<void()> callbackYes,
                                                     std::function<void()> callbackNo)
    : Stage(), form(ui().getForm("battle/battle_tb_plan_popup")), callbackYes(callbackYes),
      callbackNo(callbackNo)
{
	form->findControlTyped<Label>("MESSAGE")->setText(text);
}

BattleTurnBasedConfirmBox::~BattleTurnBasedConfirmBox() = default;

void BattleTurnBasedConfirmBox::begin() {}

void BattleTurnBasedConfirmBox::pause() {}

void BattleTurnBasedConfirmBox::resume() {}

void BattleTurnBasedConfirmBox::finish() {}

void BattleTurnBasedConfirmBox::eventOccurred(Event *e)
{
	form->eventOccured(e);

	if (e->type() == EVENT_FORM_INTERACTION)
	{
		if (e->forms().EventFlag == FormEventType::ButtonClick)
		{
			if (e->forms().RaisedBy->Name == "BUTTON_OK")
			{
				fw().stageQueueCommand({StageCmd::Command::POP});
				if (callbackYes)
					callbackYes();
				return;
			}
			else if (e->forms().RaisedBy->Name == "BUTTON_CANCEL")
			{
				fw().stageQueueCommand({StageCmd::Command::POP});
				if (callbackNo)
					callbackNo();
				return;
			}
		}
	}
}

void BattleTurnBasedConfirmBox::update() { form->update(); }

void BattleTurnBasedConfirmBox::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	form->render();
	fw().renderer->drawRect(form->Location, form->Size, Colour{48, 48, 52});
	fw().renderer->drawRect(form->Location + 2, form->Size - 2, Colour{96, 100, 104});
	fw().renderer->drawRect(form->Location + 1, form->Size - 2, Colour{236, 236, 236});
}

bool BattleTurnBasedConfirmBox::isTransition() { return false; }

}; // namespace OpenApoc

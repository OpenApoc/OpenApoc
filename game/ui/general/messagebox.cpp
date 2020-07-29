#include "game/ui/general/messagebox.h"
#include "forms/form.h"
#include "forms/label.h"
#include "forms/textbutton.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "library/strings_format.h"

namespace OpenApoc
{

MessageBox::MessageBox(const UString &title, const UString &text, ButtonOptions buttons,
                       std::function<void()> callbackYes, std::function<void()> callbackNo,
                       std::function<void()> callbackCancel)
    : Stage(), callbackYes(callbackYes), callbackNo(callbackNo), callbackCancel(callbackCancel)
{
	form = mksp<Form>();
	form->Size = {248, 100};
	form->BackgroundColour = {148, 148, 148, 255};

	const int MARGIN = 8;
	const Vec2<int> BUTTON_SIZE = {100, 28};
	const Vec2<int> BUTTON_SIZE_2 = {70, 28};

	auto lTitle = form->createChild<Label>(to_upper(title), ui().getFont("smalfont"));
	lTitle->Size.x = form->Size.x - MARGIN * 2;
	lTitle->Size.y = ui().getFont("smalfont")->getFontHeight();
	lTitle->Location = {MARGIN, MARGIN};
	lTitle->TextHAlign = HorizontalAlignment::Centre;

	auto lText = form->createChild<Label>(text, ui().getFont("smalfont"));
	lText->Size.x = form->Size.x - MARGIN * 2;
	lText->Size.y = ui().getFont("smalfont")->getFontHeight(text, lText->Size.x);
	lText->Location = lTitle->Location;
	lText->Location.y += lTitle->Size.y + MARGIN * 2;
	lText->TextHAlign = HorizontalAlignment::Centre;

	switch (buttons)
	{
		case ButtonOptions::Ok:
		{
			auto bOk = form->createChild<TextButton>(tr("OK"), ui().getFont("smallset"));
			bOk->Name = "BUTTON_OK";
			bOk->Size = BUTTON_SIZE;
			bOk->RenderStyle = TextButton::ButtonRenderStyle::Bevel;
			bOk->Location.x = (form->Size.x - bOk->Size.x) / 2;
			bOk->Location.y = lText->Location.y + lText->Size.y + MARGIN;

			form->Size.y = bOk->Location.y + bOk->Size.y + MARGIN;
			break;
		}
		case ButtonOptions::YesNo:
		{
			auto bYes = form->createChild<TextButton>(tr("Yes"), ui().getFont("smallset"));
			bYes->Name = "BUTTON_YES";
			bYes->Size = BUTTON_SIZE;
			bYes->RenderStyle = TextButton::ButtonRenderStyle::Bevel;
			bYes->Location.x = MARGIN;
			bYes->Location.y = lText->Location.y + lText->Size.y + MARGIN;

			auto bNo = form->createChild<TextButton>(tr("No"), ui().getFont("smallset"));
			bNo->Name = "BUTTON_NO";
			bNo->Size = BUTTON_SIZE;
			bNo->RenderStyle = TextButton::ButtonRenderStyle::Bevel;
			bNo->Location.x = form->Size.x - bNo->Size.x - MARGIN;
			bNo->Location.y = lText->Location.y + lText->Size.y + MARGIN;

			form->Size.y = bYes->Location.y + bYes->Size.y + MARGIN;
			break;
		}
		case ButtonOptions::YesNoCancel:
		{
			auto bYes = form->createChild<TextButton>(tr("Yes"), ui().getFont("smallset"));
			bYes->Name = "BUTTON_YES";
			bYes->Size = BUTTON_SIZE_2;
			bYes->RenderStyle = TextButton::ButtonRenderStyle::Bevel;
			bYes->Location.x = MARGIN;
			bYes->Location.y = lText->Location.y + lText->Size.y + MARGIN;

			auto bNo = form->createChild<TextButton>(tr("No"), ui().getFont("smallset"));
			bNo->Name = "BUTTON_NO2";
			bNo->Size = BUTTON_SIZE_2;
			bNo->RenderStyle = TextButton::ButtonRenderStyle::Bevel;
			bNo->Location.x = form->Size.x / 2 - bNo->Size.x / 2;
			bNo->Location.y = lText->Location.y + lText->Size.y + MARGIN;

			auto bCan = form->createChild<TextButton>(tr("Cancel"), ui().getFont("smallset"));
			bCan->Name = "BUTTON_CANCEL";
			bCan->Size = BUTTON_SIZE_2;
			bCan->RenderStyle = TextButton::ButtonRenderStyle::Bevel;
			bCan->Location.x = form->Size.x - bCan->Size.x - MARGIN;
			bCan->Location.y = lText->Location.y + lText->Size.y + MARGIN;

			form->Size.y = bYes->Location.y + bYes->Size.y + MARGIN;
			break;
		}
	}

	form->align(HorizontalAlignment::Centre, VerticalAlignment::Centre);
}

MessageBox::~MessageBox() = default;

void MessageBox::begin() {}

void MessageBox::pause() {}

void MessageBox::resume() {}

void MessageBox::finish() {}

void MessageBox::eventOccurred(Event *e)
{
	form->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			if (form->findControl("BUTTON_CANCEL"))
			{
				form->findControl("BUTTON_CANCEL")->click();
			}
			if (form->findControl("BUTTON_NO"))
			{
				form->findControl("BUTTON_NO")->click();
			}
			return;
		}
		if (e->keyboard().KeyCode == SDLK_RETURN || e->keyboard().KeyCode == SDLK_KP_ENTER)
		{
			if (form->findControl("BUTTON_OK"))
			{
				form->findControl("BUTTON_OK")->click();
			}
			if (form->findControl("BUTTON_YES"))
			{
				form->findControl("BUTTON_YES")->click();
			}
			return;
		}
		if (e->keyboard().KeyCode == SDLK_SPACE)
		{
			if (form->findControl("BUTTON_NO2"))
			{
				form->findControl("BUTTON_NO2")->click();
			}
			if (form->findControl("BUTTON_OK"))
			{
				form->findControl("BUTTON_OK")->click();
			}
			if (form->findControl("BUTTON_YES"))
			{
				form->findControl("BUTTON_YES")->click();
			}
			return;
		}
	}
	if (e->type() == EVENT_FORM_INTERACTION)
	{
		if (e->forms().EventFlag == FormEventType::ButtonClick)
		{
			if (e->forms().RaisedBy->Name == "BUTTON_OK" ||
			    e->forms().RaisedBy->Name == "BUTTON_YES")
			{
				fw().stageQueueCommand({StageCmd::Command::POP});
				if (callbackYes)
					callbackYes();
				return;
			}
			else if (e->forms().RaisedBy->Name == "BUTTON_NO2" ||
			         e->forms().RaisedBy->Name == "BUTTON_NO")
			{
				fw().stageQueueCommand({StageCmd::Command::POP});
				if (callbackNo)
					callbackNo();
				return;
			}
			else if (e->forms().RaisedBy->Name == "BUTTON_CANCEL")
			{
				fw().stageQueueCommand({StageCmd::Command::POP});
				if (callbackCancel)
					callbackCancel();
				return;
			}
		}
	}
}

void MessageBox::update() { form->update(); }

void MessageBox::render()
{
	fw().stageGetPrevious(this->shared_from_this())->render();
	form->render();
	fw().renderer->drawRect(form->Location, form->Size, Colour{48, 48, 52});
	fw().renderer->drawRect(form->Location + 2, form->Size - 2, Colour{96, 100, 104});
	fw().renderer->drawRect(form->Location + 1, form->Size - 2, Colour{236, 236, 236});
}

bool MessageBox::isTransition() { return false; }

}; // namespace OpenApoc

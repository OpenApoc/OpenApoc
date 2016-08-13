#include "game/ui/general/messagebox.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"

namespace OpenApoc
{

MessageBox::MessageBox(const UString &title, const UString &text, ButtonOptions buttons,
                       std::function<void()> callbackYes, std::function<void()> callbackNo)
    : Stage(), callbackYes(callbackYes), callbackNo(callbackNo)
{
	form = mksp<Form>();
	form->Size = {248, 100};
	form->BackgroundColour = {148, 148, 148, 255};

	const int MARGIN = 8;
	const Vec2<int> BUTTON_SIZE = {100, 28};

	auto lTitle = form->createChild<Label>(title.toUpper(), ui().GetFont("SMALFONT"));
	lTitle->Size.x = form->Size.x - MARGIN * 2;
	lTitle->Size.y = ui().GetFont("SMALFONT")->GetFontHeight();
	lTitle->Location = {MARGIN, MARGIN};
	lTitle->TextHAlign = HorizontalAlignment::Centre;

	auto lText = form->createChild<Label>(text, ui().GetFont("SMALFONT"));
	lText->Size.x = form->Size.x - MARGIN * 2;
	lText->Size.y = ui().GetFont("SMALFONT")->GetFontHeight(text, lText->Size.x);
	lText->Location = lTitle->Location;
	lText->Location.y += lTitle->Size.y + MARGIN * 2;
	lText->TextHAlign = HorizontalAlignment::Centre;

	if (buttons == ButtonOptions::Ok)
	{
		auto bOk = form->createChild<TextButton>(tr("OK"), ui().GetFont("SMALLSET"));
		bOk->Name = "BUTTON_OK";
		bOk->Size = BUTTON_SIZE;
		bOk->RenderStyle = TextButton::ButtonRenderStyle::Bevel;
		bOk->Location.x = (form->Size.x - bOk->Size.x) / 2;
		bOk->Location.y = lText->Location.y + lText->Size.y + MARGIN;

		form->Size.y = bOk->Location.y + bOk->Size.y + MARGIN;
	}
	else if (buttons == ButtonOptions::YesNo)
	{
		auto bYes = form->createChild<TextButton>(tr("Yes"), ui().GetFont("SMALLSET"));
		bYes->Name = "BUTTON_YES";
		bYes->Size = BUTTON_SIZE;
		bYes->RenderStyle = TextButton::ButtonRenderStyle::Bevel;
		bYes->Location.x = MARGIN;
		bYes->Location.y = lText->Location.y + lText->Size.y + MARGIN;

		auto bNo = form->createChild<TextButton>(tr("No"), ui().GetFont("SMALLSET"));
		bNo->Name = "BUTTON_NO";
		bNo->Size = BUTTON_SIZE;
		bNo->RenderStyle = TextButton::ButtonRenderStyle::Bevel;
		bNo->Location.x = form->Size.x - bNo->Size.x - MARGIN;
		bNo->Location.y = lText->Location.y + lText->Size.y + MARGIN;

		form->Size.y = bYes->Location.y + bYes->Size.y + MARGIN;
	}

	form->Align(HorizontalAlignment::Centre, VerticalAlignment::Centre);
}

MessageBox::~MessageBox() {}

void MessageBox::Begin() {}

void MessageBox::Pause() {}

void MessageBox::Resume() {}

void MessageBox::Finish() {}

void MessageBox::EventOccurred(Event *e)
{
	form->EventOccured(e);

	if (e->Type() == EVENT_FORM_INTERACTION)
	{
		if (e->Forms().EventFlag == FormEventType::ButtonClick)
		{
			if (e->Forms().RaisedBy->Name == "BUTTON_OK" ||
			    e->Forms().RaisedBy->Name == "BUTTON_YES")
			{
				stageCmd.cmd = StageCmd::Command::POP;
				if (callbackYes)
					callbackYes();
				return;
			}
			else if (e->Forms().RaisedBy->Name == "BUTTON_CANCEL" ||
			         e->Forms().RaisedBy->Name == "BUTTON_NO")
			{
				stageCmd.cmd = StageCmd::Command::POP;
				if (callbackNo)
					callbackNo();
				return;
			}
		}
	}
}

void MessageBox::Update(StageCmd *const cmd)
{
	form->Update();
	*cmd = this->stageCmd;
	this->stageCmd = StageCmd();
}

void MessageBox::Render()
{
	fw().Stage_GetPrevious(this->shared_from_this())->Render();
	form->Render();
	fw().renderer->drawRect(form->Location, form->Size, Colour{48, 48, 52});
	fw().renderer->drawRect(form->Location + 2, form->Size - 2, Colour{96, 100, 104});
	fw().renderer->drawRect(form->Location + 1, form->Size - 2, Colour{236, 236, 236});
}

bool MessageBox::IsTransition() { return false; }

}; // namespace OpenApoc

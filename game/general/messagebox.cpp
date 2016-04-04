#include "game/general/messagebox.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"

namespace OpenApoc
{

MessageBox::MessageBox(const UString &title, const UString &text, ButtonOptions buttons) : Stage()
{
	form = mksp<Form>();
	form->Size = {248, 100};
	form->Location.x = (fw().Display_GetWidth() / 2) - (form->Size.x / 2);
	form->Location.y = (fw().Display_GetHeight() / 2) - (form->Size.y / 2);
	form->BackgroundColour = {148, 148, 148, 255};

	const int MARGIN = 8;

	auto lTitle = form->createChild<Label>(tr(title).toUpper(), fw().gamecore->GetFont("SMALFONT"));
	lTitle->Size.x = form->Size.x - MARGIN * 2;
	lTitle->Size.y = fw().gamecore->GetFont("SMALFONT")->GetFontHeight();
	lTitle->Location = {MARGIN, MARGIN};
	lTitle->TextHAlign = HorizontalAlignment::Centre;

	auto lText = form->createChild<Label>(tr(text), fw().gamecore->GetFont("SMALFONT"));
	lText->Size.x = form->Size.x - MARGIN * 2;
	lText->Size.y = fw().gamecore->GetFont("SMALFONT")->GetFontHeight();
	lText->Location = lTitle->Location;
	lText->Location.y += lTitle->Size.y + MARGIN;
	lText->TextHAlign = HorizontalAlignment::Centre;

	if (buttons == ButtonOptions::Ok)
	{
		auto bOk = form->createChild<TextButton>(tr("OK"), fw().gamecore->GetFont("SMALLSET"));
		bOk->Name = "BUTTON_OK";
		bOk->Size = {100, 28};
		bOk->RenderStyle = TextButton::ButtonRenderStyle::Bevel;
		bOk->Location.x = (form->Size.x - bOk->Size.x) / 2;
		bOk->Location.y = form->Size.y - bOk->Size.y - MARGIN;
	}
	else if (buttons == ButtonOptions::YesNo)
	{
		auto bYes = form->createChild<TextButton>(tr("Yes"), fw().gamecore->GetFont("SMALLSET"));
		bYes->Name = "BUTTON_YES";
		bYes->Size = {100, 28};
		bYes->RenderStyle = TextButton::ButtonRenderStyle::Bevel;
		bYes->Location.x = MARGIN;
		bYes->Location.y = form->Size.y - bYes->Size.y - MARGIN;

		auto bNo = form->createChild<TextButton>(tr("No"), fw().gamecore->GetFont("SMALLSET"));
		bNo->Name = "BUTTON_NO";
		bNo->Size = {100, 28};
		bNo->RenderStyle = TextButton::ButtonRenderStyle::Bevel;
		bNo->Location.x = form->Size.x - bNo->Size.x - MARGIN;
		bNo->Location.y = form->Size.y - bNo->Size.y - MARGIN;
	}
}

MessageBox::~MessageBox() {}

void MessageBox::Begin() {}

void MessageBox::Pause() {}

void MessageBox::Resume() {}

void MessageBox::Finish() {}

void MessageBox::EventOccurred(Event *e)
{
	form->EventOccured(e);
	fw().gamecore->MouseCursor->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION)
	{
		if (e->Forms().EventFlag == FormEventType::ButtonClick)
		{
			if (e->Forms().RaisedBy->Name == "BUTTON_OK")
			{
				stageCmd.cmd = StageCmd::Command::POP;
				return;
			}
			if (e->Forms().RaisedBy->Name == "BUTTON_YES")
			{
				stageCmd.cmd = StageCmd::Command::POP;
				return;
			}
			if (e->Forms().RaisedBy->Name == "BUTTON_NO")
			{
				stageCmd.cmd = StageCmd::Command::POP;
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
	fw().gamecore->MouseCursor->Render();
}

bool MessageBox::IsTransition() { return false; }

}; // namespace OpenApoc

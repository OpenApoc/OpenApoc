#include "library/sp.h"

#include "game/ufopaedia/ufopaedia.h"
#include "framework/framework.h"

namespace OpenApoc
{

std::vector<sp<UfopaediaCategory>> Ufopaedia::UfopaediaDB;

Ufopaedia::Ufopaedia(Framework &fw)
    : Stage(fw), menuform(fw.gamecore->GetForm("FORM_UFOPAEDIA_TITLE"))
{
}

Ufopaedia::~Ufopaedia() {}

void Ufopaedia::Begin() {}

void Ufopaedia::Pause() {}

void Ufopaedia::Resume() {}

void Ufopaedia::Finish() {}

void Ufopaedia::EventOccurred(Event *e)
{
	menuform->EventOccured(e);
	fw.gamecore->MouseCursor->EventOccured(e);

	if (e->Type == EVENT_KEY_DOWN)
	{
		if (e->Data.Keyboard.KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick)
	{
		if (e->Data.Forms.RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
		else if (e->Data.Forms.RaisedBy->Name.substr(0, 7) == "BUTTON_")
		{
			UString categoryname =
			    e->Data.Forms.RaisedBy->Name.substr(7, e->Data.Forms.RaisedBy->Name.length() - 7);
			std::string btnname = e->Data.Forms.RaisedBy->Name.str();

			for (auto dbcat = UfopaediaDB.begin(); dbcat != UfopaediaDB.end(); dbcat++)
			{
				sp<UfopaediaCategory> catrecord = *dbcat;
				if (catrecord->ID == categoryname)
				{
					stageCmd.cmd = StageCmd::Command::PUSH;
					stageCmd.nextStage = catrecord;
				}
			}

			return;
		}
	}
}

void Ufopaedia::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void Ufopaedia::Render()
{
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect({0, 0}, fw.Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool Ufopaedia::IsTransition() { return false; }

}; // namespace OpenApoc

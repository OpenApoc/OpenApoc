
#include "game/general/ingameoptions.h"
#include "framework/framework.h"

namespace OpenApoc {

InGameOptions::InGameOptions(Framework &fw)
	: Stage(fw)
{
	menuform = fw.gamecore->GetForm("FORM_INGAMEOPTIONS");
}

InGameOptions::~InGameOptions()
{
}

void InGameOptions::Begin()
{
}

void InGameOptions::Pause()
{
}

void InGameOptions::Resume()
{
}

void InGameOptions::Finish()
{
}

void InGameOptions::EventOccurred(Event *e)
{
	menuform->EventOccured( e );
	fw.gamecore->MouseCursor->EventOccured( e );

	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick )
	{
		if (e->Data.Forms.RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}
}

void InGameOptions::Update(StageCmd * const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	//Reset the command to default
	this->stageCmd = StageCmd();
}

void InGameOptions::Render()
{
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect({0,0}, fw.Display_GetSize(), Colour{0,0,0,128});
	menuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool InGameOptions::IsTransition()
{
	return false;
}

}; //namespace OpenApoc

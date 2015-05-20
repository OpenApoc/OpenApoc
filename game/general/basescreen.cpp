
#include "game/general/basescreen.h"
#include "framework/framework.h"


namespace OpenApoc {

BaseScreen::BaseScreen(Framework &fw)
	: Stage(fw)
{
		basescreenform = fw.gamecore->GetForm("FORM_BASESCREEN");
}

BaseScreen::~BaseScreen()
{
}

void BaseScreen::Begin()
{
}

void BaseScreen::Pause()
{
}

void BaseScreen::Resume()
{
}

void BaseScreen::Finish()
{
}

void BaseScreen::EventOccurred(Event *e)
{
		basescreenform->EventOccured( e );
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
			LogInfo("Button %s clicked", e->Data.Forms.RaisedBy->Name.str().c_str());
			return;
		}
}

void BaseScreen::Update(StageCmd * const cmd)
{
		basescreenform->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void BaseScreen::Render()
{
		basescreenform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool BaseScreen::IsTransition()
{
	return false;
}

}; //namespace OpenApoc

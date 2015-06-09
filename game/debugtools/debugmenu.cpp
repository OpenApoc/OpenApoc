
#include "game/debugtools/debugmenu.h"
#include "framework/framework.h"

namespace OpenApoc {

DebugMenu::DebugMenu(Framework &fw)
	: Stage(fw)
{
	menuform = fw.gamecore->GetForm("FORM_DEBUG_MENU");
}

DebugMenu::~DebugMenu()
{
}

void DebugMenu::Begin()
{
}

void DebugMenu::Pause()
{
}

void DebugMenu::Resume()
{
}

void DebugMenu::Finish()
{
}

void DebugMenu::EventOccurred(Event *e)
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
		if( e->Data.Forms.RaisedBy->Name == "BUTTON_QUIT" )
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
		else if( e->Data.Forms.RaisedBy->Name == "BUTTON_DUMPPCK" )
		{
			// TODO: Hardcoded PCK dumps
			return;
		}
	}
}

void DebugMenu::Update(StageCmd * const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	//Reset the command to default
	this->stageCmd = StageCmd();
}

void DebugMenu::Render()
{
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect( Vec2<float>(0,0), Vec2<float>(fw.Display_GetWidth(), fw.Display_GetHeight()), Colour(0,0,0,128) );
	menuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool DebugMenu::IsTransition()
{
	return false;
}

}; //namespace OpenApoc

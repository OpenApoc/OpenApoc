
#include "game/ufopaedia/ufopaedia.h"
#include "framework/framework.h"

namespace OpenApoc {

Ufopaedia::Ufopaedia(Framework &fw)
	: Stage(fw)
{
	menuform = fw.gamecore->GetForm("FORM_UFOPAEDIA_TITLE");
}

Ufopaedia::~Ufopaedia()
{
}

void Ufopaedia::Begin()
{
}

void Ufopaedia::Pause()
{
}

void Ufopaedia::Resume()
{
}

void Ufopaedia::Finish()
{
}

void Ufopaedia::EventOccurred(Event *e)
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
		if( e->Data.Forms.RaisedBy->Name == "BUTTON_TEST_XCOMBASE" )
		{
			return;
		}
	}
}

void Ufopaedia::Update(StageCmd * const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	//Reset the command to default
	this->stageCmd = StageCmd();
}

void Ufopaedia::Render()
{
	menuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool Ufopaedia::IsTransition()
{
	return false;
}

}; //namespace OpenApoc

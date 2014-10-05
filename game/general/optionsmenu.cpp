
#include "optionsmenu.h"
#include "basescreen.h"
#include "../../framework/framework.h"
#include "../../transitions/transitions.h"

namespace OpenApoc {

OptionsMenu::OptionsMenu()
{
	menuform = FRAMEWORK->gamecore->GetForm("FORM_OPTIONSMENU");
}

OptionsMenu::~OptionsMenu()
{
}

void OptionsMenu::Begin()
{
}

void OptionsMenu::Pause()
{
}

void OptionsMenu::Resume()
{
}

void OptionsMenu::Finish()
{
}

void OptionsMenu::EventOccurred(Event *e)
{
	menuform->EventOccured( e );
	FRAMEWORK->gamecore->MouseCursor->EventOccured( e );

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
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = std::make_shared<TransitionFadeAcross>(std::make_shared<BaseScreen>(), FRAMES_PER_SECOND / 4);
            return;
        }
    }
}

void OptionsMenu::Update(StageCmd * const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	//Reset the command to default
	this->stageCmd = StageCmd();
}

void OptionsMenu::Render()
{
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
	menuform->Render();
	FRAMEWORK->gamecore->MouseCursor->Render();
}

bool OptionsMenu::IsTransition()
{
	return false;
}

}; //namespace OpenApoc


#include "optionsmenu.h"
#include "basescreen.h"
#include "../../framework/framework.h"
#include "../../transitions/transitions.h"

OptionsMenu::OptionsMenu()
{
	menuform = GAMECORE->GetForm("FORM_OPTIONSMENU");
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
	GAMECORE->MouseCursor->EventOccured( e );

	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			delete FRAMEWORK->ProgramStages->Pop();
			return;
		}
	}

    if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick )
    {
        if( e->Data.Forms.RaisedBy->Name == "BUTTON_TEST_XCOMBASE" )
        {
            FRAMEWORK->ProgramStages->Push( new TransitionFadeAcross( new BaseScreen(), FRAMES_PER_SECOND >> 2 ) );
            return;
        }
    }
}

void OptionsMenu::Update()
{
	menuform->Update();
}

void OptionsMenu::Render()
{
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
	menuform->Render();
	GAMECORE->MouseCursor->Render();
}

bool OptionsMenu::IsTransition()
{
	return false;
}

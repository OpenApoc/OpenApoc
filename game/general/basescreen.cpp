
#include "basescreen.h"
#include "../../framework/framework.h"

namespace OpenApoc {

BaseScreen::BaseScreen()
{
    basescreenform = GAMECORE->GetForm("FORM_BASESCREEN");
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
        std::cerr << "button " << e->Data.Forms.RaisedBy->Name << " clicked.\n";
        return;
    }
}

void BaseScreen::Update()
{
    basescreenform->Update();
}

void BaseScreen::Render()
{
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
    basescreenform->Render();
	GAMECORE->MouseCursor->Render();
}

bool BaseScreen::IsTransition()
{
	return false;
}

}; //namespace OpenApoc

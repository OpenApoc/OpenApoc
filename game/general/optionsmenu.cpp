
#include "optionsmenu.h"
#include "../../framework/framework.h"

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

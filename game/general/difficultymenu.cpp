
#include "../../framework/framework.h"
#include "difficultymenu.h"
#include "../../transitions/transitions.h"

#include <iostream>

DifficultyMenu::DifficultyMenu()
{
	difficultymenuform = GAMECORE->GetForm("FORM_DIFFICULTYMENU");
	assert(difficultymenuform);
}

DifficultyMenu::~DifficultyMenu()
{
}

void DifficultyMenu::Begin()
{
}

void DifficultyMenu::Pause()
{
}

void DifficultyMenu::Resume()
{
}

void DifficultyMenu::Finish()
{
}

void DifficultyMenu::EventOccurred(Event *e)
{
	difficultymenuform->EventOccured( e );
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
		if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY1") == 0)
		{
			std::cerr << "Difficulty1\n";
		}
		else if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY2") == 0)
		{
			std::cerr << "Difficulty2\n";
		}
		else if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY3") == 0)
		{
			std::cerr << "Difficulty3\n";
		}
		else if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY4") == 0)
		{
			std::cerr << "Difficulty4\n";
		}
		else if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY5") == 0)
		{
			std::cerr << "Difficulty5\n";
		}
		else
		{
			std::cerr << "Unknown button pressed: " << e->Data.Forms.RaisedBy->Name
				<< "\n";
		}
	}
}

void DifficultyMenu::Update()
{
	difficultymenuform->Update();
}

void DifficultyMenu::Render()
{
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
	difficultymenuform->Render();
	GAMECORE->MouseCursor->Render();
}

bool DifficultyMenu::IsTransition()
{
	return false;
}

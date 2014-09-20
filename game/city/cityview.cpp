#include "cityview.h"

CityView::CityView()
{
}

CityView::~CityView()
{
}

void CityView::Begin()
{
}

void CityView::Pause()
{
}

void CityView::Resume()
{
}

void CityView::Finish()
{
}

void CityView::EventOccurred(Event *e)
{
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

void CityView::Update()
{
}

void CityView::Render()
{
}

bool CityView::IsTransition()
{
	return false;
}


#include "fadein.h"
#include "../framework/framework.h"

TransitionFadeIn::TransitionFadeIn( Stage* Target, ALLEGRO_COLOR Source, int Frames )
{
	targetStage = Target;
	transitionFrom = Source;
	transitionFrames = Frames;
	currentFrame = transitionFrames;
	targetRender = 0;
}

TransitionFadeIn::~TransitionFadeIn()
{
	if( targetRender != 0 )
	{
		al_destroy_bitmap( targetRender );
	}
}

void TransitionFadeIn::Begin()
{
	targetRender = al_create_bitmap( FRAMEWORK->Display_GetWidth(), FRAMEWORK->Display_GetHeight() );
	FRAMEWORK->Display_SetTarget( targetRender );
	targetStage->Render();
	FRAMEWORK->Display_SetTarget();
}

void TransitionFadeIn::Pause()
{
}

void TransitionFadeIn::Resume()
{
}

void TransitionFadeIn::Finish()
{
}

void TransitionFadeIn::EventOccurred(Event *e)
{
	if( e->Type == EVENT_KEY_DOWN || e->Type == EVENT_MOUSE_DOWN )
	{
		//currentFrame--;
		FinishTransition();
	}
}

void TransitionFadeIn::Update()
{
	currentFrame--;
	if( currentFrame <= 0 )
	{
		FinishTransition();
	}
}

void TransitionFadeIn::Render()
{
	//targetStage->Render();
	al_draw_bitmap( targetRender, 0, 0, 0 );
	transitionFrom.a = (float)currentFrame / (float)transitionFrames;
	if( transitionFrom.a < 0.0f )
	{
		transitionFrom.a = 0.0f;
	}
	al_draw_filled_rectangle( 0, 0, FRAMEWORK->Display_GetWidth(), FRAMEWORK->Display_GetHeight(), transitionFrom );
}

void TransitionFadeIn::FinishTransition()
{
	Stage* t = targetStage;
	delete Framework::System->ProgramStages->Pop();
	Framework::System->ProgramStages->Push( t );
}

bool TransitionFadeIn::IsTransition()
{
	return true;
}
